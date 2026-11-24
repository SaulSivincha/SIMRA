#include "check.hpp"
#include "simra/window.hpp"
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
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
  CHECK(!window.findChild<QTableWidget *>("ordersTable"));
  auto *currentScenario = window.findChild<QLabel *>("currentScenario");
  CHECK(currentScenario && !currentScenario->text().isEmpty());
  auto *guide = window.findChild<QLabel *>("demonstrationGuide");
  CHECK(guide && guide->text().contains("hilo"));
  auto *workers = window.findChild<QTableWidget *>("workerTable");
  CHECK(workers);
  CHECK(QTest::qWaitFor([&] {
    return workers->rowCount() == 2 && workers->item(0, 5) &&
           workers->item(1, 5) && !workers->item(0, 5)->text().isEmpty() &&
           !workers->item(1, 5)->text().isEmpty();
  }));
  CHECK(workers->item(0, 5)->text() != workers->item(1, 5)->text());
  QTest::mouseClick(pause, Qt::LeftButton);
  CHECK(pause->text() == "Reanudar");
  QTest::mouseClick(pause, Qt::LeftButton);
  CHECK(pause->text() == "Pausar");
  auto *home = button("homeScreen");
  QTest::mouseClick(home, Qt::LeftButton);
  CHECK(basicDemo->isVisible());
}
