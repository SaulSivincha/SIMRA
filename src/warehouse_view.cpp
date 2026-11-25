#include "simra/warehouse_view.hpp"
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QMouseEvent>
#include <QPainterPath>
#include <QResizeEvent>
#include <QTimer>
#include <algorithm>

namespace simra {
WarehouseView::WarehouseView(QWidget *parent)
    : QGraphicsView(parent), scene_(new QGraphicsScene(this)),
      animationTimer_(new QTimer(this)), milestoneTimer_(new QTimer(this)) {
  setScene(scene_);
  setRenderHint(QPainter::Antialiasing);
  setBackgroundBrush(QColor("#172235"));
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
    pausedAt_.reset();
    if (!motions_.empty())
      animationTimer_->start();
  }
}
void WarehouseView::rebuildMap(const Snapshot &snapshot) {
  animationTimer_->stop();
  milestoneTimer_->stop();
  motions_.clear();
  robotVisuals_.clear();
  shelfBadges_.clear();
  milestoneModal_ = nullptr;
  milestoneRobot_.clear();
  pendingMilestone_.reset();
  scene_->clear();
  const auto &config = snapshot.scenario;
  width_ = config.width;
  height_ = config.height;
  scene_->setSceneRect(0, 0, width_ * Cell, height_ * Cell);
  for (int y = 0; y < height_; ++y)
    for (int x = 0; x < width_; ++x) {
      const auto px = x * Cell, py = y * Cell;
      scene_->addRect(px, py, Cell, Cell, QPen(QColor("#CBD5E1"), 1),
                      QBrush(QColor("#F8FAFC")));
    }
  for (const auto obstacle : config.obstacles) {
    const int x = obstacle.x * Cell, y = obstacle.y * Cell;
    scene_->addRect(x + 4, y + 4, Cell - 8, Cell - 8,
                    QPen(QColor("#334155"), 1), QBrush(QColor("#526174")));
    for (int i = 0; i < 3; ++i)
      scene_->addLine(x + 10 + i * 10, y + 10, x + 10, y + 10 + i * 10,
                      QPen(QColor("#94A3B8"), 1));
  }
  for (const auto &shelf : config.shelves) {
    const int x = shelf.position.x * Cell, y = shelf.position.y * Cell;
    scene_->addRect(x + 5, y + 5, Cell - 10, Cell - 10,
                    QPen(QColor("#6B4E32"), 2), QBrush(QColor("#B08A62")));
    scene_->addLine(x + 11, y + 19, x + Cell - 11, y + 19,
                    QPen(QColor("#6B4E32"), 2));
    scene_->addLine(x + 11, y + 32, x + Cell - 11, y + 32,
                    QPen(QColor("#6B4E32"), 2));
    auto *badge = scene_->addText({});
    shelfBadges_.emplace(shelf.id, badge);
    badge->setDefaultTextColor(QColor("#FFF7ED"));
    badge->setScale(0.65);
    badge->setPos(x + Cell - 22, y + 4);
  }
  for (const auto &station : config.stations) {
    const int x = station.position.x * Cell, y = station.position.y * Cell;
    QColor color;
    QString glyph;
    if (station.kind == Terrain::Delivery) {
      color = QColor("#16A34A");
      glyph = "E";
    } else if (station.kind == Terrain::Reception) {
      color = QColor("#0891B2");
      glyph = "R";
    } else {
      color = QColor("#D97706");
      glyph = "C";
    }
    QPainterPath path;
    path.addRoundedRect(x + 4, y + 4, Cell - 8, Cell - 8, 6, 6);
    auto *item =
        scene_->addPath(path, QPen(color.darker(130), 2), QBrush(color));
    item->setToolTip(QString::fromStdString(station.id));
    auto *text = scene_->addText(glyph);
    QFont font = text->font();
    font.setBold(true);
    font.setPointSize(15);
    text->setFont(font);
    text->setDefaultTextColor(Qt::white);
    text->setPos(x + 16, y + 7);
  }
  if (selectedCell_) {
    const auto x = selectedCell_->x * Cell, y = selectedCell_->y * Cell;
    scene_->addRect(x + 1, y + 1, Cell - 2, Cell - 2,
                    QPen(QColor("#2563EB"), 3), Qt::NoBrush);
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
      bodyPath.addRoundedRect(9, 9, Cell - 18, Cell - 19, 8, 8);
      body->setPath(bodyPath);
      auto *label = new QGraphicsTextItem(group);
      auto font = label->font();
      font.setBold(true);
      font.setPointSize(8);
      label->setFont(font);
      label->setDefaultTextColor(Qt::white);
      label->setPos(12, 13);
      auto *cargo = new QGraphicsRectItem(Cell - 17, 3, 12, 12, group);
      auto *battery = new QGraphicsRectItem(9, Cell - 8, 0, 4, group);
      auto *route = scene_->addPath(QPainterPath{});
      route->setData(0, QString::fromStdString(robot.id));
      it->second = {group, focus, body, label, cargo, battery, target, route, {}};
      group->setPos(target);
    }
    auto &visual = it->second;
    if (visual.target != target) {
      const auto current = displayedPosition(robot.id, now);
      visual.target = target;
      motions_[robot.id] = {current, target, now,
                            robot.kind == RobotKind::Light ? 760 : 1160};
      if (!pausedAt_ && !animationTimer_->isActive())
        animationTimer_->start();
    }
    QColor color = robot.kind == RobotKind::Light ? QColor("#2563EB")
                                                   : QColor("#9333EA");
    if (robot.status == RobotStatus::Waiting)
      color = QColor("#D97706");
    if (robot.status == RobotStatus::Blocked)
      color = QColor("#EF4444");
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
    routeColor.setAlpha(focused ? 255 : 175);
    visual.route->setPen(QPen(routeColor, focused ? 4 : 3, Qt::DashLine,
                             Qt::RoundCap, Qt::RoundJoin));
    visual.route->setZValue(focused ? 5 : 4);
    visual.focus->setVisible(focused);
    updateRoute(visual);
    visual.label->setPlainText(QString::fromStdString(robot.id));
    visual.cargo->setPen(QPen(QColor("#92400E")));
    visual.cargo->setBrush(QBrush(QColor("#FBBF24")));
    visual.cargo->setVisible(!robot.carrying.empty());
    const double battery = robot.maxBattery > 0
                               ? double(robot.battery) / robot.maxBattery
                               : 0.0;
    visual.battery->setRect(9, Cell - 8, (Cell - 18) * battery, 4);
    visual.battery->setPen(Qt::NoPen);
    visual.battery->setBrush(
        QBrush(battery < .25 ? QColor("#EF4444") : QColor("#22C55E")));
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
  if (motions_.empty())
    animationTimer_->stop();
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
  text->setDefaultTextColor(QColor("#F8FAFC"));
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
  bubble->setBrush(QBrush(QColor("#0F172A")));
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
