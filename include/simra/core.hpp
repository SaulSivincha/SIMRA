#pragma once
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace simra {
struct Position {
  int x{}, y{};
  bool operator==(const Position &) const = default;
  bool operator<(const Position &p) const {
    return y == p.y ? x < p.x : y < p.y;
  }
  Position operator+(Position p) const { return {x + p.x, y + p.y}; }
};
std::ostream &operator<<(std::ostream &, Position);
int distance(Position a, Position b);

template <class T> class Matrix {
  int w_, h_;
  std::vector<T> cells_;

public:
  Matrix(int w, int h, T value = {})
      : w_(w), h_(h), cells_(size_t(w) * size_t(h), value) {
    if (w <= 0 || h <= 0)
      throw std::invalid_argument("invalid matrix size");
  }
  int width() const { return w_; }
  int height() const { return h_; }
  bool valid(Position p) const {
    return p.x >= 0 && p.y >= 0 && p.x < w_ && p.y < h_;
  }
  T &at(Position p) {
    if (!valid(p))
      throw std::out_of_range("matrix position");
    return cells_[size_t(p.y * w_ + p.x)];
  }
  const T &at(Position p) const {
    if (!valid(p))
      throw std::out_of_range("matrix position");
    return cells_[size_t(p.y * w_ + p.x)];
  }
};
std::optional<std::vector<Position>> bfs(const Matrix<uint8_t> &blocked,
                                         Position from, Position to);

template <class T> class ThreadSafeQueue {
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<T> items_;
  bool closed_{};

public:
  bool push(T item) {
    {
      std::lock_guard lock(mutex_);
      if (closed_)
        return false;
      items_.push_back(std::move(item));
    }
    cv_.notify_one();
    return true;
  }
  std::optional<T> waitPop() {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [&] { return closed_ || !items_.empty(); });
    if (items_.empty())
      return {};
    T item = std::move(items_.front());
    items_.pop_front();
    return item;
  }
  std::optional<T> tryPop() {
    std::lock_guard lock(mutex_);
    if (items_.empty())
      return {};
    T item = std::move(items_.front());
    items_.pop_front();
    return item;
  }
  size_t size() const {
    std::lock_guard lock(mutex_);
    return items_.size();
  }
  void close() {
    {
      std::lock_guard lock(mutex_);
      closed_ = true;
    }
    cv_.notify_all();
  }
};

enum class Terrain {
  Floor,
  Shelf,
  Obstacle,
  ChargeLight,
  ChargeHeavy,
  Delivery,
  Reception
};
enum class RobotKind { Light, Heavy };
enum class OrderKind { Delivery, Restock };
enum class OrderStatus { Pending, Assigned, InProgress, Completed, Blocked };
enum class RobotStatus {
  Idle,
  ToPickup,
  ToDelivery,
  ToCharge,
  Charging,
  Waiting,
  Blocked,
  Stopped
};
struct RobotConfig {
  std::string id;
  RobotKind kind;
  Position position;
  int battery;
};
struct CargoConfig {
  std::string id;
  int weight;
  std::string locationId;
};
struct ShelfConfig {
  std::string id;
  Position position;
  int capacity;
};
struct StationConfig {
  std::string id;
  Terrain kind;
  Position position;
};
template <class T> class Prototype {
public:
  virtual ~Prototype() = default;
  virtual std::unique_ptr<T> clone() const = 0;
};
struct Scenario : Prototype<Scenario> {
  std::string name;
  int width{14}, height{10}, seed{42}, generationIntervalMs{1800};
  std::vector<ShelfConfig> shelves;
  std::vector<StationConfig> stations;
  std::vector<Position> obstacles;
  std::vector<RobotConfig> robots;
  std::vector<CargoConfig> cargo;
  std::unique_ptr<Scenario> clone() const override {
    return std::make_unique<Scenario>(*this);
  }
  void validate() const;
};

