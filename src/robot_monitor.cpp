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
static const char *statusColor(RobotStatus status, RobotKind kind) {
  switch (status) {
  case RobotStatus::Waiting:
    return visual::waiting;
  case RobotStatus::Blocked:
    return visual::blocked;
  case RobotStatus::ToCharge:
  case RobotStatus::Charging:
    return visual::energy;
  case RobotStatus::ToPickup:
  case RobotStatus::ToDelivery:
    return kind == RobotKind::Light ? visual::light : visual::heavy;
  case RobotStatus::Idle:
    return visual::delivery;
  case RobotStatus::Stopped:
    return visual::muted;
  }
  return visual::muted;
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
  content_->setObjectName("fleetList");
  content_->setAutoFillBackground(true);
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
          robot.kind == RobotKind::Light ? "robot" : "heavy",
          QColor(robot.kind == RobotKind::Light ? visual::light : visual::heavy)));
      card.iconKind = robot.kind;
    }
    card.title->setChecked(focusedRobot == robot.id);
    card.title->setText(
        QString::fromStdString(robot.id) +
        (robot.kind == RobotKind::Light ? " · ligero" : " · pesado"));
    const auto status = statusText(robot.status);
    const auto statusStyle = QString("color:%1; font-weight:600;")
                                 .arg(statusColor(robot.status, robot.kind));
    if (card.status->text() != status)
      card.status->setText(status);
    if (card.status->styleSheet() != statusStyle)
      card.status->setStyleSheet(statusStyle);
    QString detail = robot.carrying.empty()
                         ? "Sin carga"
                         : "Carga: " + QString::fromStdString(robot.carrying);
    if (!robot.waitingFor.empty())
      detail += "  ·  Espera: " + QString::fromStdString(robot.waitingFor);
    detail += QString("  ·  Batería %1/%2").arg(robot.battery).arg(robot.maxBattery);
    card.detail->setText(detail);
    card.battery->setRange(0, robot.maxBattery);
    card.battery->setValue(robot.battery);
    const auto batteryColor = robot.battery * 4 < robot.maxBattery ? visual::blocked
                              : robot.status == RobotStatus::Charging ? visual::energy
                                                                     : visual::light;
    const auto batteryStyle = QString("QProgressBar::chunk {background:%1; border-radius:3px;}")
                                  .arg(batteryColor);
    if (card.battery->styleSheet() != batteryStyle)
      card.battery->setStyleSheet(batteryStyle);
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
