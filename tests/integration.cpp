#include "check.hpp"
#include "simra/core.hpp"
#include "simra/demo_program.hpp"
#include <barrier>
#include <chrono>
#include <iostream>
#include <thread>
using namespace simra;
int main() {
  BasicScenarioBuilder builder;
  auto s = ScenarioDirector{}.construct(builder, "integration");
  WarehouseState collision(s);
  std::barrier sync(3);
  bool a = false, b = false;
  std::thread one([&] {
    sync.arrive_and_wait();
    a = collision.move("R01", {0, 1});
  });
  std::thread two([&] {
    sync.arrive_and_wait();
    b = collision.move("R02", {0, 1});
  });
  sync.arrive_and_wait();
  one.join();
  two.join();
  CHECK(a != b);
  auto view = collision.snapshot();
  CHECK(view.metrics.distance == 1);
  CHECK(view.metrics.conflicts == 1);
  auto reciprocal = s;
  reciprocal.robots[1].position = {1, 0};
  reciprocal.validate();
  WarehouseState waitCycle(reciprocal);
  CHECK(!waitCycle.move("R01", {1, 0}));
  CHECK(!waitCycle.move("R02", {0, 0}));
  CHECK(waitCycle.snapshot().metrics.cycles >= 1);
  auto charging = s;
  for (auto &st : charging.stations)
    if (st.id == "C01")
      st.position = {1, 0};
  charging.validate();
  WarehouseState charger(charging);
  CHECK(!charger.route({0, 0}, {1, 0}, false, "R01"));
  CHECK(charger.previewRoute({0, 0}, {1, 0}, "R01"));
  CHECK(charger.reserveCharger("R01"));
  CHECK(!charger.reserveCharger("R02"));
  charger.publishRoute("R01", {{1, 0}, {1, 1}});
  CHECK(charger.snapshot().plannedRoutes.at("R01").size() == 2);
  CHECK(charger.move("R01", {1, 0}));
  CHECK(charger.snapshot().plannedRoutes.at("R01").size() == 1);
  CHECK(charger.charge("R01"));
  CHECK(charger.move("R01", {1, 1}));
  CHECK(charger.snapshot().plannedRoutes.empty());
  CHECK(charger.reserveCharger("R02"));
  auto serving = s;
  serving.robots[0].position = {0, 4};
  serving.validate();
  WarehouseState service(serving);
  CHECK(service.reserveService("R01", "P01"));
  CHECK(service.snapshot().stationReservations.size() == 1);
  CHECK(!service.reserveService("R02", "P01"));
  CHECK(service.move("R01", {0, 5}));
  CHECK(service.move("R01", {1, 5}));
  CHECK(service.snapshot().stationReservations.empty());
  CHECK(service.reserveService("R02", "P01"));
  SimulationEngine engine(s);
  DeliveryOrderCreator delivery;
  RestockOrderCreator restock;
  CHECK(engine.state().addOrder(delivery.create("O1", "K01", "D01", 2)));
  CHECK(engine.state().addOrder(restock.create("O2", "K_IN", "S04", 1)));
  engine.start();
  engine.pause(true);
  auto before = engine.state().snapshot().metrics.distance;
  auto activeBefore = engine.state().snapshot().metrics.activeMs;
  std::this_thread::sleep_for(std::chrono::milliseconds(600));
  CHECK(engine.state().snapshot().metrics.distance == before);
  CHECK(engine.state().snapshot().metrics.activeMs - activeBefore < 50);
  engine.pause(false);
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(23);
  while (std::chrono::steady_clock::now() < deadline &&
         engine.state().snapshot().metrics.completed < 2)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  engine.stop();
  auto result = engine.state().snapshot();
  if (result.metrics.completed != 2) {
    std::cerr << "completed=" << result.metrics.completed << "\n";
    for (auto &o : result.orders)
      std::cerr << o.id << " " << int(o.status) << " " << o.robotId << "\n";
    for (auto &r : result.robots)
      std::cerr << r.id << " " << r.position << " " << int(r.status) << "\n";
    return 1;
  }
  for (auto &o : result.orders)
    CHECK(o.status == OrderStatus::Completed);
  for (auto &c : result.cargo) {
    if (c.id == "K01")
      CHECK(c.locationId == "DELIVERED");
    if (c.id == "K_IN")
      CHECK(c.locationId == "S04");
  }
  auto generated = s;
  generated.generationIntervalMs = 100;
  SimulationEngine automatic(generated);
  automatic.start(true);
  auto generationDeadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (std::chrono::steady_clock::now() < generationDeadline &&
         automatic.state().snapshot().orders.empty())
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  automatic.stop();
  CHECK(!automatic.state().snapshot().orders.empty());
  auto energyScenario =
      loadScenario(std::string(SIMRA_SOURCE_DIR) + "/scenarios/charging.json");
  SimulationEngine energy(energyScenario);
  energy.start();
  auto chargeDeadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(10);
  bool sawRouteToCharger = false;
  while (std::chrono::steady_clock::now() < chargeDeadline &&
         energy.state().snapshot().metrics.recharges < 2) {
    const auto current = energy.state().snapshot();
    for (const auto &[id, route] : current.plannedRoutes)
      sawRouteToCharger |= route.back() == Position{1, 1};
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  energy.stop();
  CHECK(sawRouteToCharger);
  CHECK(energy.state().snapshot().metrics.recharges >= 2);
  const auto events = SimulationLogger::instance().events();
  const auto hasLocatedEvent = [&](EventKind kind) {
    return std::any_of(events.begin(), events.end(), [kind](const auto &event) {
      return event.kind == kind && event.location.has_value();
    });
  };
  CHECK(hasLocatedEvent(EventKind::CargoPickedUp));
  CHECK(hasLocatedEvent(EventKind::CargoPlaced));
  CHECK(hasLocatedEvent(EventKind::ChargePulse));
  CHECK(hasLocatedEvent(EventKind::ChargingCompleted));
  for (const auto &[name, orderCount] :
       {std::pair{"basic", 4}, {"mixed", 5}, {"congestion", 7}, {"charging", 4}}) {
    SimulationEngine demo(loadScenario(std::string(SIMRA_SOURCE_DIR) +
                                       "/scenarios/" + name + ".json"));
    loadDemoOrders(demo.state(), name);
    CHECK(demo.state().snapshot().orders.size() == size_t(orderCount));
    demo.state().setSpeedPercent(400);
    demo.start(false);
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    auto current = demo.state().snapshot();
    while (std::chrono::steady_clock::now() < limit &&
           (current.metrics.completed != orderCount ||
            !current.stationReservations.empty())) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      current = demo.state().snapshot();
    }
    std::cout << name << ": " << current.metrics.completed << "/" << orderCount
              << " pedidos, " << current.metrics.recharges << " recargas\n";
    if (current.metrics.completed != orderCount) {
      for (const auto &order : current.orders)
        std::cerr << order.id << ' ' << order.cargoId << ' '
                  << int(order.status) << ' ' << order.robotId << '\n';
      for (const auto &robot : current.robots)
        std::cerr << robot.id << ' ' << robot.position << ' ' << int(robot.status)
                  << ' ' << robot.battery << ' ' << robot.waitingFor << '\n';
    }
    CHECK(current.metrics.completed == orderCount);
    CHECK(current.stationReservations.empty());
    if (std::string_view{name} == "charging")
      CHECK(current.metrics.recharges >= 3);
  }
  std::cout << "two concurrent orders completed; distance="
            << result.metrics.distance
            << " conflicts=" << result.metrics.conflicts << "\n";
}
