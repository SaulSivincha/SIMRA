#include "simra/window.hpp"
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
  moment_ = new QLabel;
  moment_->setObjectName("conceptMoment");
  moment_->setWordWrap(true);
  root->addWidget(moment_);
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
    nextOrder_ = 1;
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
    nextOrder_ = 1;
    view_->resetAnimation();
    engine_ = std::make_unique<SimulationEngine>(scenario_);
    engine_->state().setSpeedPercent(100);
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
  if (scenarioIndex_ == 2) {
    const auto delivery = std::find_if(
        snapshot.scenario.stations.begin(), snapshot.scenario.stations.end(),
        [](const auto &station) { return station.kind == Terrain::Delivery; });
    int accepted = 0;
    if (delivery != snapshot.scenario.stations.end())
      for (const auto &item : cargo) {
        const auto stock = std::find_if(
            snapshot.scenario.shelves.begin(), snapshot.scenario.shelves.end(),
            [&](const auto &shelf) { return shelf.id == item.locationId; });
        if (stock == snapshot.scenario.shelves.end())
          continue;
        if (engine_->state().addOrder(DeliveryOrderCreator().create(
                "O" + std::to_string(nextOrder_++), item.id, delivery->id, 2)))
          ++accepted;
        if (accepted == int(snapshot.scenario.robots.size()))
          break;
      }
    status_->setText(
        QString("%1 pedidos para evidenciar el mutex").arg(accepted));
    return;
  }
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
  updateConceptMoment(s);
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
                                       return order.status == OrderStatus::Completed ||
                                              order.status == OrderStatus::Blocked;
                                     });
  if (completed && !start_->isEnabled()) {
    start_->setEnabled(true);
    start_->setText("Reiniciar demostración");
    pause_->setEnabled(false);
    pause_->setText("Pausar");
    status_->setText("Demostración finalizada. Puedes reiniciarla.");
  }
}
void MainWindow::updateConceptMoment(const Snapshot &snapshot) {
  if (milestoneFrames_ > 0) {
    --milestoneFrames_;
    return;
  }
  const auto busy = [](const RobotView &robot) {
    return robot.status != RobotStatus::Idle && robot.status != RobotStatus::Stopped;
  };
  const auto active = std::count_if(snapshot.robots.begin(), snapshot.robots.end(), busy);
  if (scenarioIndex_ == 0) {
    if (active >= 2)
      setConceptMoment("HILOS EN EJECUCIÓN",
                "dos trabajadores avanzan de forma independiente sobre el mismo almacén.",
                "#38BDF8", "#0C2942");
    else
      setConceptMoment("RUTA BFS CALCULADA",
                "la línea discontinua representa el camino que seguirá el robot.",
                "#60A5FA", "#172554");
    return;
  }
  if (scenarioIndex_ == 1) {
    const auto light = std::find_if(snapshot.robots.begin(), snapshot.robots.end(),
                                    [&](const auto &robot) {
                                      return robot.kind == RobotKind::Light && busy(robot);
                                    });
    const auto heavy = std::find_if(snapshot.robots.begin(), snapshot.robots.end(),
                                    [&](const auto &robot) {
                                      return robot.kind == RobotKind::Heavy && busy(robot);
                                    });
    if (light != snapshot.robots.end() && heavy != snapshot.robots.end())
      setConceptMoment("POLIMORFISMO ACTIVO",
                "la misma interfaz Robot coordina un ligero y un pesado con reglas propias.",
                "#C084FC", "#2E1065");
    else
      setConceptMoment("HERENCIA DE ROBOTS",
                "LightRobot y HeavyRobot especializan capacidad, consumo y velocidad.",
                "#C084FC", "#2E1065");
    return;
  }
  if (scenarioIndex_ == 2) {
    const auto waiting = std::find_if(snapshot.robots.begin(), snapshot.robots.end(),
                                      [](const auto &robot) {
                                        return robot.status == RobotStatus::Waiting ||
                                               robot.status == RobotStatus::Blocked;
                                      });
    if (waiting != snapshot.robots.end()) {
      const auto resource = waiting->waitingFor.empty()
                                ? QString("la zona crítica")
                                : QString::fromStdString(waiting->waitingFor);
      setConceptMoment("MUTEX ACTIVADO",
                QString::fromStdString(waiting->id) + " espera " + resource +
                    "; solo un robot puede usar el recurso a la vez.",
                "#F59E0B", "#451A03");
    } else if (snapshot.metrics.conflicts > 0) {
      setConceptMoment("EXCLUSIÓN MUTUA VERIFICADA",
                "la coordinación evitó que dos robots ocuparan la misma celda.",
                "#F59E0B", "#451A03");
    } else {
      setConceptMoment("PASILLO CRÍTICO",
                "el motor vigila la ocupación exclusiva antes de cada movimiento.",
                "#FBBF24", "#422006");
    }
    return;
  }
  if (!snapshot.stationReservations.empty()) {
    const auto &[station, robot] = snapshot.stationReservations.front();
    setConceptMoment("RECURSO COMPARTIDO RESERVADO",
              QString::fromStdString(robot + " posee " + station) +
                  "; los demás hilos esperan su liberación.",
              "#2DD4BF", "#134E4A");
  } else {
    setConceptMoment("RESERVA EXCLUSIVA DE RECARGA",
              "la estación admite un único robot y coordina el acceso concurrente.",
              "#2DD4BF", "#134E4A");
  }
}
void MainWindow::setConceptMoment(const QString &title, const QString &detail,
                                  const QString &accent,
                                  const QString &background) {
  moment_->setText("<b>" + title + "</b> · " + detail);
  moment_->setStyleSheet("background:" + background + ";color:#E2E8F0;"
                         "border-left:5px solid " + accent +
                         ";padding:10px 12px;border-radius:6px;");
}
void MainWindow::showMilestone(const Event &event) {
  const auto actor = event.robotId.empty()
                         ? QString("El sistema")
                         : QString::fromStdString(event.robotId);
  const auto detail = actor + " · " + QString::fromStdString(event.text);
  switch (event.kind) {
  case EventKind::ThreadStarted:
    setConceptMoment("HILO CREADO", detail,
                     "#38BDF8", "#0C2942");
    break;
  case EventKind::OrderAssigned:
    setConceptMoment(scenarioIndex_ == 1 ? "POLIMORFISMO EN ACCIÓN"
                                          : "TRABAJO ASIGNADO",
                     detail, "#60A5FA", "#172554");
    break;
  case EventKind::ResourceAcquired:
    setConceptMoment("RECURSO ADQUIRIDO", detail,
                     "#2DD4BF", "#134E4A");
    break;
  case EventKind::ResourceReleased:
    setConceptMoment("RECURSO LIBERADO", detail,
                     "#34D399", "#064E3B");
    break;
  case EventKind::MutexWait:
    setConceptMoment("MUTEX: ESPERA CONTROLADA", detail,
                     "#F59E0B", "#451A03");
    break;
  case EventKind::WaitCycleDetected:
    setConceptMoment("CICLO DE ESPERA DETECTADO", detail,
                     "#FB7185", "#4C0519");
    break;
  case EventKind::WaitCycleRecovered:
    setConceptMoment("CICLO DE ESPERA RECUPERADO", detail,
                     "#FBBF24", "#422006");
    break;
  case EventKind::ChargingStarted:
    setConceptMoment("SECCIÓN CRÍTICA DE RECARGA", detail,
                     "#2DD4BF", "#134E4A");
    break;
  case EventKind::ChargingCompleted:
    setConceptMoment("RECARGA COMPLETADA", detail,
                     "#34D399", "#064E3B");
    break;
  case EventKind::Generic:
    return;
  }
  milestoneFrames_ = 30;
}
} // namespace simra
