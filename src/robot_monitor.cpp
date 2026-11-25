#include "simra/robot_monitor.hpp"
#include "visual_system.hpp"
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
  outer->setContentsMargins(12, 12, 12, 12);
  outer->addWidget(visual::label("Flota en el almacén", "section"));
  auto *hint = visual::label("Selecciona un robot para destacar su ruta.", "caption");
  hint->setWordWrap(true);
  outer->addWidget(hint);
  auto *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  content_ = new QWidget;
  layout_ = new QVBoxLayout(content_);
  layout_->setContentsMargins(1, 6, 1, 2);
  layout_->setSpacing(8);
  layout_->setAlignment(Qt::AlignTop);
  scroll->setWidget(content_);
  outer->addWidget(scroll);
}
RobotMonitor::Card RobotMonitor::createCard(const std::string &id) {
  Card card;
  auto *frame = visual::panel();
  auto *body = new QVBoxLayout(frame);
  body->setContentsMargins(9, 8, 9, 8);
  body->setSpacing(5);
  auto *title = new QPushButton(QString::fromStdString(id));
  title->setCursor(Qt::PointingHandCursor);
  title->setProperty("variant", "robot");
  title->setCheckable(true);
  title->setAccessibleDescription("Destaca la ruta de este robot en el plano.");
  connect(title, &QPushButton::clicked, this, [this, id] {
    if (selected_)
      selected_(id);
  });
  body->addWidget(title);
  auto *status = visual::label({}, "muted");
  body->addWidget(status);
  auto *detail = visual::label({}, "caption");
  detail->setWordWrap(true);
  body->addWidget(detail);
  auto *bar = new QProgressBar;
  bar->setTextVisible(false);
  bar->setFixedHeight(6);
  body->addWidget(bar);
  auto *thread = visual::label({}, "identifier");
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
void RobotMonitor::render(const Snapshot &snapshot, const std::string &focusedRobot) {
  for (auto &robot : snapshot.robots) {
    auto [it, added] = cards_.try_emplace(robot.id);
    if (added)
      it->second = createCard(robot.id);
    auto &card = it->second;
    if (card.iconKind != robot.kind) {
      card.title->setIcon(visual::icon(
          robot.kind == RobotKind::Light ? "robot" : "heavy"));
      card.iconKind = robot.kind;
    }
    card.title->setChecked(focusedRobot == robot.id);
    card.title->setText(
        QString::fromStdString(robot.id) +
        (robot.kind == RobotKind::Light ? " · ligero" : " · pesado"));
    card.status->setText(statusText(robot.status));
    QString detail = robot.carrying.empty()
                         ? "Sin carga"
                         : "Carga: " + QString::fromStdString(robot.carrying);
    if (!robot.waitingFor.empty())
      detail += "  ·  Espera: " + QString::fromStdString(robot.waitingFor);
    detail += QString("  ·  Batería %1/%2").arg(robot.battery).arg(robot.maxBattery);
    card.detail->setText(detail);
    card.battery->setRange(0, robot.maxBattery);
    card.battery->setValue(robot.battery);
    card.battery->setAccessibleName("Nivel de batería");
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
