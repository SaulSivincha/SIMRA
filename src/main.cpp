#include "simra/window.hpp"
#include <QApplication>
#include <QFile>
#include <QString>
#include <QTimer>
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  simra::MainWindow window;
  window.show();
  if (argc >= 3 && QString(argv[1]).startsWith("--screenshot-")) {
    QString mode = QString::fromLocal8Bit(argv[1]);
    QString path = QString::fromLocal8Bit(argv[argc - 1]);
    if (mode == "--screenshot-demo" || mode == "--screenshot-stats") {
      window.startDemo();
      if (mode == "--screenshot-stats")
        window.showStatistics();
    } else if (mode == "--screenshot-comparison" && argc == 4) {
      QFile file(QString::fromLocal8Bit(argv[2]));
      if (!file.open(QIODevice::ReadOnly))
        return 1;
      window.showComparison(file.readAll());
    } else if (mode != "--screenshot-home") {
      return 1;
    }
    auto delay = mode == "--screenshot-stats" ? 7000 : 2100;
    QTimer::singleShot(delay, &window, [&window, path] {
      QApplication::exit(window.grab().save(path) ? 0 : 1);
    });
  }
  return app.exec();
}
