#pragma once
#include "simra/domain.hpp"
#include <QWidget>
#include <array>
#include <functional>

class QLabel;
namespace simra {
class WarehouseView;
class ScenarioHub final : public QWidget {
  WarehouseView *preview_{};
  QLabel *previewTitle_{}, *previewDetails_{};
  std::function<Scenario(int)> readScenario_;
  std::array<std::optional<Scenario>, 4> scenarios_;
  int previewIndex_{-1};
  void preview(int index);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

public:
  explicit ScenarioHub(std::function<void(int)> openPreset,
                       std::function<Scenario(int)> readScenario,
                       QWidget *parent = nullptr);
};
} // namespace simra
