#pragma once
#include "simra/domain.hpp"
namespace simra {
enum class EditorTool {
  Inspect,
  Shelf,
  Obstacle,
  LightRobot,
  HeavyRobot,
  LightCharger,
  HeavyCharger,
  Delivery,
  Reception,
  Cargo,
  Erase
};
class ScenarioEditor {
public:
  static void place(Scenario &scenario, Position position, EditorTool tool,
                    int cargoWeight = 5);
  static void resize(Scenario &scenario, int width, int height);
  static void setShelfCapacity(Scenario &scenario, const std::string &id,
                               int capacity);
  static void setRobotBattery(Scenario &scenario, const std::string &id,
                              int battery);
  static void setCargoWeight(Scenario &scenario, const std::string &id,
                             int weight);
};
} // namespace simra
