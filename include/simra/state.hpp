#pragma once
#include "simra/concurrency.hpp"
#include "simra/path_planner.hpp"
#include "simra/patterns.hpp"
#include <map>
#include <mutex>
namespace simra {
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
  int64_t realMs{}, activeMs{}, logicalMs{}, attentionTotalMs{};
};
struct Snapshot {
  Snapshot() = default;
  explicit Snapshot(const Scenario &source) : scenario(source) {}
  Scenario scenario;
  std::vector<RobotView> robots;
  std::vector<OrderView> orders;
  std::vector<CargoView> cargo;
  std::vector<std::pair<std::string, std::string>> stationReservations;
  std::map<std::string, std::vector<Position>> plannedRoutes;
  Metrics metrics;
  bool paused{}, stopped{};
  uint64_t version{};
};
class WarehouseState {
  struct RuntimeRobot {
    std::unique_ptr<Robot> type;
    RobotView view;
    std::string orderId, stationId;
    std::optional<TaskPlan> task;
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
  std::map<std::string, std::vector<Position>> plannedRoutes_;
  std::map<std::string, RuntimeCargo> cargo_;
  std::map<std::string, std::unique_ptr<ChargingStation>> chargers_;
  std::map<std::string, std::unique_ptr<Order>> orders_;
  std::map<std::string, std::string> stationOwners_;
  Metrics metrics_;
  bool paused_{}, stopped_{};
  uint64_t version_{};
  std::chrono::steady_clock::time_point started_{}, pausedAt_{}, stoppedAt_{};
  std::chrono::milliseconds pausedTotal_{};
  mutable std::chrono::steady_clock::time_point lastTick_{};
  mutable std::chrono::nanoseconds logicalTime_{};
  int speedPercent_{100};
  void tickClockLocked() const;
  Position targetFor(const Order &order, bool pickup) const;
  bool freeLocked(Position p, const std::string &robot) const;
  void changedLocked();

public:
  explicit WarehouseState(Scenario scenario);
  Snapshot snapshot() const;
  bool addOrder(std::unique_ptr<Order> order);
  std::optional<std::string> assign(const std::string &robotId);
  std::optional<TaskPlan> taskPlan(const std::string &robotId) const;
  bool move(const std::string &robotId, Position next);
  bool canAffordStep(const std::string &robotId, Position next) const;
  void publishRoute(const std::string &robotId, std::vector<Position> cells);
  std::optional<std::vector<Position>> previewRoute(
      Position from, Position station, const std::string &robot) const;
  bool pickUp(const std::string &robotId, const std::string &orderId);
  bool deliver(const std::string &robotId, const std::string &orderId);
  bool reserveCharger(const std::string &robotId);
  bool reserveService(const std::string &robotId, const std::string &stationId);
  bool charge(const std::string &robotId, int increment = 150);
  void setRobotStatus(const std::string &id, RobotStatus status,
                      std::string waiting = {});
  bool waitChange(uint64_t seen, std::chrono::milliseconds timeout);
  bool waitStep(std::chrono::milliseconds timeout);
  void pause(bool value);
  void stop();
  void markStarted();
  void setSpeedPercent(int percent);
  int speedPercent() const;
  int stepMs(const std::string &robotId) const;
  bool stopped() const;
  std::optional<std::vector<Position>> route(Position from, Position to,
                                             bool dynamic,
                                             const std::string &robot) const;
  std::optional<std::string> chargerFor(const std::string &robotId) const;

private:
  std::optional<std::vector<Position>> findRoute(
      Position from, Position to, bool dynamic, const std::string &robot,
      bool allowStationGoal) const;
};

} // namespace simra
