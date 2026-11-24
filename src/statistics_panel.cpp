#include "simra/statistics_panel.hpp"
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

namespace simra {
StatisticsPanel::StatisticsPanel(QWidget *parent) : QWidget(parent) {
  auto *outer = new QVBoxLayout(this);
  auto *heading = new QLabel("RESULTADOS EN TIEMPO REAL");
  heading->setStyleSheet("color:#94A3B8;font-weight:700;letter-spacing:1px;");
  outer->addWidget(heading);
  progress_ = new QProgressBar;
  progress_->setRange(0, 100);
  progress_->setStyleSheet(
      "QProgressBar{background:#111827;border:0;border-radius:5px;"
      "height:22px;text-align:center;color:white;}"
      "QProgressBar::chunk{background:#16A34A;border-radius:5px;}");
  outer->addWidget(progress_);
  auto *grid = new QGridLayout;
  QStringList labels = {"Completados",        "En espera o curso",
                        "Robots bloqueados",  "Conflictos",
                        "Distancia · celdas", "Energía · u.",
                        "Recargas",           "Tiempo activo · s",
                        "Tiempo lógico · s",  "Atención media real · s",
                        "Tiempo real · s",    "Ciclos observados"};
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
  int pending = 0, blocked = 0;
  for (auto &order : snapshot.orders) {
    pending += order.status != OrderStatus::Completed &&
               order.status != OrderStatus::Blocked;
  }
  for (auto &robot : snapshot.robots)
    blocked += robot.status == RobotStatus::Blocked;
  auto &m = snapshot.metrics;
  QStringList values = {
      QString::number(m.completed),
      QString::number(pending),
      QString::number(blocked),
      QString::number(m.conflicts),
      QString::number(m.distance),
      QString::number(m.energy),
      QString::number(m.recharges),
      QString::number(m.activeMs / 1000.0, 'f', 1),
      QString::number(m.logicalMs / 1000.0, 'f', 1),
      m.completed
          ? QString::number(m.attentionTotalMs / 1000.0 / m.completed, 'f', 1)
          : "—",
      QString::number(m.realMs / 1000.0, 'f', 1),
      QString::number(m.cycles)};
  for (int i = 0; i < int(values_.size()); ++i)
    values_[i]->setText(values[i]);
  int total = int(snapshot.orders.size());
  progress_->setValue(total ? m.completed * 100 / total : 0);
  progress_->setFormat(
      total
          ? QString("%1 de %2 pedidos completados").arg(m.completed).arg(total)
          : "Sin pedidos todavía");
}
} // namespace simra
