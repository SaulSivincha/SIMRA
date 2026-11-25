#include "simra/demo_program.hpp"
#include "simra/state.hpp"
#include <span>
#include <stdexcept>

namespace simra {
namespace {
struct WorkItem {
  const char *cargo;
  int priority;
  const char *restockShelf{};
};

constexpr WorkItem basic[] = {
    {"K01", 3}, {"K_IN", 2, "S04"}, {"K03", 1}, {"K04", 1}};
constexpr WorkItem mixed[] = {
    {"K01", 3}, {"K04", 3}, {"K02", 2}, {"K_IN", 2, "S04"}, {"K06", 1}};
constexpr WorkItem congestion[] = {
    {"K01", 3}, {"K02", 3}, {"K03", 3}, {"K04", 2},
    {"K05", 2}, {"K06", 2}, {"K_IN", 1, "S04"}};
constexpr WorkItem charging[] = {
    {"K01", 3}, {"K_IN", 2, "S04"}, {"K03", 2}, {"K04", 1}};

std::span<const WorkItem> workFor(std::string_view scenario) {
  if (scenario == "basic")
    return basic;
  if (scenario == "mixed")
    return mixed;
  if (scenario == "congestion")
    return congestion;
  if (scenario == "charging")
    return charging;
  throw std::invalid_argument("Demostración desconocida");
}
} // namespace

void loadDemoOrders(WarehouseState &state, std::string_view scenario) {
  const DeliveryOrderCreator delivery;
  const RestockOrderCreator restock;
  int sequence = 1;
  for (const auto &item : workFor(scenario)) {
    const auto id = "O" + std::to_string(sequence++);
    auto order = item.restockShelf
                     ? restock.create(id, item.cargo, item.restockShelf, item.priority)
                     : delivery.create(id, item.cargo, "D01", item.priority);
    if (!state.addOrder(std::move(order)))
      throw std::logic_error("Pedido inválido en la demostración: " + id);
  }
}
} // namespace simra
