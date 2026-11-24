#include "simra/core.hpp"
#include <cmath>
#include <queue>
#include <set>
#include <sstream>

namespace simra {
std::ostream &operator<<(std::ostream &out, Position p) {
  return out << '(' << p.x << ',' << p.y << ')';
}
int distance(Position a, Position b) {
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
static constexpr Position directions[] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
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
    if (s.empty() || !ids.insert(s).second)
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
  auto reachable = [&](Position p) {
    for (auto &r : robots)
      if (bfs(blocked, r.position, p))
        return true;
    return false;
  };
  for (auto &s : stations)
    if (!reachable(s.position))
      throw std::invalid_argument("unreachable station");
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
std::unique_ptr<Order> OrderCreator::create(std::string id, std::string cargo,
                                            std::string destination,
                                            int priority) const {
  if (id.empty() || cargo.empty() || destination.empty() || priority < 0 ||
      priority > 3)
    throw std::invalid_argument("invalid order");
  auto order = make();
  order->id = std::move(id);
  order->cargoId = std::move(cargo);
  order->destinationId = std::move(destination);
  order->priority = priority;
  return order;
}
bool OrderPriorityComparator::operator()(const Order *a, const Order *b) const {
  auto rank = [&](const Order *o) {
    auto age =
        std::chrono::duration_cast<std::chrono::seconds>(now - o->created)
            .count() /
        30;
    return std::min(3, int(o->priority + age));
  };
  if (rank(a) != rank(b))
    return rank(a) > rank(b);
  if (a->created != b->created)
    return a->created < b->created;
  return a->id < b->id;
}
void BasicScenarioBuilder::reset(std::string name) {
  scenario_ = {};
  scenario_.name = std::move(name);
}
void BasicScenarioBuilder::layout() {
  scenario_.shelves = {{"S01", {2, 2}, 5},
                       {"S02", {4, 2}, 5},
                       {"S03", {7, 2}, 5},
                       {"S04", {2, 6}, 5}};
  scenario_.stations = {{"C01", Terrain::ChargeLight, {13, 8}},
                        {"D01", Terrain::Delivery, {13, 2}},
                        {"P01", Terrain::Reception, {0, 5}}};
}
void BasicScenarioBuilder::equipment() {
  scenario_.robots = {{"R01", RobotKind::Light, {0, 0}, 100},
                      {"R02", RobotKind::Light, {0, 2}, 100}};
  scenario_.cargo = {{"K01", 5, "S01"}, {"K02", 5, "S02"}, {"K_IN", 5, "P01"}};
}
Scenario BasicScenarioBuilder::build() {
  scenario_.validate();
  return scenario_;
}
void IndustrialScenarioBuilder::reset(std::string name) {
  scenario_ = {};
  scenario_.name = std::move(name);
}
void IndustrialScenarioBuilder::layout() {
  BasicScenarioBuilder b;
  ScenarioDirector d;
  scenario_ = d.construct(b, scenario_.name);
  scenario_.stations.push_back({"C02", Terrain::ChargeHeavy, {12, 0}});
}
void IndustrialScenarioBuilder::equipment() {
  scenario_.robots.push_back({"R03", RobotKind::Heavy, {0, 4}, 150});
  scenario_.cargo.push_back({"K03", 25, "S03"});
}
Scenario IndustrialScenarioBuilder::build() {
  scenario_.validate();
  return scenario_;
}
void SimulationLogger::log(std::string text, std::string robotId) {
  std::ostringstream out;
  out << std::this_thread::get_id();
  Event event{std::move(text), std::move(robotId), out.str(),
              std::chrono::system_clock::now()};
  {
    std::lock_guard lock(mutex_);
    events_.push_back(event);
    if (events_.size() > 2000)
      events_.pop_front();
  }
  pending_.push(std::move(event));
  if (pending_.size() > 2000)
    pending_.tryPop();
}
std::vector<Event> SimulationLogger::events() const {
  std::lock_guard lock(mutex_);
  return {events_.begin(), events_.end()};
}
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
    robots_.emplace(r.id, RuntimeRobot{std::move(type), view, {}, {}});
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
  Snapshot s;
  s.scenario = scenario_;
  s.metrics = metrics_;
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
std::optional<Position> WarehouseState::accessPosition(const Order &order,
                                                       bool pickup) const {
  std::lock_guard lock(mutex_);
  auto target = targetFor(order, pickup);
  if (terrain_.at(target) != Terrain::Shelf)
    return target;
  for (auto d : directions) {
    auto p = target + d;
    if (terrain_.valid(p) && terrain_.at(p) != Terrain::Shelf &&
        terrain_.at(p) != Terrain::Obstacle)
      return p;
  }
  return {};
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
    auto target = targetFor(*o, true), end = targetFor(*o, false);
    if (terrain_.at(target) == Terrain::Shelf) {
      for (auto d : directions) {
        auto p = target + d;
        if (terrain_.valid(p) && terrain_.at(p) != Terrain::Shelf &&
            terrain_.at(p) != Terrain::Obstacle) {
          target = p;
          break;
        }
      }
    }
    if (terrain_.at(end) == Terrain::Shelf) {
      for (auto d : directions) {
        auto p = end + d;
        if (terrain_.valid(p) && terrain_.at(p) != Terrain::Shelf &&
            terrain_.at(p) != Terrain::Obstacle) {
          end = p;
          break;
        }
      }
    }
    Matrix<uint8_t> blocked(scenario_.width, scenario_.height, 0);
    for (auto p : scenario_.obstacles)
      blocked.at(p) = 1;
    for (auto &s : scenario_.shelves)
      blocked.at(s.position) = 1;
    auto start = r.view.position;
    auto battery = r.view.battery;
    auto robotKind = r.view.kind;
    auto seen = version_;
    lock.unlock();
    auto a = bfs(blocked, start, target), b = bfs(blocked, target, end);
    int chargerDistance = 100000;
    if (a && b)
      for (auto &s : scenario_.stations)
        if (s.kind == (robotKind == RobotKind::Light ? Terrain::ChargeLight
                                                     : Terrain::ChargeHeavy)) {
          auto path = bfs(blocked, end, s.position);
          if (path)
            chargerDistance = std::min(chargerDistance, int(path->size()));
        }
    int emptyCost = robotKind == RobotKind::Light ? 1 : 2;
    int loadedCost = emptyCost + 1;
    int need = a && b
                   ? int(a->size()) * emptyCost + int(b->size()) * loadedCost +
                         chargerDistance * emptyCost + 2
                   : 100000;
    lock.lock();
    if (version_ != seen || stopped_ || paused_ || r.view.position != start ||
        r.view.battery != battery || !r.orderId.empty() ||
        o->status != OrderStatus::Pending || !c.reservedBy.empty() ||
        need > battery)
      continue;
    c.reservedBy = o->id;
    o->status = OrderStatus::Assigned;
    o->robotId = robotId;
    r.orderId = o->id;
    r.view.status = RobotStatus::ToPickup;
    changedLocked();
    return o->id;
  }
  return {};
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
        r.view.waitingFor = otherId;
        if (other.view.waitingFor == robotId)
          ++metrics_.cycles;
        break;
      }
    changedLocked();
    return false;
  }
  auto t = terrain_.at(next);
  if (t == Terrain::Shelf || t == Terrain::Obstacle)
    return false;
  if (t == Terrain::ChargeLight || t == Terrain::ChargeHeavy) {
    bool owned = false;
    for (auto &s : scenario_.stations)
      if (s.position == next)
        owned = stationOwners_[s.id] == robotId;
    if (!owned)
      return false;
  }
  int cost = r.type->energy(!r.view.carrying.empty());
  if (r.view.battery < cost)
    return false;
  auto old = r.view.position;
  r.view.position = next;
  r.view.waitingFor.clear();
  r.view.battery -= cost;
  ++metrics_.distance;
  metrics_.energy += cost;
  if (!r.stationId.empty()) {
    auto it = std::find_if(scenario_.stations.begin(), scenario_.stations.end(),
                           [&](auto &s) { return s.id == r.stationId; });
    if (it != scenario_.stations.end() && old == it->position) {
      stationOwners_.erase(r.stationId);
      r.stationId.clear();
      r.view.status = RobotStatus::Idle;
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
  std::lock_guard lock(mutex_);
  if (stopped_ || paused_ || !robots_.contains(robotId))
    return false;
  auto &r = robots_.at(robotId);
  if (!r.orderId.empty())
    return false;
  auto kind = r.view.kind == RobotKind::Light ? Terrain::ChargeLight
                                              : Terrain::ChargeHeavy;
  for (auto &s : scenario_.stations)
    if (s.kind == kind && chargers_.at(s.id)->compatible(*r.type) &&
        (stationOwners_[s.id].empty() || stationOwners_[s.id] == robotId)) {
      stationOwners_[s.id] = robotId;
      r.stationId = s.id;
      r.view.status = RobotStatus::ToCharge;
      changedLocked();
      return true;
    }
  return false;
}
bool WarehouseState::charge(const std::string &robotId) {
  std::lock_guard lock(mutex_);
  if (stopped_ || paused_ || !robots_.contains(robotId))
    return false;
  auto &r = robots_.at(robotId);
  if (r.stationId.empty())
    return false;
  auto it = std::find_if(scenario_.stations.begin(), scenario_.stations.end(),
                         [&](auto &s) { return s.id == r.stationId; });
  if (it == scenario_.stations.end() || r.view.position != it->position)
    return false;
  r.view.battery = r.type->maxBattery();
  r.view.status = RobotStatus::Charging;
  ++metrics_.recharges;
  changedLocked();
  return true;
}
void WarehouseState::setRobotStatus(const std::string &id, RobotStatus status,
                                    std::string waiting) {
  std::lock_guard lock(mutex_);
  if (!robots_.contains(id))
    return;
  auto &r = robots_.at(id).view;
  r.status = status;
  if (!waiting.empty())
    r.waitingFor = std::move(waiting);
  std::ostringstream out;
  out << std::this_thread::get_id();
  r.threadId = out.str();
  changedLocked();
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
  if (stopped_)
    return false;
  cv_.wait_for(lock, timeout, [&] { return stopped_ || paused_; });
  cv_.wait(lock, [&] { return stopped_ || !paused_; });
  return !stopped_;
}
void WarehouseState::pause(bool value) {
  {
    std::lock_guard lock(mutex_);
    if (stopped_ || paused_ == value)
      return;
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
  if (started_ == std::chrono::steady_clock::time_point{})
    started_ = std::chrono::steady_clock::now();
}
bool WarehouseState::stopped() const {
  std::lock_guard lock(mutex_);
  return stopped_;
}
static bool travel(WarehouseState &state, const std::string &id,
                   Position destination, int ms) {
  int failures = 0;
  while (!state.stopped()) {
    auto snap = state.snapshot();
    auto it = std::find_if(snap.robots.begin(), snap.robots.end(),
                           [&](auto &r) { return r.id == id; });
    if (it == snap.robots.end())
      return false;
    if (it->position == destination)
      return true;
    auto path = state.route(it->position, destination, true, id);
    if (!path || path->empty())
      path = state.route(it->position, destination, false, id);
    if (!path || path->empty()) {
      state.setRobotStatus(id, RobotStatus::Waiting, "ruta");
      state.waitChange(snap.version, std::chrono::milliseconds(250));
      if (++failures > 40) {
        state.setRobotStatus(id, RobotStatus::Blocked, "sin ruta libre");
        return false;
      }
      continue;
    }
    if (!state.waitStep(std::chrono::milliseconds(ms)))
      return false;
    if (state.move(id, path->front())) {
      failures = 0;
      continue;
    }
    state.setRobotStatus(id, RobotStatus::Waiting);
    state.waitChange(snap.version, std::chrono::milliseconds(200));
    if (++failures > 40) {
      state.setRobotStatus(id, RobotStatus::Blocked, "conflicto prolongado");
      return false;
    }
  }
  return false;
}
void RobotWorker::operator()() {
  SimulationLogger::instance().log("hilo iniciado", id);
  state.setRobotStatus(id, RobotStatus::Idle);
  while (!state.stopped()) {
    auto snap = state.snapshot();
    auto r = std::find_if(snap.robots.begin(), snap.robots.end(),
                          [&](auto &x) { return x.id == id; });
    if (r == snap.robots.end())
      break;
    int ms = r->kind == RobotKind::Light ? 300 : 500;
    if (r->battery < r->maxBattery / 3 && r->carrying.empty()) {
      if (state.reserveCharger(id)) {
        auto s = state.snapshot();
        auto station = std::find_if(
            s.scenario.stations.begin(), s.scenario.stations.end(),
            [&](auto &x) { return x.id == state.chargerFor(id).value_or(""); });
        if (station != s.scenario.stations.end() &&
            travel(state, id, station->position, ms) && state.charge(id)) {
          SimulationLogger::instance().log("recarga", id);
          state.waitStep(std::chrono::milliseconds(600));
          auto after = state.snapshot();
          auto rr = std::find_if(after.robots.begin(), after.robots.end(),
                                 [&](auto &x) { return x.id == id; });
          if (rr != after.robots.end())
            for (auto d : directions) {
              auto p = rr->position + d;
              if (state.move(id, p))
                break;
            }
        }
      } else
        state.setRobotStatus(id, RobotStatus::Waiting, "cargador");
    }
    snap = state.snapshot();
    r = std::find_if(snap.robots.begin(), snap.robots.end(),
                     [&](auto &x) { return x.id == id; });
    if (r == snap.robots.end())
      break;
    if (r->status == RobotStatus::Charging) {
      bool left = false;
      for (auto d : directions)
        if (state.move(id, r->position + d)) {
          left = true;
          break;
        }
      if (!left)
        state.waitChange(snap.version, std::chrono::milliseconds(250));
      continue;
    }
    if (r->status == RobotStatus::ToCharge) {
      state.waitChange(snap.version, std::chrono::milliseconds(250));
      continue;
    }
    auto assigned = state.assign(id);
    if (!assigned) {
      state.setRobotStatus(id, RobotStatus::Idle);
      auto seen = state.snapshot().version;
      state.waitChange(seen, std::chrono::milliseconds(250));
      continue;
    }
    SimulationLogger::instance().log("pedido asignado: " + *assigned, id);
    auto now = state.snapshot();
    auto order = std::find_if(now.orders.begin(), now.orders.end(),
                              [&](auto &o) { return o.id == *assigned; });
    if (order == now.orders.end())
      continue;
    auto cargo = std::find_if(now.cargo.begin(), now.cargo.end(),
                              [&](auto &c) { return c.id == order->cargoId; });
    if (cargo == now.cargo.end())
      continue;
    auto shelf =
        std::find_if(now.scenario.shelves.begin(), now.scenario.shelves.end(),
                     [&](auto &s) { return s.id == cargo->locationId; });
    Position pickup{};
    if (shelf != now.scenario.shelves.end()) {
      for (auto d : directions) {
        auto p = shelf->position + d;
        if (state.route(r->position, p, false, id)) {
          pickup = p;
          break;
        }
      }
    } else {
      auto st = std::find_if(
          now.scenario.stations.begin(), now.scenario.stations.end(),
          [&](auto &s) { return s.id == cargo->locationId; });
      if (st != now.scenario.stations.end())
        pickup = st->position;
    }
    while (!state.stopped() &&
           (!travel(state, id, pickup, ms) || !state.pickUp(id, *assigned))) {
      auto seen = state.snapshot().version;
      state.waitChange(seen, std::chrono::milliseconds(500));
    }
    if (state.stopped())
      break;
    now = state.snapshot();
    order = std::find_if(now.orders.begin(), now.orders.end(),
                         [&](auto &o) { return o.id == *assigned; });
    if (order == now.orders.end())
      continue;
    Position destination{};
    if (order->kind == OrderKind::Delivery) {
      for (auto &s : now.scenario.stations)
        if (s.id == order->destinationId)
          destination = s.position;
    } else
      for (auto &s : now.scenario.shelves)
        if (s.id == order->destinationId) {
          for (auto d : directions) {
            auto p = s.position + d;
            if (state.route(r->position, p, false, id)) {
              destination = p;
              break;
            }
          }
        }
    state.setRobotStatus(id, RobotStatus::ToDelivery);
    while (!state.stopped() && (!travel(state, id, destination, ms) ||
                                !state.deliver(id, *assigned))) {
      auto seen = state.snapshot().version;
      state.waitChange(seen, std::chrono::milliseconds(500));
    }
    if (!state.stopped()) {
      SimulationLogger::instance().log("pedido completado: " + *assigned, id);
      if (order->kind == OrderKind::Delivery) {
        while (!state.stopped()) {
          auto current = state.snapshot();
          auto robot =
              std::find_if(current.robots.begin(), current.robots.end(),
                           [&](auto &item) { return item.id == id; });
          if (robot == current.robots.end() || robot->position != destination)
            break;
          bool left = false;
          for (auto d : directions)
            if (state.move(id, destination + d)) {
              left = true;
              break;
            }
          if (left)
            break;
          state.waitChange(current.version, std::chrono::milliseconds(250));
        }
      }
    }
  }
  state.setRobotStatus(id, RobotStatus::Stopped);
  SimulationLogger::instance().log("hilo finalizado", id);
}
SimulationEngine::SimulationEngine(Scenario scenario)
    : state_(std::make_unique<WarehouseState>(std::move(scenario))) {}
SimulationEngine::~SimulationEngine() { stop(); }
void SimulationEngine::start(bool automaticOrders) {
  if (!workers_.empty())
    return;
  state_->markStarted();
  for (auto &r : state_->snapshot().robots)
    workers_.emplace_back(RobotWorker{*state_, r.id});
  if (automaticOrders)
    generator_ = std::thread([this] {
      int sequence = 1;
      auto config = state_->snapshot().scenario;
      while (!state_->stopped()) {
        if (!state_->waitStep(
                std::chrono::milliseconds(config.generationIntervalMs)))
          break;
        auto s = state_->snapshot();
        for (auto &c : s.cargo) {
          if (c.locationId == "DELIVERED")
            continue;
          bool active = false;
          for (auto &o : s.orders)
            if (o.cargoId == c.id)
              active = true;
          if (active)
            continue;
          std::string destination;
          bool reception = false;
          for (auto &st : s.scenario.stations)
            if (st.id == c.locationId && st.kind == Terrain::Reception)
              reception = true;
          if (reception) {
            if (!s.scenario.shelves.empty())
              destination = s.scenario.shelves.front().id;
          } else
            for (auto &st : s.scenario.stations)
              if (st.kind == Terrain::Delivery) {
                destination = st.id;
                break;
              }
          if (destination.empty())
            continue;
          std::string id = "G" + std::to_string(sequence++);
          auto order =
              reception
                  ? RestockOrderCreator().create(id, c.id, destination, 1)
                  : DeliveryOrderCreator().create(id, c.id, destination, 1);
          if (state_->addOrder(std::move(order)))
            SimulationLogger::instance().log("pedido automático: " + id);
          break;
        }
      }
    });
}
void SimulationEngine::pause(bool value) { state_->pause(value); }
void SimulationEngine::stop() {
  if (!state_)
    return;
  state_->stop();
  for (auto &t : workers_)
    if (t.joinable())
      t.join();
  workers_.clear();
  if (generator_.joinable())
    generator_.join();
}
} // namespace simra
