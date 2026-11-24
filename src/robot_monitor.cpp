#include "simra/robot_monitor.hpp"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace simra {
static QString statusText(RobotStatus status) {
  switch (status) {
  case RobotStatus::Idle:
    return "Disponible";
  case RobotStatus::ToPickup:
    return "Camino a recogida";
  case RobotStatus::ToDelivery:
    return "Transportando a destino";
  case RobotStatus::ToCharge:
    return "Camino a recarga";
  case RobotStatus::Charging:
    return "Recargando";
  case RobotStatus::Waiting:
    return "Esperando recurso";
  case RobotStatus::Blocked:
    return "Bloqueado: reintentando";
  case RobotStatus::Stopped:
    return "Detenido";
  }
  return {};
}
RobotMonitor::RobotMonitor(QWidget *parent) : QWidget(parent) {
  auto *outer = new QVBoxLayout(this);
  auto *heading = new QLabel("ROBOTS EN TIEMPO REAL");
  heading->setStyleSheet("color:#94A3B8;font-weight:700;letter-spacing:1px;");
  outer->addWidget(heading);
  auto *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  content_ = new QWidget;
  layout_ = new QVBoxLayout(content_);
  layout_->setSpacing(10);
  layout_->setAlignment(Qt::AlignTop);
  scroll->setWidget(content_);
  outer->addWidget(scroll);
}
RobotMonitor::Card RobotMonitor::createCard(const std::string &id) {
  Card card;
  auto *frame = new QFrame(content_);
  frame->setStyleSheet(
      "QFrame{background:#273449;border:1px solid #3B4B63;"
      "border-radius:10px;} QLabel{border:0;background:transparent;}"
      " QPushButton{border:0;background:transparent;text-align:left;"
      "font-weight:700;font-size:15px;color:#F8FAFC;}");
  auto *body = new QVBoxLayout(frame);
  auto *top = new QHBoxLayout;
  auto *title = new QPushButton(QString::fromStdString(id));
  title->setCursor(Qt::PointingHandCursor);
  connect(title, &QPushButton::clicked, this, [this, id] {
    if (selected_)
      selected_(id);
  });
  top->addWidget(title, 1);
  auto *status = new QLabel;
  status->setStyleSheet("color:#93C5FD;font-weight:600;");
  top->addWidget(status);
  body->addLayout(top);
  auto *detail = new QLabel;
  detail->setStyleSheet("color:#CBD5E1;");
  body->addWidget(detail);
  auto *bar = new QProgressBar;
  bar->setTextVisible(true);
  bar->setStyleSheet(
      "QProgressBar{background:#111827;border:0;border-radius:5px;"
      "height:12px;text-align:center;color:white;}"
      "QProgressBar::chunk{background:#2563EB;border-radius:5px;}");
  body->addWidget(bar);
  auto *thread = new QLabel;
  thread->setStyleSheet("color:#94A3B8;font-size:11px;");
  body->addWidget(thread);
  layout_->addWidget(frame);
  card.root = frame;
  card.title = title;
  card.status = status;
  card.detail = detail;
  card.battery = bar;
  card.thread = thread;
  return card;
}
void RobotMonitor::render(const Snapshot &snapshot) {
  for (auto &robot : snapshot.robots) {
    auto [it, added] = cards_.try_emplace(robot.id);
    if (added)
      it->second = createCard(robot.id);
    auto &card = it->second;
    card.title->setText(
        QString::fromStdString(robot.id) +
        (robot.kind == RobotKind::Light ? " · ligero" : " · pesado"));
    card.status->setText(statusText(robot.status));
    QString detail = robot.carrying.empty()
                         ? "Sin carga"
                         : "Carga: " + QString::fromStdString(robot.carrying);
    if (!robot.waitingFor.empty())
      detail += "  ·  Espera: " + QString::fromStdString(robot.waitingFor);
    card.detail->setText(detail);
    card.battery->setRange(0, robot.maxBattery);
    card.battery->setValue(robot.battery);
    card.battery->setFormat(QString::number(robot.battery) + " / " +
                            QString::number(robot.maxBattery) + " u.");
    card.thread->setText(robot.threadId.empty()
                             ? "Hilo aún no iniciado"
                             : "Hilo: " +
                                   QString::fromStdString(robot.threadId));
    card.thread->setToolTip(card.thread->text());
  }
  for (auto it = cards_.begin(); it != cards_.end();) {
    bool present =
        std::any_of(snapshot.robots.begin(), snapshot.robots.end(),
                    [&](auto &robot) { return robot.id == it->first; });
    if (!present) {
      delete it->second.root;
      it = cards_.erase(it);
    } else
      ++it;
  }
}
} // namespace simra
