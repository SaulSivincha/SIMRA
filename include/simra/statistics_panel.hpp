#pragma once
#include "simra/state.hpp"
#include <QWidget>
#include <array>
class QLabel;
namespace simra {
class StatisticsPanel final : public QWidget {
  std::array<QLabel *, 10> values_{};

public:
  explicit StatisticsPanel(QWidget *parent = nullptr);
  void render(const Snapshot &snapshot);
};
} // namespace simra
