#include "simra/engine.hpp"
#include <algorithm>
#include <random>
#include <thread>
namespace simra {
namespace {
using namespace std::chrono_literals;

void waitForResource(WarehouseState &state) {
  state.waitChange(state.snapshot().version, 200ms);
}

bool travel(WarehouseState &state, const std::string &id, Position destination,
            RobotStatus phase) {
  const auto interval = std::chrono::milliseconds(state.stepMs(id));
  std::optional<int64_t> waitingSince;
  while (!state.stopped()) {
    auto snapshot = state.snapshot();
    const auto robot =
        std::find_if(snapshot.robots.begin(), snapshot.robots.end(),
                     [&](const auto &r) { return r.id == id; });
    if (robot->position == destination) {
      state.setRobotStatus(id, phase);
      return true;
    }
    auto path = state.route(robot->position, destination, true, id);
    if (!path || !state.canAffordStep(id, path->front()))
      path = state.route(robot->position, destination, false, id);
    if (path && state.canAffordStep(id, path->front())) {
      if (!state.waitStep(interval))
        return false;
      if (state.move(id, path->front())) {
        state.setRobotStatus(id, phase);
        waitingSince.reset();
        continue;
      }
      auto blocked = state.snapshot();
      auto self = std::find_if(blocked.robots.begin(), blocked.robots.end(),
                               [&](const auto &r) { return r.id == id; });
      auto other =
          std::find_if(blocked.robots.begin(), blocked.robots.end(),
                       [&](const auto &r) { return r.id == self->waitingFor; });
      if (other != blocked.robots.end() && other->waitingFor == id &&
          id > other->id) {
        bool moved = false;
        for (auto delta : directions) {
          auto refuge = self->position + delta;
          auto continuation = state.route(refuge, destination, false, id);
          if (refuge == path->front() || !continuation ||
              !state.canAffordStep(id, refuge))
            continue;
          if (state.move(id, refuge)) {
            SimulationLogger::instance().log(EventKind::WaitCycleRecovered,
                                             "cedió el paso", id);
            moved = true;
            break;
          }
        }
        if (moved)
          continue;
      }
    }
    const auto now = state.snapshot().metrics.logicalMs;
    if (!waitingSince)
      waitingSince = now;
    if (now - *waitingSince >= 2000) {
      if (robot->status != RobotStatus::Blocked)
        SimulationLogger::instance().log(EventKind::MutexWait,
                                         "espera prolongada en ruta", id);
      state.setRobotStatus(id, RobotStatus::Blocked, "ruta sin paso libre");
    } else {
      state.setRobotStatus(id, RobotStatus::Waiting, path ? "" : "ruta");
    }
    waitForResource(state);
  }
  return false;
}

bool leaveStation(WarehouseState &state, const std::string &id, Position cell) {
  const auto interval = std::chrono::milliseconds(state.stepMs(id));
  auto position = cell;
  while (state.waitStep(interval)) {
    bool moved = false;
    for (auto delta : directions) {
      const auto next = position + delta;
      if (distance(next, cell) <= distance(position, cell) ||
          !state.move(id, next))
        continue;
      position = next;
      moved = true;
      break;
    }
    // Free the station and its approaches before parking the worker.
    if (distance(position, cell) == 2)
      return true;
    if (moved)
      continue;
    if (position != cell)
      return true;
    waitForResource(state);
  }
  return false;
}

bool acquireService(WarehouseState &state, const std::string &id,
                    const std::string &station) {
  while (!state.stopped()) {
    if (state.reserveService(id, station))
      return true;
    state.setRobotStatus(id, RobotStatus::Waiting, station);
    waitForResource(state);
  }
  return false;
}

bool recharge(WarehouseState &state, const std::string &id) {
  if (!state.reserveCharger(id)) {
    state.setRobotStatus(id, RobotStatus::Waiting, "recarga compatible");
    waitForResource(state);
    return false;
  }
  const auto snapshot = state.snapshot();
  const auto stationId = state.chargerFor(id);
  const auto station = std::find_if(
      snapshot.scenario.stations.begin(), snapshot.scenario.stations.end(),
      [&](const auto &s) { return s.id == stationId; });
  if (!travel(state, id, station->position, RobotStatus::ToCharge))
    return false;
  state.setRobotStatus(id, RobotStatus::Charging);
  SimulationLogger::instance().log(EventKind::ChargingStarted,
                                   "recarga iniciada: " + station->id, id);
  while (state.waitStep(120ms)) {
    if (!state.charge(id, 20))
      continue;
    const auto progress = state.snapshot();
    auto robot = std::find_if(progress.robots.begin(), progress.robots.end(),
                              [&](const auto &r) { return r.id == id; });
    if (robot->battery == robot->maxBattery) {
      SimulationLogger::instance().log(EventKind::ChargingCompleted,
                                       "recarga completada: " + station->id,
                                       id);
      return leaveStation(state, id, station->position);
    }
  }
  return false;
}

bool executeOrder(WarehouseState &state, const std::string &id,
                  const std::string &assigned) {
  const auto snapshot = state.snapshot();
  const auto order =
      std::find_if(snapshot.orders.begin(), snapshot.orders.end(),
                   [&](const auto &o) { return o.id == assigned; });
  const auto cargo =
      std::find_if(snapshot.cargo.begin(), snapshot.cargo.end(),
                   [&](const auto &c) { return c.id == order->cargoId; });
  const auto plan = state.taskPlan(id);
  if (order->kind == OrderKind::Restock &&
      !acquireService(state, id, cargo->locationId))
    return false;
  do {
    if (!travel(state, id, plan->pickup, RobotStatus::ToPickup))
      return false;
  } while (!state.pickUp(id, assigned));

  SimulationLogger::instance().log("mercancía recogida: " + cargo->id, id);
  if (order->kind == OrderKind::Delivery &&
      !acquireService(state, id, order->destinationId))
    return false;
  do {
    if (!travel(state, id, plan->destination, RobotStatus::ToDelivery))
      return false;
  } while (!state.deliver(id, assigned));
  SimulationLogger::instance().log("pedido completado: " + assigned, id);
  return order->kind == OrderKind::Restock ||
         leaveStation(state, id, plan->destination);
}
} // namespace

void RobotWorker::operator()() {
  SimulationLogger::instance().log(EventKind::ThreadStarted, "hilo iniciado",
                                   id);
  while (state.waitStep(std::chrono::milliseconds(0))) {
    const auto snapshot = state.snapshot();
    const auto robot =
        std::find_if(snapshot.robots.begin(), snapshot.robots.end(),
                     [&](const auto &r) { return r.id == id; });
    if (robot->battery < robot->maxBattery / 3) {
      recharge(state, id);
      continue;
    }
    state.setRobotStatus(id, RobotStatus::Idle);
    if (auto order = state.assign(id)) {
      if (!executeOrder(state, id, *order))
        break;
    } else {
      waitForResource(state);
    }
  }
  state.setRobotStatus(id, RobotStatus::Stopped);
  SimulationLogger::instance().log("hilo finalizado", id);
}

SimulationEngine::SimulationEngine(Scenario scenario)
    : state_(std::make_unique<WarehouseState>(std::move(scenario))) {}
SimulationEngine::~SimulationEngine() { stop(); }
void SimulationEngine::start(bool automaticOrders) {
  if (!workers_.empty())
    return;
  state_->markStarted();
  for (auto &r : state_->snapshot().robots)
    workers_.emplace_back(RobotWorker{*state_, r.id});
  if (automaticOrders)
    generator_ = std::thread([this] {
      int sequence = 1;
      auto config = state_->snapshot().scenario;
      std::mt19937 random(config.seed);
      while (!state_->stopped()) {
        if (!state_->waitStep(
                std::chrono::milliseconds(config.generationIntervalMs)))
          break;
        auto s = state_->snapshot();
        std::shuffle(s.cargo.begin(), s.cargo.end(), random);
        for (auto &c : s.cargo) {
          if (c.locationId == "DELIVERED")
            continue;
          bool active = false;
          for (auto &o : s.orders)
            if (o.cargoId == c.id && o.status != OrderStatus::Completed)
              active = true;
          if (active)
            continue;
          std::string destination;
          bool reception = false;
          for (auto &st : s.scenario.stations)
            if (st.id == c.locationId && st.kind == Terrain::Reception)
              reception = true;
          if (reception) {
            for (const auto &shelf : s.scenario.shelves) {
              auto occupied = std::count_if(
                  s.cargo.begin(), s.cargo.end(), [&](const auto &item) {
                    return item.locationId == shelf.id;
                  });
              auto incoming = std::count_if(
                  s.orders.begin(), s.orders.end(), [&](const auto &order) {
                    return order.kind == OrderKind::Restock &&
                           order.destinationId == shelf.id &&
                           order.status != OrderStatus::Completed;
                  });
              if (occupied + incoming < shelf.capacity) {
                destination = shelf.id;
                break;
              }
            }
          } else
            for (auto &st : s.scenario.stations)
              if (st.kind == Terrain::Delivery) {
                destination = st.id;
                break;
              }
          if (destination.empty())
            continue;
          std::string id = "G" + std::to_string(sequence++);
          auto order =
              reception
                  ? RestockOrderCreator().create(id, c.id, destination, 1)
                  : DeliveryOrderCreator().create(id, c.id, destination, 1);
          if (state_->addOrder(std::move(order)))
            SimulationLogger::instance().log("pedido automático: " + id);
          break;
        }
      }
    });
}
void SimulationEngine::pause(bool value) { state_->pause(value); }
void SimulationEngine::stop() {
  if (!state_)
    return;
  state_->stop();
  for (auto &t : workers_)
    if (t.joinable())
      t.join();
  workers_.clear();
  if (generator_.joinable())
    generator_.join();
}

} // namespace simra
