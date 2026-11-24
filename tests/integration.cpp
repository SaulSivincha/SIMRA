#include "simra/core.hpp"
#include <barrier>
#include <cassert>
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
  assert(a != b);
  auto view = collision.snapshot();
  assert(view.metrics.distance == 1);
  assert(view.metrics.conflicts == 1);
  auto charging = s;
  for (auto &st : charging.stations)
    if (st.id == "C01")
      st.position = {1, 0};
  charging.validate();
  WarehouseState charger(charging);
  assert(charger.reserveCharger("R01"));
  assert(!charger.reserveCharger("R02"));
  assert(charger.move("R01", {1, 0}));
  assert(charger.charge("R01"));
  assert(charger.move("R01", {1, 1}));
  assert(charger.reserveCharger("R02"));
  SimulationEngine engine(s);
  DeliveryOrderCreator delivery;
  RestockOrderCreator restock;
  assert(engine.state().addOrder(delivery.create("O1", "K01", "D01", 2)));
  assert(engine.state().addOrder(restock.create("O2", "K_IN", "S04", 1)));
  engine.start();
  engine.pause(true);
  auto before = engine.state().snapshot().metrics.distance;
  auto activeBefore = engine.state().snapshot().metrics.activeMs;
  std::this_thread::sleep_for(std::chrono::milliseconds(600));
  assert(engine.state().snapshot().metrics.distance == before);
  assert(engine.state().snapshot().metrics.activeMs - activeBefore < 50);
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
    assert(o.status == OrderStatus::Completed);
  for (auto &c : result.cargo) {
    if (c.id == "K01")
      assert(c.locationId == "DELIVERED");
    if (c.id == "K_IN")
      assert(c.locationId == "S04");
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
  assert(!automatic.state().snapshot().orders.empty());
  std::cout << "two concurrent orders completed; distance="
            << result.metrics.distance
            << " conflicts=" << result.metrics.conflicts << "\n";
}
