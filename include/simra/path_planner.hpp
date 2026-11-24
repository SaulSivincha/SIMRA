#pragma once
#include "simra/domain.hpp"

namespace simra {
struct TaskPlan {
  Position pickup, destination;
};
struct PlannedTask {
  TaskPlan stops;
  int energy;
};

std::optional<PlannedTask> planTask(const Scenario &scenario,
                                    const Robot &robot, Position start,
                                    Position pickup, Position destination);
std::vector<const StationConfig *> reachableChargers(const Scenario &scenario,
                                                     const Robot &robot,
                                                     Position start,
                                                     int battery);
std::optional<int> remainingTaskEnergy(const Scenario &scenario,
                                       const Robot &robot, Position from,
                                       TaskPlan plan, bool carrying);
std::optional<int> energyToStation(const Scenario &scenario, const Robot &robot,
                                   Position from, Position station);
} // namespace simra
