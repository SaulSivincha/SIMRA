#include "simra/window.hpp"
#include "simra/demo_program.hpp"
#include "simra/scenario_hub.hpp"
#include "visual_system.hpp"
#include <QFrame>
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
static Scenario readPreset(int index) {
  const auto path = QString(SIMRA_SOURCE_DIR) + "/scenarios/" +
                    visual::scenarios.at(index).id + ".json";
  return loadScenario(path.toStdString());
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("SIMRA · Robots de almacén");
  resize(1440, 900);
  visual::applyTheme(*this);
  setWindowIcon(visual::icon("brand", QColor(visual::light)));
  pages_ = new QStackedWidget;
  setCentralWidget(pages_);
  workspace_ = new QWidget;
  pages_->addWidget(workspace_);
  hub_ = new ScenarioHub(
      [this](int index) {
        scenarioIndex_ = index;
        if (!loadPreset())
          return;
        pages_->setCurrentWidget(workspace_);
        start_->setFocus();
      }, readPreset);
  pages_->addWidget(hub_);
  auto *root = new QVBoxLayout(workspace_);
  root->setContentsMargins(22, 16, 22, 16);
  root->setSpacing(12);
  auto *toolbar = new QHBoxLayout;
  root->addLayout(toolbar);
  auto *home = new QPushButton(visual::icon("home"), "Escenarios", workspace_);
  home->setObjectName("homeScreen");
  connect(home, &QPushButton::clicked, this, [this] {
    if (engine_ && !engine_->state().stopped())
      stopSimulation();
    pages_->setCurrentWidget(hub_);
    if (auto *preset = hub_->findChild<QPushButton *>(QString("preset%1").arg(scenarioIndex_)))
      preset->setFocus();
  });
  toolbar->addWidget(home);
  toolbar->addSpacing(7);
  toolbar->addWidget(visual::label("SIMRA", "wordmark"));
  toolbar->addStretch();
  status_ = visual::label("Listo", "muted");
  toolbar->addWidget(status_);
  auto *buttonRow = new QHBoxLayout;
  root->addLayout(buttonRow);
  auto *heading = new QVBoxLayout;
  heading->setSpacing(2);
  scenarioName_ = visual::label({}, "title");
  scenarioName_->setWordWrap(true);
  scenarioName_->setObjectName("currentScenario");
  scenarioObservation_ = visual::label({}, "caption");
  scenarioObservation_->setWordWrap(true);
  heading->addWidget(scenarioName_);
  heading->addWidget(scenarioObservation_);
  buttonRow->addLayout(heading, 1);
  start_ = new QPushButton(visual::icon("play", Qt::white), "Iniciar demostración", workspace_);
  start_->setProperty("variant", "primary");
  buttonRow->addWidget(start_);
  connect(start_, &QPushButton::clicked, this, &MainWindow::startSimulation);
  start_->setObjectName("startSimulation");
  pause_ = new QPushButton(visual::icon("pause"), "Pausar", workspace_);
  buttonRow->addWidget(pause_);
  connect(pause_, &QPushButton::clicked, this, [this] {
    if (!engine_)
      return;
    bool paused = engine_->state().snapshot().paused;
    engine_->pause(!paused);
    view_->pauseAnimation(!paused);
    pause_->setText(paused ? "Pausar" : "Reanudar");
    pause_->setIcon(visual::icon(paused ? "pause" : "play"));
    status_->setText(paused ? "Simulación en ejecución" : "Simulación pausada");
  });
  pause_->setObjectName("pauseSimulation");
  auto *split = new QSplitter(Qt::Horizontal);
  root->addWidget(split, 1);
  view_ = new WarehouseView;
  view_->setObjectName("warehouseMap");
  view_->setAccessibleName("Plano del almacén y rutas de los robots");
  view_->setFocusPolicy(Qt::NoFocus);
  auto *mapPanel = visual::panel();
  auto *mapLayout = new QVBoxLayout(mapPanel);
  mapLayout->setContentsMargins(12, 10, 12, 10);
  mapLayout->addWidget(visual::label("Plano del almacén", "section"));
  actionStatus_ = visual::label({}, "muted");
  actionStatus_->setWordWrap(true);
  mapLayout->addWidget(actionStatus_);
  mapLayout->addWidget(view_, 1);
  mapLayout->addLayout(visual::mapLegend(true));
  split->addWidget(mapPanel);
  rightTabs_ = new QTabWidget;
  auto *right = rightTabs_;
  split->addWidget(rightTabs_);
  monitor_ = new RobotMonitor;
  monitor_->onSelected(
      [this](std::string id) { selectedRobot_ = std::move(id); });
  right->addTab(monitor_, visual::icon("robot"), "Robots");
  metrics_ = new StatisticsPanel;
  metrics_->setObjectName("technicalStats");
  right->addTab(metrics_, visual::icon("stats"), "Estadísticas");
  right->setMinimumWidth(285);
  split->setChildrenCollapsible(false);
  split->setStretchFactor(0, 1);
  split->setSizes({995, 370});
  timer_ = new QTimer(this);
  timer_->setTimerType(Qt::PreciseTimer);
  timer_->setInterval(33);
  connect(timer_, &QTimer::timeout, this, [this] { refresh(); });
  timer_->start();
  loadPreset();
  pages_->setCurrentWidget(hub_);
}
bool MainWindow::loadPreset() {
  try {
    const auto &presentation = visual::scenarios.at(scenarioIndex_);
    auto candidate = readPreset(scenarioIndex_);
    if (engine_)
      stopSimulation();
    engine_.reset();
    selectedRobot_.clear();
    scenario_ = std::move(candidate);
    render(Snapshot{scenario_});
    scenarioName_->setText(presentation.title);
    scenarioObservation_->setText(presentation.observation);
    status_->setText("Escenario preparado");
    actionStatus_->setText("Inicia la demostración para ver cómo se mueven los paquetes.");
    start_->setEnabled(true);
    start_->setText("Iniciar demostración");
    start_->setIcon(visual::icon("play", Qt::white));
    pause_->setEnabled(false);
    return true;
  } catch (const std::exception &e) {
    QMessageBox::warning(this, "Escenario inválido", e.what());
    return false;
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
    pause_->setFocus();
    status_->setText("Simulación en ejecución");
    actionStatus_->setText("Sigue las rutas punteadas para ver el próximo destino.");
  } catch (const std::exception &e) {
    engine_.reset();
    QMessageBox::warning(this, "No se pudo iniciar", e.what());
  }
}
void MainWindow::stopSimulation() {
  if (!engine_)
    return;
  status_->setText("Deteniendo hilos…");
  engine_->stop();
  refresh();
  pause_->setEnabled(false);
  start_->setEnabled(true);
  pause_->setText("Pausar");
  pause_->setIcon(visual::icon("pause"));
  status_->setText(
      "Demostración finalizada");
}
bool MainWindow::startDemo(int scenario) {
  if (scenario < 0 || scenario >= int(visual::scenarios.size()))
    return false;
  scenarioIndex_ = scenario;
  if (!loadPreset())
    return false;
  pages_->setCurrentWidget(workspace_);
  startSimulation();
  return bool(engine_);
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
  view_->render(s, selectedRobot_);
  auto robotSnapshot = s;
  if (robotSnapshot.robots.empty())
    for (auto &robot : s.scenario.robots)
      robotSnapshot.robots.push_back(
          {robot.id, "", "", "", robot.position, robot.kind, RobotStatus::Idle,
           robot.battery, robot.kind == RobotKind::Light ? 100 : 150});
  monitor_->render(robotSnapshot, selectedRobot_);
  metrics_->render(s);
  const bool completed = !s.orders.empty() &&
                         std::all_of(s.orders.begin(), s.orders.end(),
                                     [](const auto &order) {
                                       return order.status == OrderStatus::Completed;
                                     });
  if (completed && s.stationReservations.empty() && !start_->isEnabled()) {
    start_->setEnabled(true);
    start_->setText("Reiniciar demostración");
    start_->setIcon(visual::icon("restart", Qt::white));
    pause_->setEnabled(false);
    pause_->setText("Pausar");
    pause_->setIcon(visual::icon("pause"));
    status_->setText("Demostración finalizada. Puedes reiniciarla.");
  }
}
void MainWindow::showMilestone(const Event &event) {
  view_->showAction(event);
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
      show("Hilo creado", QColor(visual::light));
    break;
  case EventKind::OrderAssigned:
    show(scenarioIndex_ == 1 ? "Polimorfismo en acción" : "Trabajo asignado",
         QColor(scenarioIndex_ == 1 ? visual::heavy : visual::light),
         scenarioIndex_ == 1 ? 3 : 1);
    break;
  case EventKind::ResourceAcquired:
    show("Recurso adquirido", QColor(visual::receipt), 2);
    break;
  case EventKind::ResourceReleased:
    show("Recurso liberado", QColor(visual::delivery), 2);
    break;
  case EventKind::MutexWait:
    show("Espera por un recurso", QColor(visual::waiting), 2);
    break;
  case EventKind::WaitCycleDetected:
    show("Ciclo de espera detectado", QColor(visual::blocked), 4);
    break;
  case EventKind::WaitCycleRecovered:
    show("Ciclo de espera recuperado", QColor(visual::waiting), 3);
    break;
  case EventKind::ChargingStarted:
    actionStatus_->setText(actor + " está usando un cargador en exclusiva.");
    show("Uso exclusivo del cargador", QColor(visual::energy), 3);
    break;
  case EventKind::ChargingCompleted:
    actionStatus_->setText(actor + " completó la recarga en " +
                           QString::fromStdString(event.text) + ".");
    show("Recarga completada", QColor(visual::energy), 2);
    break;
  case EventKind::CargoPickedUp:
    actionStatus_->setText(actor + " recogió el paquete " + QString::fromStdString(event.text) + ".");
    break;
  case EventKind::CargoPlaced:
    actionStatus_->setText(actor + " dejó el paquete " + QString::fromStdString(event.text) + " en su destino.");
    break;
  case EventKind::ChargePulse:
    break;
  case EventKind::Generic:
    return;
  }
}
} // namespace simra
