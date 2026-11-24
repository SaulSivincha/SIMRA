#include "simra/warehouse_view.hpp"
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QMouseEvent>
#include <QPainterPath>
#include <QResizeEvent>

namespace simra {
WarehouseView::WarehouseView(QWidget *parent)
    : QGraphicsView(parent), scene_(new QGraphicsScene(this)) {
  setScene(scene_);
  setRenderHint(QPainter::Antialiasing);
  setBackgroundBrush(QColor("#172235"));
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);
  setAlignment(Qt::AlignCenter);
  setTransformationAnchor(QGraphicsView::AnchorViewCenter);
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
    auto point = mapToScene(event->pos());
    if (point.x() >= 0 && point.y() >= 0 && point.x() < width_ * Cell &&
        point.y() < height_ * Cell) {
      editHandler_({int(point.x()) / Cell, int(point.y()) / Cell},
                   event->button() == Qt::RightButton);
      return;
    }
  }
  QGraphicsView::mousePressEvent(event);
}
void WarehouseView::render(const Snapshot &snapshot,
                           const std::optional<RouteOverlay> &route) {
  const auto &config = snapshot.scenario;
  width_ = config.width;
  height_ = config.height;
  scene_->clear();
  scene_->setSceneRect(0, 0, width_ * Cell, height_ * Cell);
  for (int y = 0; y < height_; ++y)
    for (int x = 0; x < width_; ++x) {
      auto px = x * Cell, py = y * Cell;
      scene_->addRect(px, py, Cell, Cell, QPen(QColor("#CBD5E1"), 1),
                      QBrush(QColor("#F8FAFC")));
    }
  for (auto obstacle : config.obstacles) {
    int x = obstacle.x * Cell, y = obstacle.y * Cell;
    scene_->addRect(x + 4, y + 4, Cell - 8, Cell - 8,
                    QPen(QColor("#334155"), 1), QBrush(QColor("#526174")));
    for (int i = 0; i < 3; ++i)
      scene_->addLine(x + 10 + i * 10, y + 10, x + 10, y + 10 + i * 10,
                      QPen(QColor("#94A3B8"), 1));
  }
  for (auto &shelf : config.shelves) {
    int x = shelf.position.x * Cell, y = shelf.position.y * Cell;
    scene_->addRect(x + 5, y + 5, Cell - 10, Cell - 10,
                    QPen(QColor("#6B4E32"), 2), QBrush(QColor("#B08A62")));
    scene_->addLine(x + 11, y + 19, x + Cell - 11, y + 19,
                    QPen(QColor("#6B4E32"), 2));
    scene_->addLine(x + 11, y + 32, x + Cell - 11, y + 32,
                    QPen(QColor("#6B4E32"), 2));
    auto count = std::count_if(
        snapshot.cargo.begin(), snapshot.cargo.end(),
        [&](auto &cargo) { return cargo.locationId == shelf.id; });
    if (snapshot.cargo.empty())
      count = std::count_if(
          config.cargo.begin(), config.cargo.end(),
          [&](auto &cargo) { return cargo.locationId == shelf.id; });
    auto *badge = scene_->addText(QString::number(count));
    badge->setDefaultTextColor(QColor("#FFF7ED"));
    badge->setScale(0.65);
    badge->setPos(x + Cell - 22, y + 4);
  }
  for (auto &station : config.stations) {
    int x = station.position.x * Cell, y = station.position.y * Cell;
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
  if (route) {
    QColor color("#2563EB");
    for (auto &robot : snapshot.robots)
      if (robot.id == route->robotId && robot.kind == RobotKind::Heavy)
        color = QColor("#9333EA");
    QPen pen(color, 3, Qt::DashLine, Qt::RoundCap);
    Position previous{};
    bool hasPrevious = false;
    for (auto &robot : snapshot.robots)
      if (robot.id == route->robotId) {
        previous = robot.position;
        hasPrevious = true;
      }
    if (hasPrevious)
      for (auto next : route->cells) {
        scene_->addLine(previous.x * Cell + Cell / 2,
                        previous.y * Cell + Cell / 2, next.x * Cell + Cell / 2,
                        next.y * Cell + Cell / 2, pen);
        previous = next;
      }
  }
  std::vector<RobotView> robots = snapshot.robots;
  if (robots.empty())
    for (auto &robot : config.robots)
      robots.push_back({robot.id, "", "", "", robot.position, robot.kind,
                        RobotStatus::Idle, robot.battery,
                        robot.kind == RobotKind::Light ? 100 : 150});
  for (auto &robot : robots) {
    int x = robot.position.x * Cell, y = robot.position.y * Cell;
    QColor color =
        robot.kind == RobotKind::Light ? QColor("#2563EB") : QColor("#9333EA");
    if (robot.status == RobotStatus::Waiting)
      color = QColor("#D97706");
    if (robot.status == RobotStatus::Blocked)
      color = QColor("#EF4444");
    if (route && route->robotId == robot.id)
      scene_->addEllipse(x + 1, y + 1, Cell - 2, Cell - 2, QPen(color, 3),
                         Qt::NoBrush);
    QPainterPath body;
    body.addRoundedRect(x + 9, y + 9, Cell - 18, Cell - 19, 8, 8);
    auto *item =
        scene_->addPath(body, QPen(color.darker(140), 2), QBrush(color));
    item->setToolTip(QString::fromStdString(robot.id + " · " + robot.threadId));
    auto *label = scene_->addText(QString::fromStdString(robot.id));
    QFont font = label->font();
    font.setBold(true);
    font.setPointSize(8);
    label->setFont(font);
    label->setDefaultTextColor(Qt::white);
    label->setPos(x + 12, y + 13);
    if (!robot.carrying.empty())
      scene_->addRect(x + Cell - 17, y + 3, 12, 12, QPen(QColor("#92400E")),
                      QBrush(QColor("#FBBF24")));
    double fraction =
        robot.maxBattery > 0 ? double(robot.battery) / robot.maxBattery : 0.0;
    scene_->addRect(
        x + 9, y + Cell - 8, (Cell - 18) * fraction, 4, Qt::NoPen,
        QBrush(fraction < .25 ? QColor("#EF4444") : QColor("#22C55E")));
  }
  fitMap();
}
} // namespace simra
