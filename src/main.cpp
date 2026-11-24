#include "simra/window.hpp"
#include <QApplication>
#include <QString>
#include <QTimer>
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  simra::MainWindow window;
  window.show();
  if (argc == 3 && QString(argv[1]) == "--screenshot-demo") {
    window.startDemo();
    QString path = QString::fromLocal8Bit(argv[2]);
    QTimer::singleShot(1500, &window, [&window, path] {
      window.grab().save(path);
      QApplication::quit();
    });
  }
  return app.exec();
}
