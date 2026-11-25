#include "check.hpp"
#include "simra/window.hpp"
#include <QApplication>
#include <QGraphicsItemGroup>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTest>
#include <cmath>

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  simra::MainWindow window;
  window.resize(1280, 720);
  window.show();
  auto button = [&](const char *name) {
    auto *result = window.findChild<QPushButton *>(name);
    CHECK(result);
    return result;
  };
  auto *basicDemo = button("preset0");
  CHECK(basicDemo->isVisible());
  CHECK(basicDemo->text() == "Hilos y pedidos concurrentes");
  auto *basicConcept =
      window.findChild<QLabel *>("scenarioConcept0");
  auto *chargingConcept =
      window.findChild<QLabel *>("scenarioConcept3");
  CHECK(basicConcept && basicConcept->text().contains("Cola segura"));
  CHECK(chargingConcept &&
        chargingConcept->text().contains("Reserva exclusiva"));
  QTest::mouseClick(basicDemo, Qt::LeftButton);
  auto *map = window.findChild<QGraphicsView *>("warehouseMap");
  CHECK(map && map->isVisible());
  auto *start = button("startSimulation");
  auto *pause = button("pauseSimulation");
  CHECK(start->isEnabled() && !pause->isEnabled());
  for (auto *control : window.findChildren<QPushButton *>()) {
    CHECK(control->text() != "Cargar escenario");
    CHECK(control->text() != "Herramientas");
    CHECK(control->text() != "Editar mapa");
    CHECK(control->text() != "Detener");
  }
  QTest::mouseClick(start, Qt::LeftButton);
  CHECK(!start->isEnabled());
  auto *currentScenario = window.findChild<QLabel *>("currentScenario");
  CHECK(currentScenario && !currentScenario->text().isEmpty());
  CHECK(!window.findChild<QLabel *>("demonstrationGuide"));
  CHECK(!window.findChild<QLabel *>("conceptMoment"));
  auto *panels = window.findChild<QTabWidget *>();
  CHECK(panels && panels->count() == 2);
  CHECK(panels->tabText(0) == "Robots");
  CHECK(panels->tabText(1) == "Estadísticas");
  CHECK(window.findChild<QWidget *>("technicalStats"));
  CHECK(QTest::qWaitFor([&] {
    int routes = 0;
    for (const auto *item : map->scene()->items())
      if (auto *line = qgraphicsitem_cast<const QGraphicsPathItem *>(item);
          line && line->pen().style() == Qt::DashLine && !line->path().isEmpty())
        ++routes;
    return routes == 2;
  }, 4000));
  QGraphicsItem *movingRobot = nullptr;
  CHECK(QTest::qWaitFor([&] {
    for (auto *item : map->scene()->items()) {
      if (item->type() != QGraphicsItemGroup::Type || item->zValue() != 10)
        continue;
      const auto position = item->pos();
      if (position.x() != std::floor(position.x()) ||
          position.y() != std::floor(position.y())) {
        movingRobot = item;
        return true;
      }
    }
    return false;
  }, 4000));
  const auto robotId = movingRobot->toolTip().section(" · ", 0, 0);
  bool anchored = false;
  for (const auto *item : map->scene()->items())
    if (auto *line = qgraphicsitem_cast<const QGraphicsPathItem *>(item);
        line && line->data(0).toString() == robotId && !line->path().isEmpty()) {
      const auto startPoint = line->path().elementAt(0);
      anchored = QPointF(startPoint.x, startPoint.y) ==
                 movingRobot->pos() + QPointF(26, 26);
    }
  CHECK(anchored);
  QTest::mouseClick(pause, Qt::LeftButton);
  CHECK(pause->text() == "Reanudar");
  const auto pausedPosition = movingRobot->pos();
  QTest::qWait(150);
  CHECK(movingRobot->pos() == pausedPosition);
  QTest::mouseClick(pause, Qt::LeftButton);
  CHECK(pause->text() == "Pausar");
  CHECK(QTest::qWaitFor(
      [&] { return start->isEnabled() && start->text() == "Reiniciar demostración"; },
      55000));
  QTest::mouseClick(start, Qt::LeftButton);
  CHECK(!start->isEnabled());
  auto *home = button("homeScreen");
  QTest::mouseClick(home, Qt::LeftButton);
  CHECK(basicDemo->isVisible());
}