class Robot {
public:
  virtual ~Robot() = default;
  virtual RobotKind kind() const = 0;
  virtual int capacity() const = 0;
  virtual int maxBattery() const = 0;
  virtual int energy(bool loaded) const = 0;
  virtual int stepMs() const = 0;
};
class LightRobot final : public Robot {
public:
  RobotKind kind() const override { return RobotKind::Light; }
  int capacity() const override { return 10; }
  int maxBattery() const override { return 100; }
  int energy(bool loaded) const override { return loaded ? 2 : 1; }
  int stepMs() const override { return 300; }
};
class HeavyRobot final : public Robot {
public:
  RobotKind kind() const override { return RobotKind::Heavy; }
  int capacity() const override { return 50; }
  int maxBattery() const override { return 150; }
  int energy(bool loaded) const override { return loaded ? 3 : 2; }
  int stepMs() const override { return 500; }
};
class ChargingStation {
public:
  virtual ~ChargingStation() = default;
  virtual Terrain kind() const = 0;
  bool compatible(const Robot &robot) const {
    return (kind() == Terrain::ChargeLight &&
            robot.kind() == RobotKind::Light) ||
           (kind() == Terrain::ChargeHeavy && robot.kind() == RobotKind::Heavy);
  }
};
class LightChargingStation final : public ChargingStation {
public:
  Terrain kind() const override { return Terrain::ChargeLight; }
};
class IndustrialChargingStation final : public ChargingStation {
public:
  Terrain kind() const override { return Terrain::ChargeHeavy; }
};
class EquipmentFactory {
public:
  virtual ~EquipmentFactory() = default;
  virtual std::unique_ptr<Robot> createRobot() const = 0;
  virtual std::unique_ptr<ChargingStation> createChargingStation() const = 0;
};
class LightEquipmentFactory final : public EquipmentFactory {
public:
  std::unique_ptr<Robot> createRobot() const override {
    return std::make_unique<LightRobot>();
  }
  std::unique_ptr<ChargingStation> createChargingStation() const override {
    return std::make_unique<LightChargingStation>();
  }
};
class IndustrialEquipmentFactory final : public EquipmentFactory {
public:
  std::unique_ptr<Robot> createRobot() const override {
    return std::make_unique<HeavyRobot>();
  }
  std::unique_ptr<ChargingStation> createChargingStation() const override {
    return std::make_unique<IndustrialChargingStation>();
  }
};
struct Order {
  std::string id, cargoId, destinationId;
  int priority{};
  OrderStatus status{OrderStatus::Pending};
  std::string robotId;
  std::chrono::steady_clock::time_point created{
      std::chrono::steady_clock::now()},
      finished{};
  virtual ~Order() = default;
  virtual OrderKind kind() const = 0;
};
struct DeliveryOrder final : Order {
  OrderKind kind() const override { return OrderKind::Delivery; }
};
struct RestockOrder final : Order {
  OrderKind kind() const override { return OrderKind::Restock; }
};
class OrderCreator {
public:
  virtual ~OrderCreator() = default;
  std::unique_ptr<Order> create(std::string id, std::string cargo,
                                std::string destination, int priority) const;

protected:
  virtual std::unique_ptr<Order> make() const = 0;
};
class DeliveryOrderCreator final : public OrderCreator {
protected:
  std::unique_ptr<Order> make() const override {
    return std::make_unique<DeliveryOrder>();
  }
};
class RestockOrderCreator final : public OrderCreator {
protected:
  std::unique_ptr<Order> make() const override {
    return std::make_unique<RestockOrder>();
  }
};
struct OrderPriorityComparator {
  std::chrono::steady_clock::time_point now;
  bool operator()(const Order *a, const Order *b) const;
};
struct RobotCompatibility {
  bool operator()(const Robot &r, int weight) const {
    return weight > 0 && weight <= r.capacity();
  }
};
class ScenarioBuilder {
public:
  virtual ~ScenarioBuilder() = default;
  virtual void reset(std::string name) = 0;
  virtual void layout() = 0;
  virtual void equipment() = 0;
  virtual Scenario build() = 0;
};
class BasicScenarioBuilder final : public ScenarioBuilder {
  Scenario scenario_;

public:
  void reset(std::string name) override;
  void layout() override;
  void equipment() override;
  Scenario build() override;
};
class IndustrialScenarioBuilder final : public ScenarioBuilder {
  BasicScenarioBuilder base_;
  Scenario scenario_;

public:
  void reset(std::string name) override;
  void layout() override;
  void equipment() override;
  Scenario build() override;
};
class ScenarioDirector {
public:
  Scenario construct(ScenarioBuilder &b, std::string name) const {
    b.reset(std::move(name));
    b.layout();
    b.equipment();
    return b.build();
  }
};

