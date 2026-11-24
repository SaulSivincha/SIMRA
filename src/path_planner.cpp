#include "simra/path_planner.hpp"
#include <algorithm>

namespace simra {
namespace {
Matrix<uint8_t> travelGrid(const Scenario &scenario) {
  Matrix<uint8_t> grid(scenario.width, scenario.height);
  for (auto position : scenario.obstacles)
    grid.at(position) = 1;
  for (const auto &shelf : scenario.shelves)
    grid.at(shelf.position) = 1;
  for (const auto &station : scenario.stations)
    grid.at(station.position) = 1;
  return grid;
}

std::vector<Position> accessCells(const Scenario &scenario,
                                  const Matrix<uint8_t> &grid,
                                  Position target) {
  for (const auto &shelf : scenario.shelves) {
    if (shelf.position != target)
      continue;
    std::vector<Position> cells;
    for (auto delta : directions) {
      auto cell = target + delta;
      if (grid.valid(cell) && !grid.at(cell))
        cells.push_back(cell);
    }
    return cells;
  }
  return {target};
}

std::optional<int> routeLength(Matrix<uint8_t> grid, Position start,
                               Position target) {
  if (!grid.valid(start) || !grid.valid(target))
    return {};
  grid.at(target) = 0;
  if (auto path = bfs(grid, start, target))
    return static_cast<int>(path->size());
  return {};
}

Terrain chargerKind(const Robot &robot) {
  return robot.kind() == RobotKind::Light ? Terrain::ChargeLight
                                          : Terrain::ChargeHeavy;
}
} // namespace

std::optional<PlannedTask> planTask(const Scenario &scenario,
                                    const Robot &robot, Position start,
                                    Position pickup, Position destination) {
  const auto grid = travelGrid(scenario);
  std::optional<PlannedTask> best;
  for (auto end : accessCells(scenario, grid, destination)) {
    auto chargers = reachableChargers(scenario, robot, end, robot.maxBattery());
    if (chargers.empty())
      continue;
    const int reserve = *routeLength(grid, end, chargers.front()->position) *
                        robot.energy(false);
    for (auto origin : accessCells(scenario, grid, pickup)) {
      auto approach = routeLength(grid, start, origin);
      auto carrying = routeLength(grid, origin, end);
      if (!approach || !carrying)
        continue;
      const int energy = *approach * robot.energy(false) +
                         *carrying * robot.energy(true) + reserve +
                         robot.energy(false);
      if (!best || energy < best->energy)
        best = PlannedTask{{origin, end}, energy};
    }
  }
  return best;
}

std::vector<const StationConfig *> reachableChargers(const Scenario &scenario,
                                                     const Robot &robot,
                                                     Position start,
                                                     int battery) {
  const auto grid = travelGrid(scenario);
  std::vector<std::pair<int, const StationConfig *>> candidates;
  for (const auto &station : scenario.stations) {
    if (station.kind != chargerKind(robot))
      continue;
    auto steps = routeLength(grid, start, station.position);
    if (steps && *steps * robot.energy(false) <= battery)
      candidates.emplace_back(*steps, &station);
  }
  std::sort(candidates.begin(), candidates.end(),
            [](const auto &a, const auto &b) {
              if (a.first != b.first)
                return a.first < b.first;
              return a.second->id < b.second->id;
            });
  std::vector<const StationConfig *> result;
  for (auto [steps, station] : candidates)
    result.push_back(station);
  return result;
}
std::optional<int> remainingTaskEnergy(const Scenario &scenario,
                                       const Robot &robot, Position from,
                                       TaskPlan plan, bool carrying) {
  const auto grid = travelGrid(scenario);
  auto chargers =
      reachableChargers(scenario, robot, plan.destination, robot.maxBattery());
  if (chargers.empty())
    return {};
  auto transport =
      routeLength(grid, carrying ? from : plan.pickup, plan.destination);
  auto pickup =
      carrying ? std::optional<int>{0} : routeLength(grid, from, plan.pickup);
  if (!transport || !pickup)
    return {};
  auto reserve =
      routeLength(grid, plan.destination, chargers.front()->position);
  return (*pickup + *reserve) * robot.energy(false) +
         *transport * robot.energy(true);
}

std::optional<int> energyToStation(const Scenario &scenario, const Robot &robot,
                                   Position from, Position station) {
  if (auto length = routeLength(travelGrid(scenario), from, station))
    return *length * robot.energy(false);
  return {};
}
} // namespace simra
