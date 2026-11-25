#include "simra/window.hpp"
#include "simra/demo_program.hpp"
#include "simra/scenario_hub.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace simra {
static QString scenarioId(int index) {
  switch (index) {
  case 1:
    return "mixed";
  case 2:
    return "congestion";
  case 3:
    return "charging";
  default:
    return "basic";
  }
}
static QString scenarioTitle(int index) {
  switch (index) {
  case 1:
    return "Herencia y polimorfismo";
  case 2:
    return "Mutex: pasillo compartido";
  case 3:
    return "Sincronización: recarga compartida";
  default:
    return "Hilos y pedidos concurrentes";
  }
}
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("SIMRA · Simulador Multihilo de Robots de Almacén");
  resize(1440, 900);
  setStyleSheet(
      "QMainWindow,QWidget{background:#111827;color:#F1F5F9;font-size:13px} "
      "QPushButton,QComboBox{background:#1E293B;border:1px solid "
      "#334155;border-radius:6px;padding:8px} "
      "QPushButton:hover{background:#2563EB} "
      "QPushButton:disabled{color:#728299;background:#172031} "
      "QPushButton#startSimulation{background:#2563EB;font-weight:700} "
      "QPushButton#startSimulation:disabled{background:#14264B} "
      "QTableWidget,QTextEdit{background:#1E293B;gridline-color:#334155;border:"
      "1px solid #334155} "
      "QHeaderView::section{background:#334155;color:white;padding:6px;border:"
      "0}");
  pages_ = new QStackedWidget;
  setCentralWidget(pages_);
  workspace_ = new QWidget;
  pages_->addWidget(workspace_);
  hub_ = new ScenarioHub(
      [this](int index) {
        scenarioIndex_ = index;
        loadPreset();
        pages_->setCurrentWidget(workspace_);
      });
  pages_->addWidget(hub_);
  auto *root = new QVBoxLayout(workspace_);
  auto *toolbar = new QHBoxLayout;
  root->addLayout(toolbar);
  auto *title = new QLabel("SIMRA");
  title->setStyleSheet("font-size:24px;font-weight:bold;color:#60A5FA");
  toolbar->addWidget(title);
  scenarioName_ = new QLabel;
  scenarioName_->setObjectName("currentScenario");
  scenarioName_->setStyleSheet("color:#CBD5E1;font-weight:600;padding:8px;");
  toolbar->addWidget(scenarioName_);
  toolbar->addStretch();
  auto *home = new QPushButton("Inicio", workspace_);
  home->setObjectName("homeScreen");
  connect(home, &QPushButton::clicked, this, [this] {
    if (engine_ && !engine_->state().stopped())
      stopSimulation();
    pages_->setCurrentWidget(hub_);
  });
  toolbar->addWidget(home);
  auto *buttonRow = new QHBoxLayout;
  root->addLayout(buttonRow);
  start_ = new QPushButton("Iniciar demostración", workspace_);
  buttonRow->addWidget(start_);
  connect(start_, &QPushButton::clicked, this, &MainWindow::startSimulation);
  start_->setObjectName("startSimulation");
  pause_ = new QPushButton("Pausar", workspace_);
  buttonRow->addWidget(pause_);
  connect(pause_, &QPushButton::clicked, this, [this] {
    if (!engine_)
      return;
    bool paused = engine_->state().snapshot().paused;
    engine_->pause(!paused);
    view_->pauseAnimation(!paused);
    pause_->setText(paused ? "Pausar" : "Reanudar");
    status_->setText(paused ? "Simulación en ejecución" : "Simulación pausada");
  });
  pause_->setObjectName("pauseSimulation");
  auto *split = new QSplitter(Qt::Horizontal);
  root->addWidget(split, 1);
  view_ = new WarehouseView;
  view_->setObjectName("warehouseMap");
  split->addWidget(view_);
  rightTabs_ = new QTabWidget;
  auto *right = rightTabs_;
  split->addWidget(rightTabs_);
  monitor_ = new RobotMonitor;
  monitor_->onSelected(
      [this](std::string id) { selectedRobot_ = std::move(id); });
  right->addTab(monitor_, "Robots");
  metrics_ = new StatisticsPanel;
  metrics_->setObjectName("technicalStats");
  right->addTab(metrics_, "Estadísticas");
  split->setSizes({980, 425});
  status_ = new QLabel("Listo");
  root->addWidget(status_);
  timer_ = new QTimer(this);
  timer_->setInterval(100);
  connect(timer_, &QTimer::timeout, this, [this] { refresh(); });
  timer_->start();
  loadPreset();
  hub_->preview(scenario_);
  pages_->setCurrentWidget(hub_);
}
void MainWindow::loadPreset() {
  try {
    auto path = QString(SIMRA_SOURCE_DIR) + "/scenarios/" +
                scenarioId(scenarioIndex_) + ".json";
    auto candidate = loadScenario(path.toStdString());
    if (engine_)
      stopSimulation();
    engine_.reset();
    selectedRobot_.clear();
    scenario_ = std::move(candidate);
    render(Snapshot{scenario_});
    scenarioName_->setText("Escenario · " + scenarioTitle(scenarioIndex_));
    status_->setText("Escenario preparado");
    start_->setEnabled(true);
    start_->setText("Iniciar demostración");
    pause_->setEnabled(false);
  } catch (const std::exception &e) {
    QMessageBox::warning(this, "Escenario inválido", e.what());
  }
}
void MainWindow::startSimulation() {
  if (engine_) {
    stopSimulation();
    engine_.reset();
  }
  try {
    scenario_.validate();
    view_->resetAnimation();
    engine_ = std::make_unique<SimulationEngine>(scenario_);
    loadDemoOrders(engine_->state(), scenario_.name);
    engine_->state().setSpeedPercent(demoSpeedPercent);
    engine_->start(false);
    start_->setEnabled(false);
    start_->setText("Demostración en curso");
    pause_->setEnabled(true);
    status_->setText("Simulación en ejecución");
  } catch (const std::exception &e) {
    engine_.reset();
    QMessageBox::warning(this, "No se pudo iniciar", e.what());
  }
}
void MainWindow::stopSimulation() {
  if (!engine_)
    return;
  status_->setText("Deteniendo hilos...");
  engine_->stop();
  refresh();
  pause_->setEnabled(false);
  start_->setEnabled(true);
  pause_->setText("Pausar");
  status_->setText(
      "Demostración finalizada; los hilos terminaron de forma segura");
}
void MainWindow::startDemo() {
  pages_->setCurrentWidget(workspace_);
  startSimulation();
}
void MainWindow::showStatistics() { rightTabs_->setCurrentWidget(metrics_); }
void MainWindow::refresh() {
  if (!engine_)
    return;
  auto s = engine_->state().snapshot();
  render(s);
  while (auto event = SimulationLogger::instance().tryNextEvent())
    showMilestone(*event);
}
void MainWindow::render(const Snapshot &s) {
  std::optional<RouteOverlay> overlay;
  if (engine_ && !selectedRobot_.empty()) {
    auto robot = std::find_if(s.robots.begin(), s.robots.end(),
                              [&](auto &r) { return r.id == selectedRobot_; });
    auto order = std::find_if(s.orders.begin(), s.orders.end(), [&](auto &o) {
      return o.robotId == selectedRobot_ && o.status != OrderStatus::Completed;
    });
    if (robot != s.robots.end() && order != s.orders.end()) {
      if (auto plan = engine_->state().taskPlan(selectedRobot_)) {
        const auto goal =
            robot->carrying.empty() ? plan->pickup : plan->destination;
        auto route =
            engine_->state().route(robot->position, goal, true, selectedRobot_);
        if (route)
          overlay = RouteOverlay{selectedRobot_, std::move(*route)};
      }
    }
  }
  if (engine_ && !overlay) {
    for (const auto &robot : s.robots) {
      if (auto plan = engine_->state().taskPlan(robot.id)) {
        auto target = robot.carrying.empty() ? plan->pickup : plan->destination;
        if (auto route = engine_->state().route(robot.position, target, true,
                                                robot.id)) {
          overlay = RouteOverlay{robot.id, *route};
          break;
        }
      }
    }
  }
  view_->render(s, overlay);
  auto robotSnapshot = s;
  if (robotSnapshot.robots.empty())
    for (auto &robot : s.scenario.robots)
      robotSnapshot.robots.push_back(
          {robot.id, "", "", "", robot.position, robot.kind, RobotStatus::Idle,
           robot.battery, robot.kind == RobotKind::Light ? 100 : 150});
  monitor_->render(robotSnapshot);
  metrics_->render(s);
  const bool completed = !s.orders.empty() &&
                         std::all_of(s.orders.begin(), s.orders.end(),
                                     [](const auto &order) {
                                       return order.status == OrderStatus::Completed;
                                     });
  if (completed && s.stationReservations.empty() && !start_->isEnabled()) {
    start_->setEnabled(true);
    start_->setText("Reiniciar demostración");
    pause_->setEnabled(false);
    pause_->setText("Pausar");
    status_->setText("Demostración finalizada. Puedes reiniciarla.");
  }
}
void MainWindow::showMilestone(const Event &event) {
  const auto actor = event.robotId.empty()
                         ? QString("El sistema")
                         : QString::fromStdString(event.robotId);
  const auto detail = actor + " · " + QString::fromStdString(event.text);
  auto show = [this, &detail, &event](const QString &title,
                                      const QColor &accent, int priority = 1) {
    view_->showMilestone(title, detail, accent, event.robotId, priority);
  };
  switch (event.kind) {
  case EventKind::ThreadStarted:
    if (scenarioIndex_ == 0)
      show("HILO CREADO", QColor("#38BDF8"));
    break;
  case EventKind::OrderAssigned:
    show(scenarioIndex_ == 1 ? "POLIMORFISMO EN ACCIÓN"
                              : "TRABAJO ASIGNADO",
         QColor("#60A5FA"), scenarioIndex_ == 1 ? 3 : 1);
    break;
  case EventKind::ResourceAcquired:
    show("RECURSO ADQUIRIDO", QColor("#2DD4BF"), 2);
    break;
  case EventKind::ResourceReleased:
    show("RECURSO LIBERADO", QColor("#34D399"), 2);
    break;
  case EventKind::MutexWait:
    show("ESPERA POR UN RECURSO", QColor("#F59E0B"), 2);
    break;
  case EventKind::WaitCycleDetected:
    show("CICLO DE ESPERA DETECTADO", QColor("#FB7185"), 4);
    break;
  case EventKind::WaitCycleRecovered:
    show("CICLO DE ESPERA RECUPERADO", QColor("#FBBF24"), 3);
    break;
  case EventKind::ChargingStarted:
    show("USO EXCLUSIVO DEL CARGADOR", QColor("#2DD4BF"), 3);
    break;
  case EventKind::ChargingCompleted:
    show("RECARGA COMPLETADA", QColor("#34D399"), 2);
    break;
  case EventKind::Generic:
    return;
  }
}
} // namespace simra
