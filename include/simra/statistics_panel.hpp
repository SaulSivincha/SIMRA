#pragma once
#include "simra/state.hpp"
#include <QWidget>
#include <array>
class QLabel;
class QProgressBar;
namespace simra {
class StatisticsPanel final : public QWidget {
  std::array<QLabel *, 12> values_{};
  QProgressBar *progress_{};

public:
  explicit StatisticsPanel(QWidget *parent = nullptr);
  void render(const Snapshot &snapshot);
};
} // namespace simra
