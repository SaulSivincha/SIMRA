#include "simra/warehouse_view.hpp"
#include "visual_system.hpp"
#include <QCoreApplication>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QMouseEvent>
#include <QPainterPath>
#include <QResizeEvent>
#include <QTimer>
#include <algorithm>

namespace simra {
namespace {
void placeGlyph(QGraphicsPixmapItem &item, const char *name, const QColor &color,
                qreal size, QPointF origin) {
  constexpr int artworkResolution = 96;
  item.setPixmap(visual::symbol(name, color, artworkResolution));
  item.setScale(size / artworkResolution);
  item.setPos(origin);
  item.setTransformationMode(Qt::SmoothTransformation);
}
} // namespace

WarehouseView::WarehouseView(QWidget *parent)
    : QGraphicsView(parent), scene_(new QGraphicsScene(this)),
      animationTimer_(new QTimer(this)), milestoneTimer_(new QTimer(this)) {
  setScene(scene_);
  setRenderHint(QPainter::Antialiasing);
  setBackgroundBrush(QColor(visual::map));
  reduceMotion_ = QCoreApplication::arguments().contains("--reduced-motion");
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);
  setAlignment(Qt::AlignCenter);
  setTransformationAnchor(QGraphicsView::AnchorViewCenter);
  animationTimer_->setInterval(1000 / 30);
  animationTimer_->setTimerType(Qt::PreciseTimer);
  connect(animationTimer_, &QTimer::timeout, this,
          [this] { advanceAnimations(); });
  milestoneTimer_->setSingleShot(true);
  connect(milestoneTimer_, &QTimer::timeout, this, [this] {
    delete milestoneModal_;
    milestoneModal_ = nullptr;
    milestoneRobot_.clear();
    if (pendingMilestone_) {
      const auto next = std::move(*pendingMilestone_);
      pendingMilestone_.reset();
      displayMilestone(next);
    }
  });
}
void WarehouseView::fitMap() {
  if (width_ && height_)
    fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}
