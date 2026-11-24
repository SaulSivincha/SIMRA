#pragma once
#include "simra/domain.hpp"
#include <QWidget>
#include <functional>

class QLabel;
namespace simra {
class WarehouseView;
class ScenarioHub final : public QWidget {
  WarehouseView *preview_{};
  QLabel *description_{};

public:
  explicit ScenarioHub(std::function<void(int)> openPreset,
                       QWidget *parent = nullptr);
  void preview(const Scenario &scenario);
};
} // namespace simra
