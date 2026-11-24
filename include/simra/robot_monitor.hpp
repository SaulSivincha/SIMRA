#pragma once
#include "simra/state.hpp"
#include <QWidget>
#include <functional>
#include <map>

class QVBoxLayout;
class QLabel;
class QProgressBar;
class QPushButton;
namespace simra {
class RobotMonitor final : public QWidget {
  struct Card {
    QWidget *root{};
    QPushButton *title{};
    QLabel *status{}, *detail{}, *thread{};
    QProgressBar *battery{};
  };
  QWidget *content_{};
  QVBoxLayout *layout_{};
  std::map<std::string, Card> cards_;
  std::function<void(std::string)> selected_;
  Card createCard(const std::string &id);

public:
  explicit RobotMonitor(QWidget *parent = nullptr);
  void onSelected(std::function<void(std::string)> callback) {
    selected_ = std::move(callback);
  }
  void render(const Snapshot &snapshot);
};
} // namespace simra
