#include "simra/patterns.hpp"
#include <algorithm>
namespace simra {
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

} // namespace simra
