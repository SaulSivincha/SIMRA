#pragma once
#include "simra/state.hpp"
#include <thread>
namespace simra {
struct RobotWorker {
  WarehouseState &state;
  std::string id;
  void operator()();
};
class SimulationEngine {
  std::unique_ptr<WarehouseState> state_;
  std::vector<std::thread> workers_;
  std::thread generator_;

public:
  explicit SimulationEngine(Scenario scenario);
  ~SimulationEngine();
  SimulationEngine(const SimulationEngine &) = delete;
  SimulationEngine &operator=(const SimulationEngine &) = delete;
  void start(bool automaticOrders = false);
  void pause(bool value);
  void stop();
  WarehouseState &state() { return *state_; }
};

} // namespace simra
