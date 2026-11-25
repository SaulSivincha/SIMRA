#include "simra/statistics_panel.hpp"
#include <algorithm>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace simra {
StatisticsPanel::StatisticsPanel(QWidget *parent) : QWidget(parent) {
  auto *outer = new QVBoxLayout(this);
  auto *heading = new QLabel("ESTADO TÉCNICO DE LA SIMULACIÓN");
  heading->setStyleSheet("color:#94A3B8;font-weight:700;letter-spacing:1px;");
  outer->addWidget(heading);
  auto *grid = new QGridLayout;
  QStringList labels = {"Hilos activos", "Hilos en espera",
                        "Buffer de trabajo", "Recursos exclusivos libres",
                        "Recursos reservados", "Conflictos de mutex",
                        "Ciclos de espera", "Robots bloqueados"};
  for (int i = 0; i < int(values_.size()); ++i) {
    auto *tile = new QFrame;
    tile->setStyleSheet(
        "QFrame{background:#273449;border:1px solid #3B4B63;"
        "border-radius:9px;} QLabel{border:0;background:transparent;}");
    auto *box = new QVBoxLayout(tile);
    auto *label = new QLabel(labels[i]);
    label->setStyleSheet("color:#94A3B8;font-size:11px;");
    values_[i] = new QLabel("0");
    values_[i]->setStyleSheet("color:#F1F5F9;font-size:20px;font-weight:700;");
    box->addWidget(label);
    box->addWidget(values_[i]);
    grid->addWidget(tile, i / 2, i % 2);
  }
  outer->addLayout(grid);
  outer->addStretch();
}
void StatisticsPanel::render(const Snapshot &snapshot) {
  int buffer = 0, active = 0, waiting = 0, blocked = 0;
  for (auto &order : snapshot.orders) {
    buffer += order.status == OrderStatus::Pending;
  }
  for (auto &robot : snapshot.robots) {
    active += !robot.threadId.empty() && robot.status != RobotStatus::Stopped;
    waiting += robot.status == RobotStatus::Waiting ||
               robot.status == RobotStatus::Blocked;
    blocked += robot.status == RobotStatus::Blocked;
  }
  const int resources = int(std::count_if(
      snapshot.scenario.stations.begin(), snapshot.scenario.stations.end(),
      [](const auto &station) {
        return station.kind == Terrain::ChargeLight ||
               station.kind == Terrain::ChargeHeavy ||
               station.kind == Terrain::Delivery ||
               station.kind == Terrain::Reception;
      }));
  const int reserved = int(snapshot.stationReservations.size());
  auto &m = snapshot.metrics;
  QStringList values = {
      QString::number(active),
      QString::number(waiting),
      QString::number(buffer),
      QString::number(std::max(0, resources - reserved)),
      QString::number(reserved),
      QString::number(m.conflicts),
      QString::number(m.cycles),
      QString::number(blocked)};
  for (int i = 0; i < int(values_.size()); ++i)
    values_[i]->setText(values[i]);
}
} // namespace simra
