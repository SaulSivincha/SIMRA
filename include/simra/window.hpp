#pragma once
#include "simra/core.hpp"
#include "simra/robot_monitor.hpp"
#include "simra/statistics_panel.hpp"
#include "simra/warehouse_view.hpp"
#include <QMainWindow>
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
  RobotMonitor *monitor_{};
  StatisticsPanel *metrics_{};
  QLabel *status_{}, *scenarioName_{};
  QLabel *concept_{};
  QTimer *timer_{};
  QTabWidget *rightTabs_{};
  QStackedWidget *pages_{};
  QWidget *workspace_{};
  ScenarioHub *hub_{};
  QPushButton *start_{}, *pause_{};
  int scenarioIndex_{};
  int nextOrder_{1};
  std::string selectedRobot_;
  void render(const Snapshot &);
  void refresh();
  void loadPreset();
  void startSimulation();
  void stopSimulation();
  void addSampleOrders();

public:
  explicit MainWindow(QWidget *parent = nullptr);
  void startDemo();
  void showStatistics();
};
} // namespace simra
