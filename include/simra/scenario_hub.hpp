#pragma once
#include <QWidget>
#include <functional>

namespace simra {
struct Scenario;
class WarehouseView;
class ScenarioHub final : public QWidget {
  WarehouseView *preview_{};
public:
  explicit ScenarioHub(std::function<void(int)> openPreset,
                       QWidget *parent = nullptr);
  void preview(const Scenario &scenario);
};
} // namespace simra
