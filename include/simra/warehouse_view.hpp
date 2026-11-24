#pragma once
#include "simra/state.hpp"
#include <QGraphicsView>
#include <functional>

class QGraphicsScene;
namespace simra {
struct RouteOverlay {
  std::string robotId;
  std::vector<Position> cells;
};
class WarehouseView final : public QGraphicsView {
  QGraphicsScene *scene_;
  std::function<void(Position, bool)> editHandler_;
  int width_{}, height_;
  static constexpr int Cell = 52;
  void fitMap();

protected:
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

public:
  explicit WarehouseView(QWidget *parent = nullptr);
  void setEditHandler(std::function<void(Position, bool)> handler) {
    editHandler_ = std::move(handler);
  }
  void render(const Snapshot &snapshot,
              const std::optional<RouteOverlay> &route = {});
};
} // namespace simra
