#pragma once

#include <string_view>

namespace simra {
class WarehouseState;

inline constexpr int demoSpeedPercent = 38;

// Load the predefined work before starting the robot threads.
void loadDemoOrders(WarehouseState &state, std::string_view scenario);
} // namespace simra
