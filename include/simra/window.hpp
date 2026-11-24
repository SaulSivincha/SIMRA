#pragma once
#include "simra/comparison_panel.hpp"
#include "simra/core.hpp"
#include "simra/robot_monitor.hpp"
#include "simra/scenario_editor.hpp"
#include "simra/statistics_panel.hpp"
#include "simra/warehouse_view.hpp"
#include <QMainWindow>
class QComboBox;
class QTableWidget;
class QTextEdit;
class QLabel;
class QPushButton;
class QTimer;
class QTabWidget;
class QStackedWidget;
namespace simra {
class ScenarioHub;
class MainWindow final : public QMainWindow {
  Scenario scenario_;
  std::unique_ptr<SimulationEngine> engine_;
  WarehouseView *view_{};
  QComboBox *scenarios_{};
  QComboBox *tool_{};
  QTableWidget *robots_{};
  QTableWidget *orders_{};
  QTableWidget *inventory_{};
  RobotMonitor *monitor_{};
  QTextEdit *events_{};
  ComparisonPanel *comparison_{};
  StatisticsPanel *metrics_{};
  QLabel *status_{};
  QLabel *summary_{};
  QLabel *concept_{};
  QLabel *concurrencySummary_{}, *resources_{};
  QLabel *cellInspector_{};
  QTimer *timer_{};
  QTabWidget *leftTabs_{};
  QTabWidget *rightTabs_{};
  QWidget *toolsPage_{}, *inspectorPage_{}, *activity_{};
  QStackedWidget *pages_{};
  QWidget *workspace_{};
  ScenarioHub *hub_{};
  QPushButton *start_{}, *pause_{}, *stop_{}, *editor_{}, *compare_{};
  bool editing_{};
  int nextOrder_{1};
  std::string selectedRobot_;
  void render(const Snapshot &);
  void refresh();
  void loadPreset();
  void openScenario();
  void setEditing(bool);
  void startSimulation();
  void stopSimulation();
  void addOrder();
  void addSampleOrders();
  void editCell(Position, bool);
  void inspectCell(Position);
  void showCell(Position);

public:
  explicit MainWindow(QWidget *parent = nullptr);
  void startDemo();
  void showStatistics();
  void showComparison(const QByteArray &json);
};
} // namespace simra
