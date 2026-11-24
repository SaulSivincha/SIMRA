#include "check.hpp"
#include "simra/window.hpp"
#include <QApplication>
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
  auto *openEditor = button("openEditor");
  CHECK(openEditor->isVisible());
  QTest::mouseClick(openEditor, Qt::LeftButton);
  auto *edit = button("editScenario");
  CHECK(edit->isVisible() && edit->text() == "Cerrar editor");
  auto *start = button("startSimulation");
  auto *pause = button("pauseSimulation");
  auto *stop = button("stopSimulation");
  CHECK(start->isEnabled() && !pause->isEnabled() && !stop->isEnabled());
  QTest::mouseClick(start, Qt::LeftButton);
  CHECK(!start->isEnabled() && !edit->isEnabled());
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
  QTest::mouseClick(stop, Qt::LeftButton);
  CHECK(start->isEnabled() && edit->isEnabled() && !stop->isEnabled());
  CHECK(workers->item(0, 1)->text() == "Detenido");
}
