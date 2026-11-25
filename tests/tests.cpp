#include "check.hpp"
#include "simra/core.hpp"
#include "simra/path_planner.hpp"
#include "simra/scenario_editor.hpp"
#include <barrier>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
using namespace simra;
int main() {
  Position a{1, 2};
  CHECK((a + Position{2, 3} == Position{3, 5}));
  Matrix<int> grid(8, 8, 0);
  CHECK(grid.at({0, 0}) == 0);
  bool threw = false;
  try {
    grid.at({8, 0});
  } catch (const std::out_of_range &) {
    threw = true;
  }
  CHECK(threw);
  Matrix<uint8_t> blocked(8, 8, 0);
  CHECK(bfs(blocked, {0, 0}, {0, 0})->empty());
  blocked.at({1, 0}) = 1;
  CHECK(bfs(blocked, {0, 0}, {2, 0})->size() == 4);
  for (int y = 0; y < 8; ++y)
    blocked.at({1, y}) = 1;
  CHECK(!bfs(blocked, {0, 0}, {2, 0}));
  LightEquipmentFactory light;
  IndustrialEquipmentFactory heavy;
  CHECK(light.createRobot()->capacity() == 10);
  CHECK(heavy.createRobot()->capacity() == 50);
  CHECK(light.createRobot()->stepMs() == 300);
  CHECK(heavy.createRobot()->stepMs() == 500);
  CHECK(light.createChargingStation()->compatible(*light.createRobot()));
  CHECK(!light.createChargingStation()->compatible(*heavy.createRobot()));
  DeliveryOrderCreator delivery;
  RestockOrderCreator restock;
  CHECK(delivery.create("O1", "K1", "D1", 1)->kind() == OrderKind::Delivery);
  CHECK(restock.create("O2", "K2", "S1", 1)->kind() == OrderKind::Restock);
  BasicScenarioBuilder builder;
  ScenarioDirector director;
  auto scenario = director.construct(builder, "test");
  auto plan = planTask(scenario, LightRobot{}, {0, 0}, {2, 2}, {13, 2});
  CHECK(plan);
  CHECK(distance(plan->stops.pickup, {2, 2}) == 1);
  CHECK((plan->stops.destination == Position{13, 2}));
  auto remaining =
      remainingTaskEnergy(scenario, LightRobot{}, {0, 0}, plan->stops, false);
  CHECK(remaining && *remaining + 1 == plan->energy);
  auto clone = scenario.clone();
  clone->robots[0].battery = 20;
  CHECK(scenario.robots[0].battery == 100);
  auto edited = scenario;
  ScenarioEditor::place(edited, {1, 1}, EditorTool::Shelf);
  auto shelfId = edited.shelves.back().id;
  ScenarioEditor::setShelfCapacity(edited, shelfId, 1);
  ScenarioEditor::place(edited, {1, 1}, EditorTool::Cargo, 7);
  CHECK(edited.cargo.back().locationId == shelfId);
  threw = false;
  try {
    ScenarioEditor::place(edited, {1, 1}, EditorTool::Cargo, 7);
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  CHECK(threw);
  ScenarioEditor::setRobotBattery(edited, "R01", 80);
  CHECK(scenario.robots[0].battery == 100);
  threw = false;
  try {
    ScenarioEditor::resize(edited, 8, 8);
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  CHECK(threw);
  auto invalid = scenario;
  invalid.robots[1].id = invalid.robots[0].id;
  threw = false;
  try {
    invalid.validate();
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  CHECK(threw);
  auto older = delivery.create("OLD", "K1", "D1", 0);
  auto newer = delivery.create("NEW", "K2", "D1", 2);
  older->created -= std::chrono::seconds(90);
  CHECK(OrderPriorityComparator{std::chrono::steady_clock::now()}(older.get(),
                                                                  newer.get()));
  WarehouseState state(scenario);
  CHECK(state.addOrder(delivery.create("O1", "K01", "D01", 1)));
  std::barrier sync(3);
  std::optional<std::string> x, y;
  std::thread t1([&] {
    sync.arrive_and_wait();
    x = state.assign("R01");
  });
  std::thread t2([&] {
    sync.arrive_and_wait();
    y = state.assign("R02");
  });
  sync.arrive_and_wait();
  t1.join();
  t2.join();
  CHECK(bool(x) != bool(y));
  auto snapshot = state.snapshot();
  CHECK(snapshot.orders[0].status == OrderStatus::Assigned);
  ThreadSafeQueue<int> queue;
  CHECK(queue.push(4));
  CHECK(queue.size() == 1);
  CHECK(queue.tryPop() == 4);
  CHECK(queue.push(6));
  queue.close();
  CHECK(!queue.push(5));
  CHECK(queue.waitPop() == 6);
  CHECK(!queue.waitPop());
  CHECK(&SimulationLogger::instance() == &SimulationLogger::instance());
  WarehouseState clockState(scenario);
  clockState.setSpeedPercent(200);
  CHECK(clockState.speedPercent() == 200);
  clockState.markStarted();
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  auto clockMetrics = clockState.snapshot().metrics;
  CHECK(clockMetrics.logicalMs > clockMetrics.activeMs);
  clockState.pause(true);
  const auto pausedClock = clockState.snapshot().metrics.logicalMs;
  std::thread delayed([&] {
    CHECK(clockState.waitStep(std::chrono::milliseconds(160)));
    CHECK(clockState.snapshot().metrics.logicalMs >= pausedClock + 160);
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(35));
  CHECK(clockState.snapshot().metrics.logicalMs == pausedClock);
  clockState.pause(false);
  delayed.join();
  auto beforeEvents = SimulationLogger::instance().events().size();
  std::vector<std::thread> writers;
  for (int i = 0; i < 4; ++i)
    writers.emplace_back([] {
      for (int j = 0; j < 20; ++j)
        SimulationLogger::instance().log("test");
    });
  for (auto &writer : writers)
    writer.join();
  CHECK(SimulationLogger::instance().events().size() == beforeEvents + 80);
  SimulationLogger::instance().log(EventKind::ResourceAcquired,
                                   "reservó el recurso D01", "R01");
  const auto taggedEvents = SimulationLogger::instance().events();
  CHECK(taggedEvents.back().kind == EventKind::ResourceAcquired);
  auto path =
      (std::filesystem::temp_directory_path() /
       ("simra-roundtrip-" +
        std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) +
        ".json"))
          .string();
  for (auto name : {"basic", "mixed", "congestion", "charging"}) {
    auto loaded = loadScenario(std::string(SIMRA_SOURCE_DIR) + "/scenarios/" +
                               name + ".json");
    loaded.validate();
    saveScenario(loaded, path);
    auto roundtrip = loadScenario(path);
    CHECK(roundtrip.robots.size() == loaded.robots.size());
  }
  std::filesystem::remove(path);
  SimulationEngine engine(scenario);
  engine.start();
  engine.pause(true);
  CHECK(engine.state().snapshot().paused);
  engine.stop();
  CHECK(engine.state().snapshot().stopped);
  std::cout << "SIMRA core tests passed\n";
}
