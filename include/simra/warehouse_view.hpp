#pragma once
#include "simra/state.hpp"
#include <QGraphicsView>
#include <QPointF>
#include <chrono>
#include <functional>
#include <map>

class QGraphicsEllipseItem;
class QGraphicsItem;
class QGraphicsItemGroup;
class QGraphicsPathItem;
class QGraphicsRectItem;
class QGraphicsScene;
class QGraphicsTextItem;
class QTimer;
namespace simra {
struct RouteOverlay {
  std::string robotId;
  std::vector<Position> cells;
};
class WarehouseView final : public QGraphicsView {
  struct RobotVisual {
    QGraphicsItemGroup *group{};
    QGraphicsEllipseItem *focus{};
    QGraphicsPathItem *body{};
    QGraphicsTextItem *label{};
    QGraphicsRectItem *cargo{}, *battery{};
  };
  struct Motion {
    QPointF from, to;
    std::chrono::steady_clock::time_point started;
    int durationMs{};
  };
  QGraphicsScene *scene_;
  QTimer *animationTimer_{};
  std::function<void(Position, bool)> editHandler_;
  std::optional<Position> selectedCell_;
  std::map<std::string, RobotVisual> robotVisuals_;
  std::map<std::string, Motion> motions_;
  std::vector<QGraphicsItem *> routeItems_;
  bool mapReady_{};
  int width_{}, height_;
  static constexpr int Cell = 52;
  void fitMap();
  void rebuildMap(const Snapshot &snapshot);
  void updateRoute(const Snapshot &snapshot,
                   const std::optional<RouteOverlay> &route);
  void updateRobots(const Snapshot &snapshot,
                    const std::optional<RouteOverlay> &route);
  QPointF displayedPosition(const std::string &id,
                            std::chrono::steady_clock::time_point now) const;
  void advanceAnimations();

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
  void render(const Snapshot &snapshot,
              const std::optional<RouteOverlay> &route = {});
};
} // namespace simra
