#include "simra/statistics_panel.hpp"
#include "visual_system.hpp"
#include <algorithm>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace simra {
StatisticsPanel::StatisticsPanel(QWidget *parent) : QWidget(parent) {
  auto *outer = new QVBoxLayout(this);
  outer->setContentsMargins(12, 12, 12, 12);
  outer->addWidget(visual::label("Estado técnico", "section"));
  auto *hint = visual::label("Hilos, pedidos en cola y recursos compartidos.", "caption");
  hint->setWordWrap(true);
  outer->addWidget(hint);
  auto *grid = new QGridLayout;
  QStringList labels = {"Hilos activos", "Hilos en espera",
                        "Buffer de trabajo", "Recursos exclusivos libres",
                        "Recursos reservados", "Conflictos de mutex",
                        "Ciclos de espera", "Robots bloqueados"};
  for (int i = 0; i < int(values_.size()); ++i) {
    auto *tile = visual::panel();
    auto *box = new QVBoxLayout(tile);
    box->setContentsMargins(10, 8, 10, 8);
    box->setSpacing(2);
    auto *label = visual::label(labels[i], "caption");
    label->setWordWrap(true);
    values_[i] = visual::label("0", "metric");
    values_[i]->setAccessibleName(labels[i]);
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
