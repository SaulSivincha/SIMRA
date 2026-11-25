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
      animationTimer_(new QTimer(this)) {
  setScene(scene_);
  setRenderHint(QPainter::Antialiasing);
  setBackgroundBrush(QColor("#172235"));
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);
  setAlignment(Qt::AlignCenter);
  setTransformationAnchor(QGraphicsView::AnchorViewCenter);
  animationTimer_->setInterval(1000 / 30);
  connect(animationTimer_, &QTimer::timeout, this,
          [this] { advanceAnimations(); });
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
  motions_.clear();
  mapReady_ = false;
}
void WarehouseView::rebuildMap(const Snapshot &snapshot) {
  animationTimer_->stop();
  motions_.clear();
  robotVisuals_.clear();
  routeItems_.clear();
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
    auto count = std::count_if(snapshot.cargo.begin(), snapshot.cargo.end(),
                               [&](const auto &cargo) {
                                 return cargo.locationId == shelf.id;
                               });
    if (snapshot.cargo.empty())
      count = std::count_if(config.cargo.begin(), config.cargo.end(),
                            [&](const auto &cargo) {
                              return cargo.locationId == shelf.id;
                            });
    auto *badge = scene_->addText(QString::number(count));
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
void WarehouseView::updateRoute(const Snapshot &snapshot,
                                const std::optional<RouteOverlay> &route) {
  for (auto *item : routeItems_)
    delete item;
  routeItems_.clear();
  if (!route)
    return;
  QColor color("#2563EB");
  Position previous{};
  bool hasPrevious = false;
  for (const auto &robot : snapshot.robots)
    if (robot.id == route->robotId) {
      previous = robot.position;
      hasPrevious = true;
      if (robot.kind == RobotKind::Heavy)
        color = QColor("#9333EA");
      break;
    }
  if (!hasPrevious)
    return;
  const QPen pen(color, 3, Qt::DashLine, Qt::RoundCap);
  for (const auto next : route->cells) {
    routeItems_.push_back(scene_->addLine(
        previous.x * Cell + Cell / 2, previous.y * Cell + Cell / 2,
        next.x * Cell + Cell / 2, next.y * Cell + Cell / 2, pen));
    previous = next;
  }
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
  const auto eased = linear * linear * (3.0 - 2.0 * linear);
  return found->second.from + (found->second.to - found->second.from) * eased;
}
void WarehouseView::updateRobots(const Snapshot &snapshot,
                                 const std::optional<RouteOverlay> &route) {
  std::vector<RobotView> robots = snapshot.robots;
  if (robots.empty())
    for (const auto &robot : snapshot.scenario.robots)
      robots.push_back({robot.id, "", "", "", robot.position, robot.kind,
                        RobotStatus::Idle, robot.battery,
                        robot.kind == RobotKind::Light ? 100 : 150});
  const auto now = std::chrono::steady_clock::now();
  for (const auto &robot : robots) {
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
      it->second = {group, focus, body, label, cargo, battery};
      group->setPos(robot.position.x * Cell, robot.position.y * Cell);
    }
    auto &visual = it->second;
    const auto target = QPointF(robot.position.x * Cell, robot.position.y * Cell);
    const auto current = displayedPosition(robot.id, now);
    if (QLineF(current, target).length() > 0.1) {
      motions_[robot.id] = {current, target, now,
                            robot.kind == RobotKind::Light ? 260 : 420};
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
    visual.focus->setVisible(route && route->robotId == robot.id);
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
      delete it->second.group;
      motions_.erase(it->first);
      it = robotVisuals_.erase(it);
    } else
      ++it;
  }
}
void WarehouseView::advanceAnimations() {
  const auto now = std::chrono::steady_clock::now();
  bool moving = false;
  for (const auto &[id, motion] : motions_) {
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now - motion.started)
                             .count();
    robotVisuals_.at(id).group->setPos(displayedPosition(id, now));
    moving = moving || elapsed < motion.durationMs;
  }
  if (!moving)
    animationTimer_->stop();
}
void WarehouseView::render(const Snapshot &snapshot,
                           const std::optional<RouteOverlay> &route) {
  const bool runtimeSnapshot = !snapshot.robots.empty();
  const bool dimensionsChanged = width_ != snapshot.scenario.width ||
                                 height_ != snapshot.scenario.height;
  if (!mapReady_ || !runtimeSnapshot || dimensionsChanged)
    rebuildMap(snapshot);
  updateRoute(snapshot, route);
  updateRobots(snapshot, route);
}
} // namespace simra
