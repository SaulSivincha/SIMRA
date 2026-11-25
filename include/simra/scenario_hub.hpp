#pragma once
#include <QWidget>
#include <functional>

namespace simra {
class ScenarioHub final : public QWidget {
public:
  explicit ScenarioHub(std::function<void(int)> openPreset,
                       QWidget *parent = nullptr);
};
} // namespace simra
