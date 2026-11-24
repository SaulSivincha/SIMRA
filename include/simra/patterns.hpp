#pragma once
#include "simra/domain.hpp"
#include <algorithm>
namespace simra {
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

} // namespace simra
