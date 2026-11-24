#include "simra/scenario_editor.hpp"
#include <algorithm>

namespace simra {
static bool inside(const Scenario &s, Position p) {
  return p.x >= 0 && p.y >= 0 && p.x < s.width && p.y < s.height;
}
static std::string uniqueId(const Scenario &s, std::string prefix) {
  for (int number = 1;; ++number) {
    auto id = prefix + std::to_string(number);
    bool used = false;
    for (auto &x : s.shelves)
      used |= x.id == id;
    for (auto &x : s.stations)
      used |= x.id == id;
    for (auto &x : s.robots)
      used |= x.id == id;
    for (auto &x : s.cargo)
      used |= x.id == id;
    if (!used)
      return id;
  }
}
void ScenarioEditor::place(Scenario &s, Position p, EditorTool tool,
                           int cargoWeight) {
  if (!inside(s, p))
    throw std::invalid_argument("position outside map");
  if (tool == EditorTool::Inspect)
    return;
  if (tool == EditorTool::Cargo) {
    if (cargoWeight < 1 || cargoWeight > 50)
      throw std::invalid_argument("cargo weight must be 1–50 kg");
    for (auto &shelf : s.shelves)
      if (shelf.position == p) {
        int used = std::count_if(s.cargo.begin(), s.cargo.end(), [&](auto &c) {
          return c.locationId == shelf.id;
        });
        if (used >= shelf.capacity)
          throw std::invalid_argument("shelf is full");
        s.cargo.push_back({uniqueId(s, "K"), cargoWeight, shelf.id});
        return;
      }
    for (auto &station : s.stations)
      if (station.position == p && station.kind == Terrain::Reception) {
        s.cargo.push_back({uniqueId(s, "K"), cargoWeight, station.id});
        return;
      }
    throw std::invalid_argument("place cargo on a shelf or reception");
  }
  s.obstacles.erase(std::remove(s.obstacles.begin(), s.obstacles.end(), p),
                    s.obstacles.end());
  for (auto it = s.shelves.begin(); it != s.shelves.end();) {
    if (it->position == p) {
      auto id = it->id;
      s.cargo.erase(std::remove_if(s.cargo.begin(), s.cargo.end(),
                                   [&](auto &c) { return c.locationId == id; }),
                    s.cargo.end());
      it = s.shelves.erase(it);
    } else
      ++it;
  }
  for (auto it = s.stations.begin(); it != s.stations.end();) {
    if (it->position == p) {
      auto id = it->id;
      s.cargo.erase(std::remove_if(s.cargo.begin(), s.cargo.end(),
                                   [&](auto &c) { return c.locationId == id; }),
                    s.cargo.end());
      it = s.stations.erase(it);
    } else
      ++it;
  }
  s.robots.erase(std::remove_if(s.robots.begin(), s.robots.end(),
                                [&](auto &r) { return r.position == p; }),
                 s.robots.end());
  switch (tool) {
  case EditorTool::Shelf:
    s.shelves.push_back({uniqueId(s, "S"), p, 5});
    break;
  case EditorTool::Obstacle:
    s.obstacles.push_back(p);
    break;
  case EditorTool::LightRobot:
    s.robots.push_back({uniqueId(s, "R"), RobotKind::Light, p, 100});
    break;
  case EditorTool::HeavyRobot:
    s.robots.push_back({uniqueId(s, "R"), RobotKind::Heavy, p, 150});
    break;
  case EditorTool::LightCharger:
    s.stations.push_back({uniqueId(s, "C"), Terrain::ChargeLight, p});
    break;
  case EditorTool::HeavyCharger:
    s.stations.push_back({uniqueId(s, "C"), Terrain::ChargeHeavy, p});
    break;
  case EditorTool::Delivery:
    s.stations.push_back({uniqueId(s, "D"), Terrain::Delivery, p});
    break;
  case EditorTool::Reception:
    s.stations.push_back({uniqueId(s, "P"), Terrain::Reception, p});
    break;
  case EditorTool::Inspect:
  case EditorTool::Cargo:
  case EditorTool::Erase:
    break;
  }
}
void ScenarioEditor::resize(Scenario &s, int width, int height) {
  if (width < 8 || width > 30 || height < 8 || height > 20)
    throw std::invalid_argument("map dimensions must be 8×8 to 30×20");
  Scenario candidate = s;
  candidate.width = width;
  candidate.height = height;
  for (auto p : candidate.obstacles)
    if (!inside(candidate, p))
      throw std::invalid_argument("an obstacle falls outside the new map");
  for (auto &x : candidate.shelves)
    if (!inside(candidate, x.position))
      throw std::invalid_argument("a shelf falls outside the new map");
  for (auto &x : candidate.stations)
    if (!inside(candidate, x.position))
      throw std::invalid_argument("a station falls outside the new map");
  for (auto &x : candidate.robots)
    if (!inside(candidate, x.position))
      throw std::invalid_argument("a robot falls outside the new map");
  s.width = width;
  s.height = height;
}
void ScenarioEditor::setShelfCapacity(Scenario &s, const std::string &id,
                                      int capacity) {
  auto it = std::find_if(s.shelves.begin(), s.shelves.end(),
                         [&](auto &x) { return x.id == id; });
  if (it == s.shelves.end() || capacity < 1)
    throw std::invalid_argument("invalid shelf capacity");
  int used = std::count_if(s.cargo.begin(), s.cargo.end(),
                           [&](auto &c) { return c.locationId == id; });
  if (capacity < used)
    throw std::invalid_argument("capacity below current cargo count");
  it->capacity = capacity;
}
void ScenarioEditor::setRobotBattery(Scenario &s, const std::string &id,
                                     int battery) {
  auto it = std::find_if(s.robots.begin(), s.robots.end(),
                         [&](auto &x) { return x.id == id; });
  if (it == s.robots.end() || battery < 0 ||
      battery > (it->kind == RobotKind::Light ? 100 : 150))
    throw std::invalid_argument("invalid robot battery");
  it->battery = battery;
}
void ScenarioEditor::setCargoWeight(Scenario &s, const std::string &id,
                                    int weight) {
  auto it = std::find_if(s.cargo.begin(), s.cargo.end(),
                         [&](auto &x) { return x.id == id; });
  if (it == s.cargo.end() || weight < 1 || weight > 50)
    throw std::invalid_argument("cargo weight must be 1–50 kg");
  it->weight = weight;
}
} // namespace simra
