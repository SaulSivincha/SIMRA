#include "simra/domain.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <set>
namespace simra {
std::ostream &operator<<(std::ostream &out, Position p) {
  return out << '(' << p.x << ',' << p.y << ')';
}
int distance(Position a, Position b) {
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
std::optional<std::vector<Position>> bfs(const Matrix<uint8_t> &blocked,
                                         Position from, Position to) {
  if (!blocked.valid(from) || !blocked.valid(to) || blocked.at(to))
    return {};
  Matrix<uint8_t> seen(blocked.width(), blocked.height(), 0);
  Matrix<Position> parent(blocked.width(), blocked.height());
  std::queue<Position> queue;
  queue.push(from);
  seen.at(from) = 1;
  while (!queue.empty()) {
    auto p = queue.front();
    queue.pop();
    if (p == to)
      break;
    for (auto d : directions) {
      auto n = p + d;
      if (blocked.valid(n) && !blocked.at(n) && !seen.at(n)) {
        seen.at(n) = 1;
        parent.at(n) = p;
        queue.push(n);
      }
    }
  }
  if (!seen.at(to))
    return {};
  std::vector<Position> path;
  for (auto p = to; !(p == from); p = parent.at(p))
    path.push_back(p);
  std::reverse(path.begin(), path.end());
  return path;
}
void Scenario::validate() const {
  if (width < 8 || width > 30 || height < 8 || height > 20 || robots.empty() ||
      robots.size() > 12 || generationIntervalMs < 100 ||
      generationIntervalMs > 60000)
    throw std::invalid_argument(
        "dimensions, generation interval or robot count invalid");
  std::set<std::string> ids;
  std::set<Position> occupied;
  auto cell = [&](Position p) {
    if (p.x < 0 || p.y < 0 || p.x >= width || p.y >= height ||
        !occupied.insert(p).second)
      throw std::invalid_argument("invalid or overlapping position");
  };
  auto id = [&](const std::string &s) {
    if (s.empty() || s == "DELIVERED" || !ids.insert(s).second)
      throw std::invalid_argument("duplicate or empty id");
  };
  for (auto p : obstacles)
    cell(p);
  for (auto &s : shelves) {
    id(s.id);
    cell(s.position);
    if (s.capacity < 1)
      throw std::invalid_argument("shelf capacity");
  }
  for (auto &s : stations) {
    id(s.id);
    cell(s.position);
    if (s.kind != Terrain::ChargeLight && s.kind != Terrain::ChargeHeavy &&
        s.kind != Terrain::Delivery && s.kind != Terrain::Reception)
      throw std::invalid_argument("station type");
  }
  bool light = false, heavy = false;
  for (auto &r : robots) {
    id(r.id);
    cell(r.position);
    if (r.kind == RobotKind::Light)
      light = true;
    else
      heavy = true;
    int max = r.kind == RobotKind::Light ? 100 : 150;
    if (r.battery < 0 || r.battery > max)
      throw std::invalid_argument("battery range");
  }
  bool lightCharge = false, heavyCharge = false;
  for (auto &s : stations) {
    lightCharge |= s.kind == Terrain::ChargeLight;
    heavyCharge |= s.kind == Terrain::ChargeHeavy;
  }
  if ((light && !lightCharge) || (heavy && !heavyCharge))
    throw std::invalid_argument("missing compatible charger");
  std::map<std::string, int> shelfUsed;
  for (auto &c : cargo) {
    id(c.id);
    if (c.weight < 1 || c.weight > 50)
      throw std::invalid_argument("cargo weight");
    auto shelf = std::find_if(shelves.begin(), shelves.end(),
                              [&](auto &s) { return s.id == c.locationId; });
    auto reception =
        std::find_if(stations.begin(), stations.end(), [&](auto &s) {
          return s.id == c.locationId && s.kind == Terrain::Reception;
        });
    if (shelf == shelves.end() && reception == stations.end())
      throw std::invalid_argument("cargo location");
    if (shelf != shelves.end() && ++shelfUsed[shelf->id] > shelf->capacity)
      throw std::invalid_argument("shelf full");
  }
  Matrix<uint8_t> blocked(width, height, 0);
  for (auto p : obstacles)
    blocked.at(p) = 1;
  for (auto &s : shelves)
    blocked.at(s.position) = 1;
  for (auto &s : stations)
    blocked.at(s.position) = 1;
  auto reachable = [&](Position p) {
    for (auto &r : robots)
      if (bfs(blocked, r.position, p))
        return true;
    return false;
  };
  for (const auto &r : robots) {
    auto connector = r.kind == RobotKind::Light ? Terrain::ChargeLight
                                                : Terrain::ChargeHeavy;
    bool compatible = false;
    for (const auto &station : stations) {
      if (station.kind != connector)
        continue;
      blocked.at(station.position) = 0;
      compatible |= bfs(blocked, r.position, station.position).has_value();
      blocked.at(station.position) = 1;
    }
    if (!compatible)
      throw std::invalid_argument("unreachable compatible charger for " + r.id);
  }
  for (const auto &station : stations) {
    blocked.at(station.position) = 0;
    if (!reachable(station.position))
      throw std::invalid_argument("unreachable station: " + station.id);
    blocked.at(station.position) = 1;
  }
  for (auto &s : shelves) {
    bool ok = false;
    for (auto d : directions) {
      auto p = s.position + d;
      if (blocked.valid(p) && !blocked.at(p) && reachable(p))
        ok = true;
    }
    if (!ok)
      throw std::invalid_argument("inaccessible shelf");
  }
}

} // namespace simra
