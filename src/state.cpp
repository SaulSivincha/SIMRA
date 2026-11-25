#include "simra/state.hpp"
#include "simra/path_planner.hpp"
#include <algorithm>
#include <sstream>
namespace simra {
WarehouseState::WarehouseState(Scenario scenario)
    : scenario_(std::move(scenario)),
      terrain_(scenario_.width, scenario_.height, Terrain::Floor) {
  scenario_.validate();
  for (auto p : scenario_.obstacles)
    terrain_.at(p) = Terrain::Obstacle;
  for (auto &s : scenario_.shelves)
    terrain_.at(s.position) = Terrain::Shelf;
  for (auto &s : scenario_.stations) {
    terrain_.at(s.position) = s.kind;
    if (s.kind == Terrain::ChargeLight)
      chargers_[s.id] = LightEquipmentFactory().createChargingStation();
    else if (s.kind == Terrain::ChargeHeavy)
      chargers_[s.id] = IndustrialEquipmentFactory().createChargingStation();
  }
  for (auto &r : scenario_.robots) {
    std::unique_ptr<Robot> type =
        r.kind == RobotKind::Light ? LightEquipmentFactory().createRobot()
                                   : IndustrialEquipmentFactory().createRobot();
    RobotView view{r.id,
                   "",
                   "",
                   "",
                   r.position,
                   r.kind,
                   RobotStatus::Idle,
                   r.battery,
                   type->maxBattery()};
    robots_.emplace(r.id, RuntimeRobot{std::move(type), view, {}, {}, {}});
  }
  for (auto &c : scenario_.cargo)
    cargo_.emplace(c.id, RuntimeCargo{{c.id, c.locationId, c.weight}, {}});
}
void WarehouseState::changedLocked() {
  ++version_;
  cv_.notify_all();
}
Snapshot WarehouseState::snapshot() const {
  std::lock_guard lock(mutex_);
  tickClockLocked();
  Snapshot s;
  s.scenario = scenario_;
  s.metrics = metrics_;
  s.metrics.logicalMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(logicalTime_)
          .count();
  if (started_ != std::chrono::steady_clock::time_point{}) {
    auto now = stopped_ ? stoppedAt_ : std::chrono::steady_clock::now();
    auto paused = pausedTotal_;
    if (paused_ && !stopped_)
      paused += std::chrono::duration_cast<std::chrono::milliseconds>(
          now - pausedAt_);
    s.metrics.realMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - started_)
            .count();
    s.metrics.activeMs =
        std::max<int64_t>(0, s.metrics.realMs - paused.count());
  }
  s.paused = paused_;
  s.stopped = stopped_;
  s.version = version_;
  for (auto &[id, r] : robots_)
    s.robots.push_back(r.view);
  for (auto &[id, o] : orders_)
    s.orders.push_back({id, o->cargoId, o->destinationId, o->robotId, o->kind(),
                        o->status, o->priority});
  for (auto &[id, c] : cargo_)
    s.cargo.push_back(c.view);
  for (auto &[station, robot] : stationOwners_)
    if (!robot.empty())
      s.stationReservations.emplace_back(station, robot);
  return s;
}
Position WarehouseState::targetFor(const Order &order, bool pickup) const {
  std::string id =
      pickup ? cargo_.at(order.cargoId).view.locationId : order.destinationId;
  for (auto &s : scenario_.stations)
    if (s.id == id)
      return s.position;
  for (auto &s : scenario_.shelves)
    if (s.id == id)
      return s.position;
  throw std::invalid_argument("unknown target");
}
bool WarehouseState::freeLocked(Position p, const std::string &robot) const {
  for (auto &[id, r] : robots_)
    if (id != robot && r.view.position == p)
      return false;
  return true;
}
std::optional<std::vector<Position>>
WarehouseState::route(Position from, Position to, bool dynamic,
                      const std::string &robot) const {
  Matrix<uint8_t> blocked(scenario_.width, scenario_.height, 0);
  {
    std::lock_guard lock(mutex_);
    for (int y = 0; y < scenario_.height; ++y)
      for (int x = 0; x < scenario_.width; ++x) {
        auto t = terrain_.at({x, y});
        if (t == Terrain::Shelf || t == Terrain::Obstacle)
          blocked.at({x, y}) = 1;
      }
    for (auto &station : scenario_.stations) {
      auto owner = stationOwners_.find(station.id);
      if (owner == stationOwners_.end() || owner->second != robot)
        blocked.at(station.position) = 1;
    }
    if (dynamic)
      for (auto &[id, r] : robots_)
        if (id != robot)
          blocked.at(r.view.position) = 1;
    if (blocked.valid(from))
      blocked.at(from) = 0;
  }
  return bfs(blocked, from, to);
}
bool WarehouseState::addOrder(std::unique_ptr<Order> order) {
  if (!order)
    return false;
  std::lock_guard lock(mutex_);
  if (stopped_ || orders_.contains(order->id) ||
      !cargo_.contains(order->cargoId))
    return false;
  auto &c = cargo_.at(order->cargoId);
  if (!c.reservedBy.empty())
    return false;
  for (auto &[id, o] : orders_)
    if (o->cargoId == order->cargoId && o->status != OrderStatus::Completed)
      return false;
  bool valid = false;
  for (auto &s : scenario_.stations)
    if (s.id == order->destinationId && s.kind == Terrain::Delivery &&
        order->kind() == OrderKind::Delivery)
      valid = true;
  for (auto &s : scenario_.shelves)
    if (s.id == order->destinationId && order->kind() == OrderKind::Restock)
      valid = true;
  if (!valid)
    return false;
  if (order->kind() == OrderKind::Delivery) {
    bool onShelf = false;
    for (auto &s : scenario_.shelves)
      onShelf |= s.id == c.view.locationId;
    if (!onShelf)
      return false;
  } else {
    bool atReception = false;
    for (auto &s : scenario_.stations)
      atReception |= s.id == c.view.locationId && s.kind == Terrain::Reception;
    if (!atReception)
      return false;
  }
  orders_.emplace(order->id, std::move(order));
  changedLocked();
  return true;
}
std::optional<TaskPlan>
WarehouseState::taskPlan(const std::string &robotId) const {
  std::lock_guard lock(mutex_);
  return robots_.at(robotId).task;
}
std::optional<std::string> WarehouseState::assign(const std::string &robotId) {
  std::unique_lock lock(mutex_);
  if (stopped_ || paused_ || !robots_.contains(robotId))
    return {};
  auto &r = robots_.at(robotId);
  if (!r.orderId.empty() || r.view.status != RobotStatus::Idle)
    return {};
  std::vector<Order *> candidates;
  for (auto &[id, o] : orders_)
    if (o->status == OrderStatus::Pending)
      candidates.push_back(o.get());
  std::sort(candidates.begin(), candidates.end(),
            OrderPriorityComparator{std::chrono::steady_clock::now()});
  for (auto *o : candidates) {
    auto &c = cargo_.at(o->cargoId);
    if (!c.reservedBy.empty() || !RobotCompatibility{}(*r.type, c.view.weight))
      continue;
    if (o->kind() == OrderKind::Restock) {
      auto shelf =
          std::find_if(scenario_.shelves.begin(), scenario_.shelves.end(),
                       [&](auto &s) { return s.id == o->destinationId; });
      int used = 0;
      for (auto &[id, other] : cargo_)
        used += other.view.locationId == o->destinationId;
      for (auto &[id, other] : orders_)
        used += other.get() != o && other->kind() == OrderKind::Restock &&
                other->destinationId == o->destinationId &&
                other->status != OrderStatus::Pending &&
                other->status != OrderStatus::Completed;
      if (shelf == scenario_.shelves.end() || used >= shelf->capacity)
        continue;
    }
    auto pickup = targetFor(*o, true);
    auto destination = targetFor(*o, false);
    auto start = r.view.position;
    auto battery = r.view.battery;
    const Robot *robotType = r.type.get();
    auto seen = version_;
    lock.unlock();
    auto plan = planTask(scenario_, *robotType, start, pickup, destination);
    lock.lock();
    if (version_ != seen || stopped_ || paused_ || r.view.position != start ||
        r.view.battery != battery || !r.orderId.empty() ||
        o->status != OrderStatus::Pending || !c.reservedBy.empty() || !plan ||
        plan->energy > battery)
      continue;
    r.task = plan->stops;
    c.reservedBy = o->id;
    o->status = OrderStatus::Assigned;
    o->robotId = robotId;
    r.orderId = o->id;
    r.view.status = RobotStatus::ToPickup;
    changedLocked();
    SimulationLogger::instance().log(
        EventKind::OrderAssigned,
        o->id + " · " + std::to_string(c.view.weight) + " kg · capacidad " +
            std::to_string(r.type->capacity()) + " kg",
        robotId);
    return o->id;
  }
  return {};
}
bool WarehouseState::canAffordStep(const std::string &robotId,
                                   Position next) const {
  std::unique_lock lock(mutex_);
  const auto &robot = robots_.at(robotId);
  const auto *type = robot.type.get();
  const bool carrying = !robot.view.carrying.empty();
  const int battery = robot.view.battery;
  const auto task = robot.task;
  const auto charger = chargers_.contains(robot.stationId)
                           ? std::optional{robot.stationId}
                           : std::nullopt;
  lock.unlock();
  std::optional<int> remaining = 0;
  if (task)
    remaining = remainingTaskEnergy(scenario_, *type, next, *task, carrying);
  else if (charger)
    for (const auto &station : scenario_.stations)
      if (station.id == *charger)
        remaining = energyToStation(scenario_, *type, next, station.position);
  return remaining && *remaining + type->energy(carrying) <= battery;
}
bool WarehouseState::move(const std::string &robotId, Position next) {
  std::lock_guard lock(mutex_);
  if (stopped_ || paused_ || !robots_.contains(robotId) ||
      !terrain_.valid(next))
    return false;
  auto &r = robots_.at(robotId);
  if (distance(r.view.position, next) != 1)
    return false;
  if (!freeLocked(next, robotId)) {
    ++metrics_.conflicts;
    for (auto &[otherId, other] : robots_)
      if (otherId != robotId && other.view.position == next) {
        const bool changedOwner = r.view.waitingFor != otherId;
        const bool cycle = changedOwner && other.view.waitingFor == robotId;
        if (cycle)
          ++metrics_.cycles;
        r.view.waitingFor = otherId;
        if (cycle)
          SimulationLogger::instance().log(
              EventKind::WaitCycleDetected,
              "ciclo de espera con " + otherId, robotId);
        else if (changedOwner)
          SimulationLogger::instance().log(EventKind::MutexWait,
                                           "espera la celda ocupada por " +
                                               otherId,
                                           robotId);
        break;
      }
    changedLocked();
    return false;
  }
  auto t = terrain_.at(next);
  if (t == Terrain::Shelf || t == Terrain::Obstacle)
    return false;
  if (t == Terrain::ChargeLight || t == Terrain::ChargeHeavy ||
      t == Terrain::Delivery || t == Terrain::Reception) {
    bool owned = false;
    for (auto &s : scenario_.stations)
      if (s.position == next)
        owned =
            stationOwners_.contains(s.id) && stationOwners_.at(s.id) == robotId;
    if (!owned)
      return false;
  }
  int cost = r.type->energy(!r.view.carrying.empty());
  if (r.view.battery < cost)
    return false;
  auto old = r.view.position;
  r.view.position = next;
  r.view.waitingFor.clear();
  for (auto &[id, other] : robots_)
    if (other.view.waitingFor == robotId)
      other.view.waitingFor.clear();
  r.view.battery -= cost;
  ++metrics_.distance;
  metrics_.energy += cost;
  if (!r.stationId.empty()) {
    auto it = std::find_if(scenario_.stations.begin(), scenario_.stations.end(),
                           [&](auto &s) { return s.id == r.stationId; });
    if (it != scenario_.stations.end() && old == it->position) {
      const auto released = r.stationId;
      stationOwners_.erase(r.stationId);
      r.stationId.clear();
      if (it->kind == Terrain::ChargeLight || it->kind == Terrain::ChargeHeavy)
        r.view.status = RobotStatus::Idle;
      SimulationLogger::instance().log(EventKind::ResourceReleased,
                                       "liberó el recurso " + released,
                                       robotId);
    }
  }
  changedLocked();
  return true;
}
bool WarehouseState::pickUp(const std::string &robotId,
                            const std::string &orderId) {
  std::lock_guard lock(mutex_);
  if (stopped_ || paused_ || !robots_.contains(robotId) ||
      !orders_.contains(orderId))
    return false;
  auto &r = robots_.at(robotId);
  auto &o = *orders_.at(orderId);
  auto &c = cargo_.at(o.cargoId);
  if (r.orderId != orderId || o.status != OrderStatus::Assigned ||
      c.reservedBy != orderId || !r.view.carrying.empty())
    return false;
  auto target = targetFor(o, true);
  if (distance(r.view.position, target) >
      (terrain_.at(target) == Terrain::Shelf ? 1 : 0))
    return false;
  c.view.locationId = robotId;
  r.view.carrying = c.view.id;
  o.status = OrderStatus::InProgress;
  r.view.status = RobotStatus::ToDelivery;
  changedLocked();
  return true;
}
bool WarehouseState::deliver(const std::string &robotId,
                             const std::string &orderId) {
  std::lock_guard lock(mutex_);
  if (stopped_ || paused_ || !robots_.contains(robotId) ||
      !orders_.contains(orderId))
    return false;
  auto &r = robots_.at(robotId);
  auto &o = *orders_.at(orderId);
  auto &c = cargo_.at(o.cargoId);
  auto target = targetFor(o, false);
  if (r.orderId != orderId || o.status != OrderStatus::InProgress ||
      r.view.carrying != c.view.id ||
      distance(r.view.position, target) >
          (terrain_.at(target) == Terrain::Shelf ? 1 : 0))
    return false;
  c.view.locationId =
      o.kind() == OrderKind::Delivery ? "DELIVERED" : o.destinationId;
  c.reservedBy.clear();
  r.view.carrying.clear();
  r.orderId.clear();
  r.task.reset();
  r.view.status = RobotStatus::Idle;
  o.status = OrderStatus::Completed;
  o.finished = std::chrono::steady_clock::now();
  metrics_.attentionTotalMs +=
      std::chrono::duration_cast<std::chrono::milliseconds>(o.finished -
                                                            o.created)
          .count();
  ++metrics_.completed;
  changedLocked();
  return true;
}
std::optional<std::string>
WarehouseState::chargerFor(const std::string &robotId) const {
  std::lock_guard lock(mutex_);
  auto it = robots_.find(robotId);
  if (it == robots_.end())
    return {};
  if (!it->second.stationId.empty())
    return it->second.stationId;
  auto kind = it->second.view.kind == RobotKind::Light ? Terrain::ChargeLight
                                                       : Terrain::ChargeHeavy;
  for (auto &s : scenario_.stations)
    if (s.kind == kind)
      return s.id;
  return {};
}
bool WarehouseState::reserveCharger(const std::string &robotId) {
  std::unique_lock lock(mutex_);
  if (stopped_ || paused_ || !robots_.contains(robotId))
    return false;
  auto &robot = robots_.at(robotId);
  if (!robot.orderId.empty())
    return false;
  if (!robot.stationId.empty())
    return chargers_.contains(robot.stationId);

  const auto start = robot.view.position;
  const auto battery = robot.view.battery;
  auto *type = robot.type.get();
  lock.unlock();
  auto candidates = reachableChargers(scenario_, *type, start, battery);
  lock.lock();
  if (stopped_ || paused_ || robot.view.position != start ||
      robot.view.battery != battery || !robot.stationId.empty() ||
      !robot.orderId.empty())
    return false;

  for (const auto *station : candidates) {
    if (!chargers_.at(station->id)->compatible(*type) ||
        stationOwners_.contains(station->id))
      continue;
    stationOwners_.emplace(station->id, robotId);
    robot.stationId = station->id;
    robot.view.status = RobotStatus::ToCharge;
    robot.view.waitingFor.clear();
    changedLocked();
    SimulationLogger::instance().log(EventKind::ResourceAcquired,
                                     "reservó el cargador " + station->id,
                                     robotId);
    return true;
  }
  return false;
}
bool WarehouseState::reserveService(const std::string &robotId,
                                    const std::string &stationId) {
  std::lock_guard lock(mutex_);
  if (stopped_ || paused_ || !robots_.contains(robotId))
    return false;
  auto station =
      std::find_if(scenario_.stations.begin(), scenario_.stations.end(),
                   [&](auto &s) { return s.id == stationId; });
  if (station == scenario_.stations.end() ||
      (station->kind != Terrain::Delivery &&
       station->kind != Terrain::Reception))
    return false;
  auto &robot = robots_.at(robotId);
  if (!robot.stationId.empty() && robot.stationId != stationId)
    return false;
  auto &owner = stationOwners_[stationId];
  if (!owner.empty() && owner != robotId)
    return false;
  owner = robotId;
  robot.stationId = stationId;
  changedLocked();
  SimulationLogger::instance().log(EventKind::ResourceAcquired,
                                   "reservó el recurso " + stationId,
                                   robotId);
  return true;
}
bool WarehouseState::charge(const std::string &robotId, int increment) {
  std::lock_guard lock(mutex_);
  if (stopped_ || paused_ || increment <= 0 || !robots_.contains(robotId))
    return false;
  auto &robot = robots_.at(robotId);
  if (!chargers_.contains(robot.stationId))
    return false;
  auto station =
      std::find_if(scenario_.stations.begin(), scenario_.stations.end(),
                   [&](const auto &s) { return s.id == robot.stationId; });
  if (station == scenario_.stations.end() ||
      robot.view.position != station->position)
    return false;
  const int missing = robot.type->maxBattery() - robot.view.battery;
  if (missing > 0) {
    robot.view.battery += std::min(increment, missing);
    metrics_.recharges += increment >= missing;
  }
  robot.view.status = RobotStatus::Charging;
  robot.view.waitingFor.clear();
  changedLocked();
  return true;
}
void WarehouseState::setRobotStatus(const std::string &id, RobotStatus status,
                                    std::string waiting) {
  std::lock_guard lock(mutex_);
  if (!robots_.contains(id) || (stopped_ && status != RobotStatus::Stopped))
    return;
  auto &runtime = robots_.at(id);
  auto &r = runtime.view;
  if (status == RobotStatus::Waiting && waiting.empty())
    waiting = r.waitingFor;
  if (r.status == status && r.waitingFor == waiting && !r.threadId.empty())
    return;
  r.status = status;
  r.waitingFor = std::move(waiting);
  if (!r.carrying.empty() && !runtime.orderId.empty()) {
    auto &order = *orders_.at(runtime.orderId);
    if (status == RobotStatus::Blocked)
      order.status = OrderStatus::Blocked;
    else if (status == RobotStatus::ToDelivery &&
             order.status == OrderStatus::Blocked)
      order.status = OrderStatus::InProgress;
  }
  if (r.threadId.empty()) {
    std::ostringstream out;
    out << std::this_thread::get_id();
    r.threadId = out.str();
  }
  changedLocked();
  if (status == RobotStatus::Waiting || status == RobotStatus::Blocked)
    SimulationLogger::instance().log(
        EventKind::MutexWait,
        "espera " + (r.waitingFor.empty() ? std::string("un recurso")
                                            : r.waitingFor),
        id);
}
bool WarehouseState::waitChange(uint64_t seen,
                                std::chrono::milliseconds timeout) {
  std::unique_lock lock(mutex_);
  cv_.wait_for(lock, timeout,
               [&] { return stopped_ || (!paused_ && version_ != seen); });
  return !stopped_;
}
bool WarehouseState::waitStep(std::chrono::milliseconds timeout) {
  std::unique_lock lock(mutex_);
  if (lastTick_ == std::chrono::steady_clock::time_point{})
    lastTick_ = std::chrono::steady_clock::now();
  tickClockLocked();
  const auto deadline = logicalTime_ + timeout;
  while (!stopped_) {
    if (paused_) {
      cv_.wait(lock, [&] { return stopped_ || !paused_; });
      continue;
    }
    tickClockLocked();
    if (logicalTime_ >= deadline)
      return true;
    cv_.wait_for(lock, (deadline - logicalTime_) * 100 / speedPercent_);
  }
  return false;
}
void WarehouseState::pause(bool value) {
  {
    std::lock_guard lock(mutex_);
    if (stopped_ || paused_ == value)
      return;
    tickClockLocked();
    if (value)
      pausedAt_ = std::chrono::steady_clock::now();
    else
      pausedTotal_ += std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - pausedAt_);
    paused_ = value;
    changedLocked();
  }
  cv_.notify_all();
}
void WarehouseState::stop() {
  {
    std::lock_guard lock(mutex_);
    if (stopped_)
      return;
    tickClockLocked();
    stoppedAt_ = std::chrono::steady_clock::now();
    if (paused_)
      pausedTotal_ += std::chrono::duration_cast<std::chrono::milliseconds>(
          stoppedAt_ - pausedAt_);
    stopped_ = true;
    for (auto &[id, r] : robots_)
      r.view.status = RobotStatus::Stopped;
    changedLocked();
  }
  cv_.notify_all();
}
void WarehouseState::markStarted() {
  std::lock_guard lock(mutex_);
  if (started_ == std::chrono::steady_clock::time_point{}) {
    started_ = std::chrono::steady_clock::now();
    lastTick_ = started_;
  }
}
void WarehouseState::tickClockLocked() const {
  if (lastTick_ == std::chrono::steady_clock::time_point{} || stopped_)
    return;
  auto now = std::chrono::steady_clock::now();
  if (!paused_)
    logicalTime_ +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - lastTick_) *
        speedPercent_ / 100;
  lastTick_ = now;
}
void WarehouseState::setSpeedPercent(int percent) {
  if (percent < 25 || percent > 400)
    throw std::invalid_argument("invalid speed");
  std::lock_guard lock(mutex_);
  tickClockLocked();
  speedPercent_ = percent;
  changedLocked();
}
int WarehouseState::speedPercent() const {
  std::lock_guard lock(mutex_);
  return speedPercent_;
}
int WarehouseState::stepMs(const std::string &robotId) const {
  std::lock_guard lock(mutex_);
  return robots_.at(robotId).type->stepMs();
}
bool WarehouseState::stopped() const {
  std::lock_guard lock(mutex_);
  return stopped_;
}

} // namespace simra
