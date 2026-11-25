#include "check.hpp"
#include "simra/window.hpp"
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTest>

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
  QTest::mouseClick(pause, Qt::LeftButton);
  CHECK(pause->text() == "Reanudar");
  QTest::mouseClick(pause, Qt::LeftButton);
  CHECK(pause->text() == "Pausar");
  CHECK(QTest::qWaitFor(
      [&] { return start->isEnabled() && start->text() == "Reiniciar demostración"; },
      12000));
  QTest::mouseClick(start, Qt::LeftButton);
  CHECK(!start->isEnabled());
  auto *home = button("homeScreen");
  QTest::mouseClick(home, Qt::LeftButton);
  CHECK(basicDemo->isVisible());
}
