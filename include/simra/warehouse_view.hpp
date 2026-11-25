#pragma once
#include "simra/state.hpp"
#include <QColor>
#include <QGraphicsView>
#include <QPointF>
#include <QSizeF>
#include <QString>
#include <chrono>
#include <functional>
#include <map>

class QGraphicsEllipseItem;
class QGraphicsItemGroup;
class QGraphicsLineItem;
class QGraphicsPathItem;
class QGraphicsPixmapItem;
class QGraphicsRectItem;
class QGraphicsScene;
class QGraphicsTextItem;
class QTimer;
namespace simra {
class WarehouseView final : public QGraphicsView {
  struct RobotVisual {
    QGraphicsItemGroup *group{};
    QGraphicsEllipseItem *focus{};
    QGraphicsPathItem *body{};
    QGraphicsTextItem *label{};
    QGraphicsPixmapItem *cargo{};
    QGraphicsRectItem *battery{};
    QPointF target;
    QGraphicsPathItem *route{};
    std::vector<Position> routeCells;
  };
  struct Motion {
    QPointF from, to;
    std::chrono::steady_clock::time_point started;
    int durationMs{};
  };
  struct Milestone {
    QString title, detail;
    QColor accent;
    std::string robotId;
    int priority;
  };
  QGraphicsScene *scene_;
  QTimer *animationTimer_{};
  std::function<void(Position, bool)> editHandler_;
  std::optional<Position> selectedCell_;
  std::map<std::string, RobotVisual> robotVisuals_;
  std::map<std::string, QGraphicsTextItem *> shelfBadges_;
  std::map<std::string, QGraphicsPathItem *> stationMarkers_;
  std::map<std::string, Motion> motions_;
  QGraphicsItemGroup *milestoneModal_{};
  QGraphicsEllipseItem *milestoneFocus_{};
  QGraphicsLineItem *milestonePointer_{};
  std::string milestoneRobot_;
  QSizeF milestoneSize_;
  QTimer *milestoneTimer_{};
  std::optional<Milestone> pendingMilestone_;
  std::optional<std::chrono::steady_clock::time_point> pausedAt_;
  bool mapReady_{};
  bool reduceMotion_{};
  int width_{}, height_;
  static constexpr int Cell = 52;
  void fitMap();
  void rebuildMap(const Snapshot &snapshot);
  void updateRoute(const RobotVisual &robot);
  void updateRobots(const Snapshot &snapshot, const std::string &focusedRobot);
  QPointF displayedPosition(const std::string &id,
                            std::chrono::steady_clock::time_point now) const;
  void advanceAnimations();
  void positionMilestone();
  void displayMilestone(const Milestone &milestone);

protected:
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

public:
  explicit WarehouseView(QWidget *parent = nullptr);
  void selectCell(std::optional<Position> selected) {
    selectedCell_ = selected;
  }
  void setEditHandler(std::function<void(Position, bool)> handler) {
    editHandler_ = std::move(handler);
  }
  void resetAnimation();
  void pauseAnimation(bool paused);
  void showMilestone(const QString &title, const QString &detail,
                     const QColor &accent, const std::string &robotId,
                     int priority = 1);
  void render(const Snapshot &snapshot, const std::string &focusedRobot = {});
};
} // namespace simra