void WarehouseView::resizeEvent(QResizeEvent *event) {
  QGraphicsView::resizeEvent(event);
  fitMap();
}
void WarehouseView::mousePressEvent(QMouseEvent *event) {
  if (editHandler_) {
    const auto point = mapToScene(event->pos());
    if (point.x() >= 0 && point.y() >= 0 && point.x() < width_ * Cell &&
        point.y() < height_ * Cell) {
      editHandler_({int(point.x()) / Cell, int(point.y()) / Cell},
                   event->button() == Qt::RightButton);
      return;
    }
  }
  QGraphicsView::mousePressEvent(event);
}
void WarehouseView::resetAnimation() {
  animationTimer_->stop();
  milestoneTimer_->stop();
  delete milestoneModal_;
  milestoneModal_ = nullptr;
  pendingMilestone_.reset();
  pausedAt_.reset();
  motions_.clear();
  clearEffects();
  mapReady_ = false;
}
void WarehouseView::pauseAnimation(bool paused) {
  const auto now = std::chrono::steady_clock::now();
  if (paused && !pausedAt_) {
    pausedAt_ = now;
    animationTimer_->stop();
  } else if (!paused && pausedAt_) {
    for (auto &[id, motion] : motions_)
      motion.started += now - *pausedAt_;
    for (auto &effect : effects_)
      effect.started += now - *pausedAt_;
    pausedAt_.reset();
    if (!motions_.empty() || !effects_.empty())
      animationTimer_->start();
  }
}
void WarehouseView::rebuildMap(const Snapshot &snapshot) {
  animationTimer_->stop();
  milestoneTimer_->stop();
  motions_.clear();
  clearEffects();
  robotVisuals_.clear();
  shelfBadges_.clear();
  stationMarkers_.clear();
  milestoneModal_ = nullptr;
  milestoneRobot_.clear();
  pendingMilestone_.reset();
  scene_->clear();
  const auto &config = snapshot.scenario;
  width_ = config.width;
  height_ = config.height;
  scene_->setSceneRect(0, 0, width_ * Cell, height_ * Cell);
  QPainterPath grid;
  for (int y = 0; y <= height_; ++y) {
    grid.moveTo(0, y * Cell);
    grid.lineTo(width_ * Cell, y * Cell);
  }
  for (int x = 0; x <= width_; ++x) {
    grid.moveTo(x * Cell, 0);
    grid.lineTo(x * Cell, height_ * Cell);
  }
  scene_->addPath(grid, QPen(QColor(visual::grid), 1));
  for (const auto obstacle : config.obstacles) {
    const QPointF position(obstacle.x * Cell, obstacle.y * Cell);
    scene_->addRect(QRectF(position + QPointF(5, 5), QSizeF(42, 42)),
                    QPen(QColor(visual::grid)), QBrush(QColor("#25364D")));
    auto *glyph = scene_->addPixmap({});
    placeGlyph(*glyph, "obstacle", QColor("#BECADC"), 28, position + QPointF(12, 12));
    glyph->setToolTip("Obstáculo");
  }
  for (const auto &shelf : config.shelves) {
    const QPointF position(shelf.position.x * Cell, shelf.position.y * Cell);
    scene_->addRect(QRectF(position + QPointF(6, 6), QSizeF(40, 40)),
                    QPen(QColor("#6A5A43")), QBrush(QColor("#423B30")));
    auto *glyph = scene_->addPixmap({});
    placeGlyph(*glyph, "shelf", QColor(visual::storage), 29, position + QPointF(9, 11));
    glyph->setToolTip(QString("Estante %1").arg(QString::fromStdString(shelf.id)));
    auto *badge = scene_->addText({});
    shelfBadges_.emplace(shelf.id, badge);
    badge->setDefaultTextColor(QColor(visual::storage));
    badge->setScale(0.65);
    badge->setPos(position + QPointF(35, 4));
  }
  for (const auto &station : config.stations) {
    const QPointF position(station.position.x * Cell, station.position.y * Cell);
    const bool outbound = station.kind == Terrain::Delivery;
    const bool inbound = station.kind == Terrain::Reception;
    const QColor color(outbound ? visual::delivery : inbound ? visual::receipt : visual::energy);
    const char *name = outbound ? "deliver" : inbound ? "receive" : "charge";
    const auto id = QString::fromStdString(station.id);
    QPainterPath tile;
    tile.addRoundedRect(QRectF(position + QPointF(5, 5), QSizeF(42, 42)), 7, 7);
    auto *marker = scene_->addPath(tile, QPen(color, 1), QBrush(color.darker(350)));
    stationMarkers_.emplace(station.id, marker);
    auto *glyph = scene_->addPixmap({});
    placeGlyph(*glyph, name, color, 25, position + QPointF(13.5, 7));
    const auto tooltip = QString("%1 %2").arg(outbound ? "Entrega" : inbound ? "Recepción" : "Recarga", id);
    marker->setToolTip(tooltip);
    glyph->setToolTip(tooltip);
    auto *text = scene_->addSimpleText(id);
    QFont font("monospace");
    font.setPixelSize(9);
    font.setBold(true);
    text->setFont(font);
    text->setBrush(color);
    text->setPos(position + QPointF((Cell - text->boundingRect().width()) / 2, 32));
    text->setToolTip(tooltip);
  }
  if (selectedCell_) {
    const auto x = selectedCell_->x * Cell, y = selectedCell_->y * Cell;
    scene_->addRect(x + 1, y + 1, Cell - 2, Cell - 2,
                    QPen(QColor(visual::light), 3), Qt::NoBrush);
  }
  mapReady_ = true;
  fitMap();
}
void WarehouseView::updateRoute(const RobotVisual &robot) {
  const QPointF center(Cell / 2.0, Cell / 2.0);
  const auto current = robot.group->pos();
  QPainterPath path;
  if (current != robot.target || !robot.routeCells.empty()) {
    path.moveTo(current + center);
    if (current != robot.target)
      path.lineTo(robot.target + center);
    for (const auto cell : robot.routeCells)
      path.lineTo(QPointF(cell.x * Cell, cell.y * Cell) + center);
  }
  if (robot.route->path() != path)
    robot.route->setPath(path);
}
QPointF WarehouseView::displayedPosition(
    const std::string &id, std::chrono::steady_clock::time_point now) const {
  const auto found = motions_.find(id);
  if (found == motions_.end())
    return robotVisuals_.at(id).group->pos();
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - found->second.started)
                           .count();
  const auto linear =
      std::clamp(double(elapsed) / found->second.durationMs, 0.0, 1.0);
  return found->second.from + (found->second.to - found->second.from) * linear;
}
void WarehouseView::updateRobots(const Snapshot &snapshot,
                                 const std::string &focusedRobot) {
  std::vector<RobotView> robots = snapshot.robots;
  if (robots.empty())
    for (const auto &robot : snapshot.scenario.robots)
      robots.push_back({robot.id, "", "", "", robot.position, robot.kind,
                        RobotStatus::Idle, robot.battery,
                        robot.kind == RobotKind::Light ? 100 : 150});
  const auto now = pausedAt_.value_or(std::chrono::steady_clock::now());
  for (const auto &robot : robots) {
    const auto target = QPointF(robot.position.x * Cell, robot.position.y * Cell);
    auto [it, added] = robotVisuals_.try_emplace(robot.id);
    if (added) {
      auto *group = new QGraphicsItemGroup;
      group->setZValue(10);
      scene_->addItem(group);
      auto *focus = new QGraphicsEllipseItem(1, 1, Cell - 2, Cell - 2, group);
      auto *body = new QGraphicsPathItem(group);
      QPainterPath bodyPath;
      bodyPath.addRoundedRect(7, 5, 38, 40, 9, 9);
      body->setPath(bodyPath);
      auto *label = new QGraphicsTextItem(group);
      QFont font("monospace");
      font.setBold(true);
      font.setPixelSize(10);
      label->setFont(font);
      label->setDefaultTextColor(QColor(visual::ink));
      label->setPlainText(QString::fromStdString(robot.id));
      label->setPos((Cell - label->boundingRect().width()) / 2, 25);
      auto *glyph = new QGraphicsPixmapItem(group);
      placeGlyph(*glyph, robot.kind == RobotKind::Light ? "robot" : "heavy",
                 QColor(visual::ink), 20, QPointF(16, 7));
      auto *cargo = new QGraphicsPixmapItem(group);
      placeGlyph(*cargo, "cargo", QColor(visual::cargo), 15, QPointF(37, 0));
      auto *battery = new QGraphicsRectItem(12, Cell - 5, 0, 3, group);
      auto *route = scene_->addPath(QPainterPath{});
      route->setData(0, QString::fromStdString(robot.id));
      it->second = {group, focus, body, label, cargo, battery, target, route, {}};
      group->setPos(target);
    }
    auto &visual = it->second;
    if (visual.target != target) {
      const auto current = displayedPosition(robot.id, now);
      visual.target = target;
      if (reduceMotion_) {
        visual.group->setPos(target);
      } else {
        motions_[robot.id] = {current, target, now,
                              robot.kind == RobotKind::Light ? 760 : 1160};
        if (!pausedAt_ && !animationTimer_->isActive())
          animationTimer_->start();
      }
    }
    QColor color(robot.kind == RobotKind::Light ? visual::light : visual::heavy);
    if (robot.status == RobotStatus::Waiting)
      color = QColor(visual::waiting);
    if (robot.status == RobotStatus::Blocked)
      color = QColor(visual::blocked);
    if (robot.status == RobotStatus::Charging)
      color = QColor(visual::energy);
    visual.body->setPen(QPen(color.darker(140), 2));
    visual.body->setBrush(QBrush(color));
    visual.focus->setPen(QPen(color, 3));
    visual.focus->setBrush(Qt::NoBrush);
    const bool focused = focusedRobot == robot.id;
    const auto route = snapshot.plannedRoutes.find(robot.id);
    if (route == snapshot.plannedRoutes.end())
      visual.routeCells.clear();
    else
      visual.routeCells = route->second;
    auto routeColor = color;
    routeColor.setAlpha(focused ? 255 : 195);
    visual.route->setPen(QPen(routeColor, focused ? 4 : 3, Qt::DashLine,
                             Qt::RoundCap, Qt::RoundJoin));
    visual.route->setZValue(focused ? 5 : 4);
    visual.focus->setVisible(focused);
    updateRoute(visual);
    visual.cargo->setVisible(!robot.carrying.empty());
    const double battery = robot.maxBattery > 0
                               ? double(robot.battery) / robot.maxBattery
                               : 0.0;
    visual.battery->setRect(12, Cell - 5, 28 * battery, 3);
    visual.battery->setPen(Qt::NoPen);
    visual.battery->setBrush(
        QBrush(QColor(battery < .25 ? visual::blocked : visual::energy)));
    visual.group->setToolTip(
        QString::fromStdString(robot.id + " · " + robot.threadId));
  }
  for (auto it = robotVisuals_.begin(); it != robotVisuals_.end();) {
    const bool present = std::any_of(robots.begin(), robots.end(),
                                     [&](const auto &robot) {
                                       return robot.id == it->first;
                                     });
    if (!present) {
      delete it->second.route;
      delete it->second.group;
      motions_.erase(it->first);
      it = robotVisuals_.erase(it);
    } else
      ++it;
  }
}
void WarehouseView::advanceAnimations() {
  const auto now = std::chrono::steady_clock::now();
  for (auto it = motions_.begin(); it != motions_.end();) {
    const auto &[id, motion] = *it;
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now - motion.started)
                             .count();
    auto &robot = robotVisuals_.at(id);
    robot.group->setPos(displayedPosition(id, now));
    updateRoute(robot);
    if (elapsed >= motion.durationMs)
      it = motions_.erase(it);
    else
      ++it;
  }
  positionMilestone();
  advanceEffects(now);
  if (motions_.empty() && effects_.empty())
    animationTimer_->stop();
}
void WarehouseView::clearEffects() {
  for (const auto &effect : effects_) {
    delete effect.token;
    delete effect.pulse;
  }
  effects_.clear();
}
void WarehouseView::showAction(const Event &event) {
  if (!event.location || !mapReady_)
    return;
  const bool transfer = event.kind == EventKind::CargoPickedUp ||
                        event.kind == EventKind::CargoPlaced;
  const bool chargeComplete = event.kind == EventKind::ChargingCompleted;
  if (!transfer && !chargeComplete && event.kind != EventKind::ChargePulse &&
      event.kind != EventKind::ResourceReleased)
    return;

  const auto center = [](Position p) {
    return QPointF(p.x * Cell + Cell / 2, p.y * Cell + Cell / 2);
  };
  auto from = center(event.location->from);
  auto to = center(event.location->to);
  // At stations, the robot and cargo occupy the same cell. Offset the
  // package endpoint to the edge of the tile so direction stays visible.
  if (transfer && from == to) {
    const auto offset = QPointF(event.location->from.x == width_ - 1 ? -22 : 22, -15);
    if (event.kind == EventKind::CargoPickedUp)
      from += offset;
    else
      to += offset;
  }
  const QColor color(transfer ? visual::cargo
                    : event.kind == EventKind::ResourceReleased ? visual::delivery
                                                               : visual::energy);
  auto *pulse = scene_->addEllipse(QRectF{}, QPen(color, 2.5), Qt::NoBrush);
  pulse->setZValue(18);
  pulse->setData(0, "actionPulse");
  QGraphicsPathItem *token = nullptr;
  if (transfer && !reduceMotion_) {
    QPainterPath square;
    square.addRect(-4, -4, 8, 8);
    token = scene_->addPath(square, QPen(QColor(visual::ink), 1), QBrush(color));
    token->setZValue(22);
    token->setData(0, "cargoTransfer");
  }
  const auto now = pausedAt_.value_or(std::chrono::steady_clock::now());
  effects_.push_back({token, pulse, from, to, now,
                      chargeComplete ? 1150 : transfer ? 1050 : 560});
  advanceEffects(now);
  if (!pausedAt_ && !animationTimer_->isActive())
    animationTimer_->start();
}
void WarehouseView::advanceEffects(std::chrono::steady_clock::time_point now) {
  for (auto it = effects_.begin(); it != effects_.end();) {
    const auto &effect = *it;
    const auto elapsed = std::chrono::duration<double, std::milli>(now - effect.started).count();
    const auto t = elapsed / effect.durationMs;
    if (t >= 1.0) {
      delete effect.token;
      delete effect.pulse;
      it = effects_.erase(it);
      continue;
    }
    const auto travel = std::clamp(t / .68, 0.0, 1.0);
    if (effect.token) {
      const auto eased = travel * travel * (3 - 2 * travel);
      effect.token->setPos(effect.from + (effect.to - effect.from) * eased);
      effect.token->setVisible(travel < 1.0);
    }
    const auto pulseTime = effect.token ? std::max(0.0, (t - .55) / .45) : t;
    const auto radius = reduceMotion_ ? 19.0 : 9.0 + pulseTime * 19.0;
    effect.pulse->setVisible(!effect.token || pulseTime > 0);
    effect.pulse->setRect(QRectF(effect.to - QPointF(radius, radius),
                                 QSizeF(radius * 2, radius * 2)));
    effect.pulse->setOpacity(reduceMotion_ ? 1.0 : 1.0 - pulseTime);
    ++it;
  }
}
void WarehouseView::positionMilestone() {
  if (!milestoneModal_)
    return;
  const auto robot = robotVisuals_.find(milestoneRobot_);
  if (robot == robotVisuals_.end()) {
    milestoneFocus_->hide();
    milestonePointer_->hide();
    milestoneModal_->setPos((scene_->width() - milestoneSize_.width()) / 2, 8);
    return;
  }
  const auto position = robot->second.group->pos();
  const auto x = std::clamp(position.x() + Cell / 2 - milestoneSize_.width() / 2,
                            4.0, scene_->width() - milestoneSize_.width() - 4.0);
  const auto above = position.y() - milestoneSize_.height() - 10;
  const auto y = above >= 4.0 ? above : position.y() + Cell + 10.0;
  milestoneModal_->setPos(x, y);
  const auto localRobot = position - milestoneModal_->pos();
  milestoneFocus_->setPos(localRobot);
  const auto centerX = localRobot.x() + Cell / 2;
  const auto edgeX = std::clamp(centerX, 10.0, milestoneSize_.width() - 10.0);
  const bool cardAbove = above >= 4.0;
  milestonePointer_->setLine(edgeX, cardAbove ? milestoneSize_.height() : 0.0,
                             centerX, localRobot.y() + (cardAbove ? 1 : Cell - 1));
  milestoneFocus_->show();
  milestonePointer_->show();
}
void WarehouseView::showMilestone(const QString &title, const QString &detail,
                                  const QColor &accent,
                                  const std::string &robotId, int priority) {
  const Milestone next{title, detail, accent, robotId, priority};
  if (!milestoneModal_) {
    displayMilestone(next);
    return;
  }
  if (!pendingMilestone_ || priority >= pendingMilestone_->priority)
    pendingMilestone_ = next;
  // Give the current message time to be read, then show the most relevant
  // recent event. Avoid building a queue of stale map notifications.
  constexpr int fullDuration = 6000;
  constexpr int minimumReadingTime = 2500;
  const int remaining = milestoneTimer_->remainingTime();
  if (remaining > fullDuration - minimumReadingTime)
    milestoneTimer_->start(remaining - (fullDuration - minimumReadingTime));
  else if (milestoneTimer_->interval() == fullDuration)
    milestoneTimer_->start(0);
}
void WarehouseView::displayMilestone(const Milestone &milestone) {
  milestoneModal_ = new QGraphicsItemGroup;
  milestoneModal_->setZValue(30);
  scene_->addItem(milestoneModal_);
  auto *text = new QGraphicsTextItem(milestoneModal_);
  text->setDefaultTextColor(QColor(visual::ink));
  text->setTextWidth(230);
  text->setHtml("<b>" + milestone.title.toHtmlEscaped() + "</b><br>" +
                milestone.detail.toHtmlEscaped());
  text->setPos(10, 7);
  milestoneSize_ = {250, text->boundingRect().height() + 14};
  auto *bubble = new QGraphicsPathItem(milestoneModal_);
  QPainterPath shape;
  shape.addRoundedRect(0, 0, milestoneSize_.width(), milestoneSize_.height(),
                       8, 8);
  bubble->setPath(shape);
  bubble->setPen(QPen(milestone.accent, 2));
  bubble->setBrush(QBrush(QColor(visual::surface)));
  bubble->setZValue(-1);
  milestoneFocus_ =
      new QGraphicsEllipseItem(1, 1, Cell - 2, Cell - 2, milestoneModal_);
  const QPen accentPen(milestone.accent, 2);
  milestoneFocus_->setPen(accentPen);
  milestonePointer_ = new QGraphicsLineItem(milestoneModal_);
  milestonePointer_->setPen(accentPen);
  milestoneRobot_ = milestone.robotId;
  positionMilestone();
  milestoneTimer_->start(6000);
}
void WarehouseView::render(const Snapshot &snapshot,
                           const std::string &focusedRobot) {
  const bool runtimeSnapshot = !snapshot.robots.empty();
  const bool dimensionsChanged = width_ != snapshot.scenario.width ||
                                 height_ != snapshot.scenario.height;
  if (!mapReady_ || !runtimeSnapshot || dimensionsChanged)
    rebuildMap(snapshot);
  for (const auto &[id, marker] : stationMarkers_) {
    const bool reserved = std::any_of(
        snapshot.stationReservations.begin(), snapshot.stationReservations.end(),
        [&](const auto &reservation) { return reservation.first == id; });
    auto pen = marker->pen();
    pen.setWidth(reserved ? 3 : 1);
    marker->setPen(pen);
  }
  for (const auto &[id, badge] : shelfBadges_) {
    const auto atShelf = [&](const auto &cargo) { return cargo.locationId == id; };
    const auto count = runtimeSnapshot
                           ? std::count_if(snapshot.cargo.begin(), snapshot.cargo.end(), atShelf)
                           : std::count_if(snapshot.scenario.cargo.begin(),
                                           snapshot.scenario.cargo.end(), atShelf);
    const auto label = QString::number(count);
    if (badge->toPlainText() != label)
      badge->setPlainText(label);
  }
  updateRobots(snapshot, focusedRobot);
  positionMilestone();
}
} // namespace simra