struct RobotView {
  std::string id, threadId, carrying, waitingFor;
  Position position;
  RobotKind kind;
  RobotStatus status;
  int battery, maxBattery;
};
struct OrderView {
  std::string id, cargoId, destinationId, robotId;
  OrderKind kind;
  OrderStatus status;
  int priority;
};
struct CargoView {
  std::string id, locationId;
  int weight;
};
struct Metrics {
  int completed{}, conflicts{}, cycles{}, distance{}, energy{}, recharges{};
  int64_t realMs{}, activeMs{}, attentionTotalMs{};
};
struct Snapshot {
  Scenario scenario;
  std::vector<RobotView> robots;
  std::vector<OrderView> orders;
  std::vector<CargoView> cargo;
  Metrics metrics;
  bool paused{}, stopped{};
  uint64_t version{};
};
struct Event {
  std::string text, robotId, threadId;
  std::chrono::system_clock::time_point when;
};
class SimulationLogger {
  mutable std::mutex mutex_;
  std::deque<Event> events_;
  SimulationLogger() = default;

public:
  SimulationLogger(const SimulationLogger &) = delete;
  SimulationLogger &operator=(const SimulationLogger &) = delete;
  static SimulationLogger &instance() {
    static SimulationLogger value;
    return value;
  }
  void log(std::string text, std::string robotId = {});
  std::vector<Event> events() const;
};
class WarehouseState {
  struct RuntimeRobot {
    std::unique_ptr<Robot> type;
    RobotView view;
    std::string orderId, stationId;
  };
  struct RuntimeCargo {
    CargoView view;
    std::string reservedBy;
  };
  Scenario scenario_;
  Matrix<Terrain> terrain_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::map<std::string, RuntimeRobot> robots_;
  std::map<std::string, RuntimeCargo> cargo_;
  std::map<std::string, std::unique_ptr<ChargingStation>> chargers_;
  std::map<std::string, std::unique_ptr<Order>> orders_;
  std::map<std::string, std::string> stationOwners_;
  Metrics metrics_;
  bool paused_{}, stopped_{};
  uint64_t version_{};
  std::chrono::steady_clock::time_point started_{}, pausedAt_{}, stoppedAt_{};
  std::chrono::milliseconds pausedTotal_{};
  Position targetFor(const Order &order, bool pickup) const;
  bool freeLocked(Position p, const std::string &robot) const;
  void changedLocked();

public:
  explicit WarehouseState(Scenario scenario);
  Snapshot snapshot() const;
  bool addOrder(std::unique_ptr<Order> order);
  std::optional<std::string> assign(const std::string &robotId);
  bool move(const std::string &robotId, Position next);
  bool pickUp(const std::string &robotId, const std::string &orderId);
  bool deliver(const std::string &robotId, const std::string &orderId);
  bool reserveCharger(const std::string &robotId);
  bool charge(const std::string &robotId);
  void setRobotStatus(const std::string &id, RobotStatus status,
                      std::string waiting = {});
  bool waitChange(uint64_t seen, std::chrono::milliseconds timeout);
  bool waitStep(std::chrono::milliseconds timeout);
  void pause(bool value);
  void stop();
  void markStarted();
  bool stopped() const;
  std::optional<Position> accessPosition(const Order &order, bool pickup) const;
  std::optional<std::vector<Position>> route(Position from, Position to,
                                             bool dynamic,
                                             const std::string &robot) const;
  std::optional<std::string> chargerFor(const std::string &robotId) const;
};
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
Scenario loadScenario(const std::string &filename);
void saveScenario(const Scenario &scenario, const std::string &filename);
} // namespace simra
