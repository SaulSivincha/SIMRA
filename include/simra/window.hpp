#pragma once
#include "simra/core.hpp"
#include <QMainWindow>
class QGraphicsScene;
class QGraphicsView;
class QComboBox;
class QTableWidget;
class QTextEdit;
class QLabel;
class QPushButton;
class QTimer;
namespace simra {
class MainWindow final : public QMainWindow {
  Scenario scenario_;
  std::unique_ptr<SimulationEngine> engine_;
  QGraphicsScene *scene_{};
  QGraphicsView *view_{};
  QComboBox *scenarios_{};
  QComboBox *tool_{};
  QTableWidget *robots_{};
  QTableWidget *orders_{};
  QTextEdit *events_{};
  QTextEdit *comparison_{};
  QLabel *metrics_{};
  QLabel *status_{};
  QTimer *timer_{};
  QPushButton *start_{}, *pause_{}, *stop_{}, *editor_{};
  bool editing_{}, automatic_{};
  uint64_t lastEvent_{};
  int nextOrder_{1};
  void render(const Snapshot &);
  void refresh();
  void loadPreset();
  void startSimulation();
  void stopSimulation();
  void addOrder();
  void editCell(Position, QObject *);
  bool eventFilter(QObject *object, QEvent *event) override;

public:
  explicit MainWindow(QWidget *parent = nullptr);
  void startDemo();
};
} // namespace simra
