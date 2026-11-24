#pragma once
#include "simra/domain.hpp"
namespace simra {
Scenario loadScenario(const std::string &filename);
void saveScenario(const Scenario &scenario, const std::string &filename);

} // namespace simra
