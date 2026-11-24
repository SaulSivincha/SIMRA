#pragma once
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
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

inline constexpr Position directions[] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
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

} // namespace simra
