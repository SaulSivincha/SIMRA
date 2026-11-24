#include "simra/window.hpp"
#include "simra/scenario_hub.hpp"
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

namespace simra {
static QString label(RobotStatus s) {
  switch (s) {
  case RobotStatus::Idle:
    return "Disponible";
  case RobotStatus::ToPickup:
    return "A recogida";
  case RobotStatus::ToDelivery:
    return "A entrega";
  case RobotStatus::ToCharge:
    return "A recarga";
  case RobotStatus::Charging:
    return "Recargando";
  case RobotStatus::Waiting:
    return "Esperando";
  case RobotStatus::Blocked:
    return "Bloqueado";
  case RobotStatus::Stopped:
    return "Detenido";
  }
  return {};
}
static QTableWidget *table(QStringList columns) {
  auto *t = new QTableWidget;
  t->setColumnCount(columns.size());
  t->setHorizontalHeaderLabels(columns);
  t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  t->verticalHeader()->hide();
  t->setEditTriggers(QAbstractItemView::NoEditTriggers);
  return t;
}
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
  connect(start_, &QPushButton::clicked, this, [this] {
    startSimulation();
    if (engine_)
      addSampleOrders();
  });
  start_->setObjectName("startSimulation");
  pause_ = new QPushButton("Pausar", workspace_);
  buttonRow->addWidget(pause_);
  connect(pause_, &QPushButton::clicked, this, [this] {
    if (!engine_)
      return;
    bool paused = engine_->state().snapshot().paused;
    engine_->pause(!paused);
    pause_->setText(paused ? "Pausar" : "Reanudar");
    status_->setText(paused ? "Simulación en ejecución" : "Simulación pausada");
  });
  pause_->setObjectName("pauseSimulation");
  concept_ = new QLabel;
  concept_->setObjectName("demonstrationGuide");
  concept_->setWordWrap(true);
  concept_->setStyleSheet(
      "background:#1E293B;color:#CBD5E1;padding:12px;border-radius:8px;");
  root->addWidget(concept_);
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
  auto *concurrencyPage = new QWidget;
  auto *concurrencyLayout = new QVBoxLayout(concurrencyPage);
  concurrencySummary_ = new QLabel;
  concurrencySummary_->setStyleSheet(
      "color:#93C5FD;font-weight:700;padding:8px;");
  concurrencyLayout->addWidget(concurrencySummary_);
  robots_ = table({"Robot", "Estado", "Batería", "Carga", "Espera", "Hilo"});
  robots_->setObjectName("workerTable");
  concurrencyLayout->addWidget(robots_, 1);
  connect(robots_, &QTableWidget::itemSelectionChanged, this, [this] {
    auto items = robots_->selectedItems();
    if (!items.empty())
      selectedRobot_ =
          robots_->item(items.front()->row(), 0)->text().toStdString();
  });
  resources_ = new QLabel;
  resources_->setWordWrap(true);
  resources_->setStyleSheet("color:#CBD5E1;padding:8px;");
  concurrencyLayout->addWidget(resources_);
  right->addTab(concurrencyPage, "Concurrencia");
  metrics_ = new StatisticsPanel;
  right->addTab(metrics_, "Estadísticas");
  comparison_ = new ComparisonPanel;
  right->addTab(comparison_, "Comparación");
  split->setSizes({980, 425});
  auto *activity = new QFrame;
  activity->setStyleSheet("QFrame{background:#1E293B;border:1px solid #334155;"
                          "border-radius:8px;} QLabel{border:0;}");
  activity->setFixedHeight(145);
  auto *activityLayout = new QHBoxLayout(activity);
  auto *eventColumn = new QVBoxLayout;
  auto *eventTitle = new QLabel("EVENTOS EN VIVO");
  eventTitle->setStyleSheet(
      "color:#94A3B8;font-weight:700;letter-spacing:1px;");
  eventColumn->addWidget(eventTitle);
  events_ = new QTextEdit;
  events_->setReadOnly(true);
  events_->document()->setMaximumBlockCount(500);
  eventColumn->addWidget(events_);
  activityLayout->addLayout(eventColumn, 3);
  summary_ = new QLabel;
  summary_->setMinimumWidth(250);
  summary_->setAlignment(Qt::AlignCenter);
  activityLayout->addWidget(summary_, 1);
  root->addWidget(activity);
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
    engine_ = std::make_unique<SimulationEngine>(scenario_);
    engine_->state().setSpeedPercent(100);
    engine_->start(false);
    start_->setEnabled(false);
    pause_->setEnabled(true);
    events_->clear();
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
  if (engine_)
    addSampleOrders();
}
void MainWindow::addSampleOrders() {
  auto snapshot = engine_->state().snapshot();
  auto cargo = snapshot.cargo;
  std::sort(cargo.begin(), cargo.end(),
            [](const auto &first, const auto &second) {
              return first.weight > second.weight;
            });
  int accepted = 0;
  for (const auto &item : cargo) {
    auto stock = std::find_if(
        snapshot.scenario.shelves.begin(), snapshot.scenario.shelves.end(),
        [&](const auto &shelf) { return shelf.id == item.locationId; });
    if (stock == snapshot.scenario.shelves.end())
      continue;
    auto station = std::find_if(
        snapshot.scenario.stations.begin(), snapshot.scenario.stations.end(),
        [](const auto &point) { return point.kind == Terrain::Delivery; });
    if (station == snapshot.scenario.stations.end())
      break;
    if (engine_->state().addOrder(DeliveryOrderCreator().create(
            "O" + std::to_string(nextOrder_++), item.id, station->id, 2))) {
      ++accepted;
      break;
    }
  }
  bool restockAccepted = false;
  for (const auto &item : cargo) {
    const auto atReception =
        std::any_of(snapshot.scenario.stations.begin(),
                    snapshot.scenario.stations.end(), [&](const auto &station) {
                      return station.id == item.locationId &&
                             station.kind == Terrain::Reception;
                    });
    if (!atReception)
      continue;
    for (const auto &shelf : snapshot.scenario.shelves) {
      const auto used =
          std::count_if(cargo.begin(), cargo.end(), [&](const auto &other) {
            return other.locationId == shelf.id;
          });
      if (used >= shelf.capacity)
        continue;
      if (engine_->state().addOrder(RestockOrderCreator().create(
              "O" + std::to_string(nextOrder_++), item.id, shelf.id, 1))) {
        ++accepted;
        restockAccepted = true;
        break;
      }
    }
    if (restockAccepted)
      break;
  }
  status_->setText(
      accepted ? QString("%1 pedidos de ejemplo admitidos").arg(accepted)
               : "No hay mercancía disponible para el ejemplo");
}
void MainWindow::showStatistics() { rightTabs_->setCurrentWidget(metrics_); }
void MainWindow::showComparison(const QByteArray &json) {
  pages_->setCurrentWidget(workspace_);
  comparison_->showResult(json);
  rightTabs_->setCurrentWidget(comparison_);
}
void MainWindow::refresh() {
  if (!engine_)
    return;
  auto s = engine_->state().snapshot();
  render(s);
  while (auto event = SimulationLogger::instance().tryNextEvent()) {
    auto &e = *event;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  e.when.time_since_epoch())
                  .count();
    events_->append(
        QDateTime::fromMSecsSinceEpoch(ms).toString("HH:mm:ss.zzz") + " " +
        QString::fromStdString(e.robotId + " [" + e.threadId + "] " + e.text));
  }
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
  const QStringList explanations = {
      "Observa dos robots trabajando a la vez. Cada uno tiene su propio hilo. "
      "Recogen mercancía junto al estante y la llevan a entrega o reposición.",
      "Observa robots ligeros y pesados. El motor usa la misma interfaz Robot "
      "y cada tipo define capacidad, consumo y velocidad mediante "
      "polimorfismo.",
      "Observa el paso por el pasillo compartido. El mutex permite ocupar una "
      "celda a un solo robot. Las esperas aparecen en ámbar.",
      "Observa la estación C. Los robots solicitan una reserva exclusiva, "
      "esperan su turno y recuperan batería antes de salir."};
  concept_->setText(explanations.value(scenarioIndex_) +
                    (engine_ ? "\nLa línea discontinua muestra una ruta BFS. "
                               "La caja amarilla indica mercancía en custodia."
                             : "\nPulsa Iniciar demostración. Los pedidos de "
                               "ejemplo se crean automáticamente."));
  view_->render(s, overlay);
  robots_->setRowCount(int(s.robots.size()));
  for (int i = 0; i < int(s.robots.size()); ++i) {
    auto &r = s.robots[i];
    QStringList values = {QString::fromStdString(r.id),
                          label(r.status),
                          QString::number(r.battery) + "/" +
                              QString::number(r.maxBattery),
                          QString::fromStdString(r.carrying),
                          QString::fromStdString(r.waitingFor),
                          QString::fromStdString(r.threadId)};
    for (int j = 0; j < values.size(); ++j) {
      auto *item = new QTableWidgetItem(values[j]);
      item->setToolTip(values[j]);
      robots_->setItem(i, j, item);
    }
  }
  int activeThreads = 0, waitingThreads = 0;
  for (auto &robot : s.robots) {
    activeThreads +=
        !robot.threadId.empty() && robot.status != RobotStatus::Stopped;
    waitingThreads += robot.status == RobotStatus::Waiting ||
                      robot.status == RobotStatus::Blocked;
  }
  concurrencySummary_->setText(
      QString("%1 hilos activos  ·  %2 en espera  ·  %3 ciclos observados")
          .arg(activeThreads)
          .arg(waitingThreads)
          .arg(s.metrics.cycles));
  QStringList reservations;
  for (auto &[station, robot] : s.stationReservations)
    reservations << QString::fromStdString(station + " → " + robot);
  resources_->setText(
      "Reservas de estación: " +
      (reservations.empty() ? QString("ninguna") : reservations.join(" · ")));
  auto robotSnapshot = s;
  if (robotSnapshot.robots.empty())
    for (auto &robot : s.scenario.robots)
      robotSnapshot.robots.push_back(
          {robot.id, "", "", "", robot.position, robot.kind, RobotStatus::Idle,
           robot.battery, robot.kind == RobotKind::Light ? 100 : 150});
  monitor_->render(robotSnapshot);
  int pending = 0;
  for (auto &o : s.orders)
    if (o.status != OrderStatus::Completed && o.status != OrderStatus::Blocked)
      ++pending;
  summary_->setText(
      QString("<span "
              "style='color:#86EFAC;font-size:24px;font-weight:700'>%1</span> "
              "completados<br><span style='color:#94A3B8'>%2 pendientes · %3 "
              "conflictos</span>")
          .arg(s.metrics.completed)
          .arg(pending)
          .arg(s.metrics.conflicts));
  metrics_->render(s);
}
} // namespace simra
