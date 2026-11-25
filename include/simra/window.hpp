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
  QLabel *scenarioObservation_{};
  QTimer *timer_{};
  QTabWidget *rightTabs_{};
  QStackedWidget *pages_{};
  QWidget *workspace_{};
  ScenarioHub *hub_{};
  QPushButton *start_{}, *pause_{};
  int scenarioIndex_{};
  std::string selectedRobot_;
  void render(const Snapshot &);
  void showMilestone(const Event &);
  void refresh();
  bool loadPreset();
  void startSimulation();
  void stopSimulation();

public:
  explicit MainWindow(QWidget *parent = nullptr);
  bool startDemo(int scenario = 0);
  void showStatistics();
};
} // namespace simra
