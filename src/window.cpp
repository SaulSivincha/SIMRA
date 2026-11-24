#include "simra/window.hpp"
#include "simra/scenario_hub.hpp"
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
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
static QString label(OrderStatus s) {
  switch (s) {
  case OrderStatus::Pending:
    return "Pendiente";
  case OrderStatus::Assigned:
    return "Asignado";
  case OrderStatus::InProgress:
    return "En curso";
  case OrderStatus::Completed:
    return "Completado";
  case OrderStatus::Blocked:
    return "Bloqueado";
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
      [this](int index, bool edit) {
        scenarios_->setCurrentIndex(index);
        loadPreset();
        setEditing(edit);
        pages_->setCurrentWidget(workspace_);
      });
  pages_->addWidget(hub_);
  auto *root = new QVBoxLayout(workspace_);
  auto *toolbar = new QHBoxLayout;
  root->addLayout(toolbar);
  auto *title = new QLabel("SIMRA");
  title->setStyleSheet("font-size:24px;font-weight:bold;color:#60A5FA");
  toolbar->addWidget(title);
  scenarios_ = new QComboBox;
  scenarios_->addItem("Almacén básico", "basic");
  scenarios_->addItem("Flota mixta", "mixed");
  scenarios_->addItem("Congestión", "congestion");
  scenarios_->addItem("Competencia por recarga", "charging");
  toolbar->addWidget(scenarios_);
  auto *advancedMenu = new QMenu(this);
  auto *advanced = new QToolButton;
  advanced->setText("Herramientas");
  advanced->setPopupMode(QToolButton::InstantPopup);
  advanced->setMenu(advancedMenu);
  QHBoxLayout *buttonRow = toolbar;
  auto button = [&](QString text, auto action) {
    auto *b = new QPushButton(text, workspace_);
    if (text == "Abrir JSON" || text == "Guardar JSON" || text == "Clonar" ||
        text == "Configurar" || text == "Validar" || text == "Nuevo pedido" ||
        text == "Comparar 2/3") {
      b->hide();
      auto *entry = advancedMenu->addAction(text);
      connect(entry, &QAction::triggered, b, &QPushButton::click);
    } else {
      buttonRow->addWidget(b);
    }
    connect(b, &QPushButton::clicked, this, action);
    return b;
  };
  button("Inicio", [this] { pages_->setCurrentWidget(hub_); });
  button("Cargar escenario", [this] { loadPreset(); });
  button("Abrir JSON", [this] { openScenario(); });
  button("Guardar JSON", [this] {
    auto f = QFileDialog::getSaveFileName(this, "Guardar escenario", {},
                                          "JSON (*.json)");
    if (f.isEmpty())
      return;
    try {
      saveScenario(scenario_, f.toStdString());
      status_->setText("Escenario guardado");
    } catch (const std::exception &e) {
      QMessageBox::warning(this, "Error", e.what());
    }
  });
  button("Clonar", [this] {
    if (engine_ && !engine_->state().stopped())
      return;
    auto clone = scenario_.clone();
    clone->name += "_copia";
    scenario_ = *clone;
    status_->setText("Configuración clonada de forma independiente");
  });
  button("Configurar", [this] {
    if (engine_ && !engine_->state().stopped()) {
      QMessageBox::information(
          this, "SIMRA", "Carga un escenario para editar su configuración.");
      return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle("Configuración del escenario");
    QFormLayout form(&dialog);
    auto spin = [&](int minimum, int maximum, int value, QString label) {
      auto *box = new QSpinBox(&dialog);
      box->setRange(minimum, maximum);
      box->setValue(value);
      form.addRow(label, box);
      return box;
    };
    auto *width = spin(8, 30, scenario_.width, "Ancho (celdas)");
    auto *height = spin(8, 20, scenario_.height, "Alto (celdas)");
    auto *seed = spin(0, 999999, scenario_.seed, "Semilla");
    auto *interval = spin(100, 60000, scenario_.generationIntervalMs,
                          "Generación automática (ms)");
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
      return;
    try {
      auto candidate = scenario_;
      ScenarioEditor::resize(candidate, width->value(), height->value());
      candidate.seed = seed->value();
      candidate.generationIntervalMs = interval->value();
      scenario_ = std::move(candidate);
      engine_.reset();
      render(Snapshot{scenario_});
      status_->setText("Configuración actualizada. Valida antes de iniciar.");
    } catch (const std::exception &error) {
      QMessageBox::warning(this, "Configuración inválida", error.what());
    }
  });
  button("Validar", [this] {
    try {
      scenario_.validate();
      QMessageBox::information(this, "SIMRA", "Escenario válido para iniciar.");
    } catch (const std::exception &error) {
      QMessageBox::warning(this, "Escenario inválido", error.what());
    }
  });
  buttonRow = new QHBoxLayout;
  root->addLayout(buttonRow);
  editor_ = button("Editar mapa", [this] { setEditing(!editing_); });
  editor_->setObjectName("editScenario");
  tool_ = new QComboBox;
  tool_->setObjectName("editorTool");
  tool_->addItems({"Seleccionar", "Estante", "Obstáculo", "Robot ligero",
                   "Robot pesado", "Recarga ligera", "Recarga industrial",
                   "Entrega", "Recepción", "Mercancía", "Borrar"});
  start_ = button("Iniciar demostración", [this] {
    startSimulation();
    if (engine_)
      addSampleOrders();
  });
  start_->setObjectName("startSimulation");
  pause_ = button("Pausar", [this] {
    if (!engine_)
      return;
    bool paused = engine_->state().snapshot().paused;
    engine_->pause(!paused);
    pause_->setText(paused ? "Pausar" : "Reanudar");
    status_->setText(paused ? "Simulación en ejecución" : "Simulación pausada");
  });
  pause_->setObjectName("pauseSimulation");
  stop_ = button("Detener", [this] { stopSimulation(); });
  stop_->setObjectName("stopSimulation");
  button("Nuevo pedido", [this] { addOrder(); });
  compare_ = button("Comparar 2/3", [this] {
    comparison_->showRunning();
    rightTabs_->setCurrentWidget(comparison_);
    compare_->setEnabled(false);
    auto *process = new QProcess(this);
    auto program = QCoreApplication::applicationDirPath() + "/simra_compare";
    connect(process, &QProcess::finished, this,
            [this, process](int code, QProcess::ExitStatus) {
              if (code == 0)
                comparison_->showResult(process->readAllStandardOutput());
              else
                comparison_->showError(
                    QString::fromUtf8(process->readAllStandardError()));
              compare_->setEnabled(true);
              process->deleteLater();
            });
    connect(process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError) {
              comparison_->showError("no se pudo iniciar simra_compare");
              compare_->setEnabled(true);
            });
    process->start(program, {"--json", QString(SIMRA_SOURCE_DIR) +
                                           "/scenarios/basic.json"});
  });
  toolbar->addWidget(advanced);
  concept_ = new QLabel;
  concept_->setObjectName("demonstrationGuide");
  concept_->setWordWrap(true);
  concept_->setStyleSheet(
      "background:#1E293B;color:#CBD5E1;padding:12px;border-radius:8px;");
  root->addWidget(concept_);
  auto *split = new QSplitter(Qt::Horizontal);
  root->addWidget(split, 1);
  leftTabs_ = new QTabWidget;
  auto *left = leftTabs_;
  split->addWidget(left);
  orders_ = table({"Pedido", "Carga", "Tipo", "Estado", "Robot"});
  orders_->setObjectName("ordersTable");
  left->addTab(orders_, "Pedidos");
  inventory_ = table({"Mercancía", "Kg", "Ubicación"});
  connect(inventory_, &QTableWidget::cellDoubleClicked, this,
          [this](int row, int) {
            if (engine_)
              return;
            auto *item = inventory_->item(row, 0);
            if (!item)
              return;
            auto id = item->text().toStdString();
            auto cargo =
                std::find_if(scenario_.cargo.begin(), scenario_.cargo.end(),
                             [&](auto &c) { return c.id == id; });
            if (cargo == scenario_.cargo.end())
              return;
            bool ok = false;
            int weight =
                QInputDialog::getInt(this, "Mercancía " + item->text(),
                                     "Peso (kg)", cargo->weight, 1, 50, 1, &ok);
            if (ok) {
              ScenarioEditor::setCargoWeight(scenario_, id, weight);
              render(Snapshot{scenario_});
            }
          });
  left->addTab(inventory_, "Inventario");
  toolsPage_ = new QWidget;
  auto *toolLayout = new QVBoxLayout(toolsPage_);
  auto *toolHeading = new QLabel("Herramientas del mapa");
  toolHeading->setStyleSheet("font-weight:700;font-size:15px;");
  toolLayout->addWidget(toolHeading);
  toolLayout->addWidget(tool_);
  auto *toolHelp = new QLabel(
      "Clic para colocar o inspeccionar.\nClic derecho para borrar.");
  toolHelp->setWordWrap(true);
  toolHelp->setStyleSheet("color:#9CB1CC;");
  toolLayout->addWidget(toolHelp);
  toolLayout->addStretch();
  left->addTab(toolsPage_, "Herramientas");
  view_ = new WarehouseView;
  view_->setObjectName("warehouseMap");
  view_->setEditHandler([this](Position p, bool erase) {
    if (editing_ && !engine_)
      editCell(p, erase);
  });
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
  inspectorPage_ = new QWidget;
  auto *inspectLayout = new QVBoxLayout(inspectorPage_);
  auto *inspectHeading = new QLabel("Propiedades de la celda");
  inspectHeading->setStyleSheet("font-weight:700;font-size:15px;");
  inspectLayout->addWidget(inspectHeading);
  cellInspector_ = new QLabel;
  cellInspector_->setObjectName("cellInspector");
  cellInspector_->setTextFormat(Qt::PlainText);
  cellInspector_->setWordWrap(true);
  cellInspector_->setStyleSheet("color:#CBD5E1;padding:12px;");
  inspectLayout->addWidget(cellInspector_);
  inspectLayout->addStretch();
  right->addTab(inspectorPage_, "Inspector");
  split->setSizes({280, 710, 425});
  auto *activity = new QFrame;
  activity_ = activity;
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
void MainWindow::openScenario() {
  auto filename = QFileDialog::getOpenFileName(this, "Abrir escenario", {},
                                               "JSON (*.json)");
  if (filename.isEmpty())
    return;
  try {
    auto candidate = loadScenario(filename.toStdString());
    if (engine_)
      stopSimulation();
    engine_.reset();
    scenario_ = std::move(candidate);
    selectedRobot_.clear();
    setEditing(false);
    render(Snapshot{scenario_});
    pages_->setCurrentWidget(workspace_);
    status_->setText("Escenario validado");
  } catch (const std::exception &error) {
    QMessageBox::warning(this, "Escenario inválido", error.what());
  }
}
void MainWindow::setEditing(bool enabled) {
  if (engine_ && !engine_->state().stopped())
    return;
  if (enabled && engine_) {
    engine_.reset();
    render(Snapshot{scenario_});
  }
  editing_ = enabled;
  leftTabs_->setTabVisible(leftTabs_->indexOf(toolsPage_), enabled);
  rightTabs_->setTabVisible(rightTabs_->indexOf(inspectorPage_), enabled);
  leftTabs_->setCurrentIndex(enabled ? leftTabs_->indexOf(toolsPage_) : 0);
  rightTabs_->setCurrentIndex(enabled ? rightTabs_->indexOf(inspectorPage_)
                                      : 0);
  activity_->setVisible(!enabled);
  view_->selectCell({});
  cellInspector_->setText(
      "Selecciona una celda para consultar sus propiedades.");
  editor_->setText(enabled ? "Cerrar editor" : "Editar mapa");
  if (enabled)
    status_->setText("Elige una herramienta y haz clic en el plano. Clic "
                     "derecho para borrar.");
}
void MainWindow::loadPreset() {
  try {
    auto path = QString(SIMRA_SOURCE_DIR) + "/scenarios/" +
                scenarios_->currentData().toString() + ".json";
    auto candidate = loadScenario(path.toStdString());
    if (engine_)
      stopSimulation();
    engine_.reset();
    selectedRobot_.clear();
    scenario_ = std::move(candidate);
    setEditing(false);
    render(Snapshot{scenario_});
    status_->setText("Escenario " + scenarios_->currentText() + " validado");
    start_->setEnabled(true);
    pause_->setEnabled(false);
    stop_->setEnabled(false);
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
    setEditing(false);
    engine_ = std::make_unique<SimulationEngine>(scenario_);
    engine_->state().setSpeedPercent(100);
    engine_->start(false);
    start_->setEnabled(false);
    pause_->setEnabled(true);
    stop_->setEnabled(true);
    editor_->setEnabled(false);
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
  stop_->setEnabled(false);
  start_->setEnabled(true);
  editor_->setEnabled(true);
  pause_->setText("Pausar");
  status_->setText("Simulación detenida; todos los hilos unidos");
}
void MainWindow::addOrder() {
  if (!engine_ || engine_->state().snapshot().stopped) {
    QMessageBox::information(this, "SIMRA",
                             "Inicia una simulación para crear pedidos.");
    return;
  }
  auto snap = engine_->state().snapshot();
  QStringList available;
  for (const auto &c : snap.cargo) {
    const auto active = std::any_of(
        snap.orders.begin(), snap.orders.end(), [&](const auto &order) {
          return order.cargoId == c.id &&
                 order.status != OrderStatus::Completed;
        });
    if (c.locationId != "DELIVERED" && !active)
      available << QString::fromStdString(c.id);
  }
  if (available.empty()) {
    QMessageBox::information(this, "SIMRA", "No hay mercancía disponible.");
    return;
  }
  bool ok = false;
  auto id = QInputDialog::getItem(this, "Nuevo pedido", "Mercancía", available,
                                  0, false, &ok);
  if (!ok)
    return;
  auto cargo = std::find_if(snap.cargo.begin(), snap.cargo.end(), [&](auto &c) {
    return QString::fromStdString(c.id) == id;
  });
  bool reception = false;
  for (auto &st : snap.scenario.stations)
    if (st.id == cargo->locationId && st.kind == Terrain::Reception)
      reception = true;
  std::string destination;
  if (reception) {
    QStringList shelves;
    for (const auto &s : snap.scenario.shelves) {
      auto used = std::count_if(
          snap.cargo.begin(), snap.cargo.end(),
          [&](const auto &item) { return item.locationId == s.id; });
      if (used < s.capacity)
        shelves << QString::fromStdString(s.id);
    }
    if (shelves.empty()) {
      QMessageBox::information(this, "Reposición",
                               "No hay estantes con espacio libre.");
      return;
    }
    auto target = QInputDialog::getItem(this, "Reposición", "Estante destino",
                                        shelves, 0, false, &ok);
    if (!ok)
      return;
    destination = target.toStdString();
  } else {
    QStringList stations;
    for (const auto &station : snap.scenario.stations)
      if (station.kind == Terrain::Delivery)
        stations << QString::fromStdString(station.id);
    if (stations.empty()) {
      QMessageBox::information(this, "Entrega",
                               "Agrega una estación de entrega al escenario.");
      return;
    }
    destination = QInputDialog::getItem(this, "Entrega", "Estación de destino",
                                        stations, 0, false, &ok)
                      .toStdString();
    if (!ok)
      return;
  }
  int priority = QInputDialog::getInt(this, "Prioridad",
                                      "0 baja · 1 normal · 2 alta · 3 urgente",
                                      1, 0, 3, 1, &ok);
  if (!ok)
    return;
  std::string orderId = "O" + std::to_string(nextOrder_++);
  auto order = reception
                   ? RestockOrderCreator().create(orderId, id.toStdString(),
                                                  destination, priority)
                   : DeliveryOrderCreator().create(orderId, id.toStdString(),
                                                   destination, priority);
  if (!engine_->state().addOrder(std::move(order)))
    QMessageBox::warning(
        this, "Pedido rechazado",
        "La carga ya tiene un pedido activo o el destino no es válido.");
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
  concept_->setText(explanations.value(scenarios_->currentIndex()) +
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
  orders_->setRowCount(int(s.orders.size()));
  for (int i = 0; i < int(s.orders.size()); ++i) {
    auto &o = s.orders[i];
    QStringList values = {
        QString::fromStdString(o.id), QString::fromStdString(o.cargoId),
        o.kind == OrderKind::Delivery ? "Entrega" : "Reposición",
        label(o.status), QString::fromStdString(o.robotId)};
    for (int j = 0; j < values.size(); ++j)
      orders_->setItem(i, j, new QTableWidgetItem(values[j]));
  }
  auto cargoViews = s.cargo;
  if (cargoViews.empty())
    for (auto &cargo : s.scenario.cargo)
      cargoViews.push_back({cargo.id, cargo.locationId, cargo.weight});
  inventory_->setRowCount(int(cargoViews.size()));
  for (int i = 0; i < int(cargoViews.size()); ++i) {
    auto &cargo = cargoViews[i];
    QStringList values = {QString::fromStdString(cargo.id),
                          QString::number(cargo.weight),
                          QString::fromStdString(cargo.locationId)};
    for (int j = 0; j < values.size(); ++j)
      inventory_->setItem(i, j, new QTableWidgetItem(values[j]));
  }
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
void MainWindow::inspectCell(Position p) {
  for (auto &shelf : scenario_.shelves)
    if (shelf.position == p) {
      bool accepted = false;
      int capacity = QInputDialog::getInt(
          this, "Estante " + QString::fromStdString(shelf.id), "Capacidad",
          shelf.capacity, 1, 100, 1, &accepted);
      if (accepted)
        ScenarioEditor::setShelfCapacity(scenario_, shelf.id, capacity);
      return;
    }
  for (auto &robot : scenario_.robots)
    if (robot.position == p) {
      bool accepted = false;
      int maximum = robot.kind == RobotKind::Light ? 100 : 150;
      int battery = QInputDialog::getInt(
          this, "Robot " + QString::fromStdString(robot.id), "Batería inicial",
          robot.battery, 0, maximum, 1, &accepted);
      if (accepted)
        ScenarioEditor::setRobotBattery(scenario_, robot.id, battery);
      return;
    }
  for (auto &station : scenario_.stations)
    if (station.position == p) {
      QMessageBox::information(this, "Estación",
                               "ID: " + QString::fromStdString(station.id) +
                                   "\nCapacidad de servicio: 1 robot");
      return;
    }
  status_->setText(
      "Celda libre. Usa una herramienta para colocar un elemento.");
}
void MainWindow::editCell(Position p, bool erase) {
  try {
    view_->selectCell(p);
    auto tool = erase ? EditorTool::Erase
                      : static_cast<EditorTool>(tool_->currentIndex());
    if (tool == EditorTool::Inspect)
      inspectCell(p);
    else if (tool == EditorTool::Cargo) {
      bool accepted = false;
      int weight = QInputDialog::getInt(this, "Nueva mercancía", "Peso (kg)", 5,
                                        1, 50, 1, &accepted);
      if (!accepted)
        return;
      ScenarioEditor::place(scenario_, p, tool, weight);
    } else
      ScenarioEditor::place(scenario_, p, tool);
    showCell(p);
    render(Snapshot{scenario_});
    status_->setText(
        QString("Celda (%1,%2) actualizada. Valida antes de iniciar.")
            .arg(p.x)
            .arg(p.y));
  } catch (const std::exception &error) {
    QMessageBox::warning(this, "No se pudo editar", error.what());
  }
}
void MainWindow::showCell(Position p) {
  QString detail = QString("Columna %1 · Fila %2").arg(p.x + 1).arg(p.y + 1);
  for (const auto &shelf : scenario_.shelves)
    if (shelf.position == p)
      detail += QString("\n\nEstante %1\nCapacidad %2")
                    .arg(QString::fromStdString(shelf.id))
                    .arg(shelf.capacity);
  for (const auto &robot : scenario_.robots)
    if (robot.position == p)
      detail += QString("\n\nRobot %1\nBatería inicial %2 u.")
                    .arg(QString::fromStdString(robot.id))
                    .arg(robot.battery);
  for (const auto &station : scenario_.stations)
    if (station.position == p)
      detail += QString("\n\nEstación %1\nCapacidad de servicio · 1 robot")
                    .arg(QString::fromStdString(station.id));
  if (std::find(scenario_.obstacles.begin(), scenario_.obstacles.end(), p) !=
      scenario_.obstacles.end())
    detail += "\n\nObstáculo";
  cellInspector_->setText(detail);
}
} // namespace simra
