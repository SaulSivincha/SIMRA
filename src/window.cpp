#include "simra/window.hpp"
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
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
      "QTableWidget,QTextEdit{background:#1E293B;gridline-color:#334155;border:"
      "1px solid #334155} "
      "QHeaderView::section{background:#334155;color:white;padding:6px;border:"
      "0}");
  auto *central = new QWidget;
  setCentralWidget(central);
  auto *root = new QVBoxLayout(central);
  auto *toolbar = new QHBoxLayout;
  root->addLayout(toolbar);
  auto *title = new QLabel("SIMRA");
  title->setStyleSheet("font-size:24px;font-weight:bold;color:#60A5FA");
  toolbar->addWidget(title);
  scenarios_ = new QComboBox;
  scenarios_->addItems({"basic", "mixed", "congestion", "charging"});
  toolbar->addWidget(scenarios_);
  auto button = [&](QString text, auto action) {
    auto *b = new QPushButton(text);
    toolbar->addWidget(b);
    connect(b, &QPushButton::clicked, this, action);
    return b;
  };
  button("Cargar escenario", [this] { loadPreset(); });
  button("Abrir JSON", [this] {
    auto f = QFileDialog::getOpenFileName(this, "Abrir escenario", {},
                                          "JSON (*.json)");
    if (f.isEmpty())
      return;
    try {
      if (engine_) {
        stopSimulation();
        engine_.reset();
      }
      scenario_ = loadScenario(f.toStdString());
      render(Snapshot{scenario_});
      status_->setText("Escenario validado");
    } catch (const std::exception &e) {
      QMessageBox::warning(this, "Error", e.what());
    }
  });
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
    if (engine_) {
      QMessageBox::information(this, "SIMRA",
                               "Detén la simulación antes de clonar.");
      return;
    }
    auto clone = scenario_.clone();
    clone->name += "_copia";
    scenario_ = *clone;
    status_->setText("Configuración clonada de forma independiente");
  });
  editor_ = button("Editar mapa", [this] {
    if (engine_) {
      QMessageBox::information(
          this, "SIMRA", "Carga o reinicia un escenario antes de editar.");
      return;
    }
    editing_ = !editing_;
    editor_->setText(editing_ ? "Cerrar editor" : "Editar mapa");
    status_->setText(editing_ ? "Selecciona una herramienta y haz clic en el "
                                "mapa; clic derecho borra"
                              : "Editor cerrado");
  });
  tool_ = new QComboBox;
  tool_->addItems({"Estante", "Obstáculo", "Robot ligero", "Robot pesado",
                   "Recarga ligera", "Recarga industrial", "Entrega",
                   "Recepción", "Borrar"});
  toolbar->addWidget(tool_);
  auto *automatic = button("Auto: no", [this] {
    automatic_ = !automatic_;
    auto *b = qobject_cast<QPushButton *>(sender());
    b->setText(automatic_ ? "Auto: sí" : "Auto: no");
  });
  (void)automatic;
  start_ = button("Iniciar", [this] { startSimulation(); });
  pause_ = button("Pausar", [this] {
    if (!engine_)
      return;
    bool paused = engine_->state().snapshot().paused;
    engine_->pause(!paused);
    pause_->setText(paused ? "Pausar" : "Reanudar");
  });
  stop_ = button("Detener", [this] { stopSimulation(); });
  button("Nuevo pedido", [this] { addOrder(); });
  auto *compareButton = button("Comparar 2/3", [this] {
    comparison_->setPlainText(
        "Ejecutando dos simulaciones de 8 segundos con la misma carga...");
    auto *process = new QProcess(this);
    auto program = QCoreApplication::applicationDirPath() + "/simra_compare";
    connect(process, &QProcess::finished, this,
            [this, process](int code, QProcess::ExitStatus) {
              comparison_->setPlainText(
                  QString::fromUtf8(process->readAllStandardOutput()) +
                  QString::fromUtf8(process->readAllStandardError()));
              if (code != 0)
                comparison_->append("La comparación terminó con error.");
              process->deleteLater();
            });
    process->start(program,
                   {QString(SIMRA_SOURCE_DIR) + "/scenarios/basic.json"});
  });
  (void)compareButton;
  auto *split = new QSplitter(Qt::Horizontal);
  root->addWidget(split, 1);
  auto *left = new QTabWidget;
  split->addWidget(left);
  orders_ = table({"Pedido", "Carga", "Tipo", "Estado", "Robot"});
  left->addTab(orders_, "Pedidos");
  events_ = new QTextEdit;
  events_->setReadOnly(true);
  left->addTab(events_, "Eventos");
  scene_ = new QGraphicsScene(this);
  view_ = new QGraphicsView(scene_);
  view_->setRenderHint(QPainter::Antialiasing);
  view_->setBackgroundBrush(QColor("#E8EDF2"));
  view_->viewport()->installEventFilter(this);
  split->addWidget(view_);
  auto *right = new QTabWidget;
  split->addWidget(right);
  robots_ = table({"Robot", "Estado", "Batería", "Carga", "Hilo"});
  right->addTab(robots_, "Robots e hilos");
  metrics_ = new QLabel;
  metrics_->setAlignment(Qt::AlignTop);
  metrics_->setMargin(16);
  right->addTab(metrics_, "Estadísticas");
  comparison_ = new QTextEdit;
  comparison_->setReadOnly(true);
  comparison_->setPlainText(
      "Comparación controlada del escenario básico: 2 frente a 3 robots, mismo "
      "inventario, pedidos y horizonte de 8 s.");
  right->addTab(comparison_, "Comparación");
  split->setSizes({300, 800, 380});
  status_ = new QLabel("Listo");
  root->addWidget(status_);
  timer_ = new QTimer(this);
  timer_->setInterval(100);
  connect(timer_, &QTimer::timeout, this, [this] { refresh(); });
  timer_->start();
  loadPreset();
}
void MainWindow::loadPreset() {
  if (engine_) {
    stopSimulation();
    engine_.reset();
  }
  try {
    auto path = QString(SIMRA_SOURCE_DIR) + "/scenarios/" +
                scenarios_->currentText() + ".json";
    scenario_ = loadScenario(path.toStdString());
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
    editing_ = false;
    editor_->setText("Editar mapa");
    engine_ = std::make_unique<SimulationEngine>(scenario_);
    engine_->start(automatic_);
    start_->setEnabled(false);
    pause_->setEnabled(true);
    stop_->setEnabled(true);
    status_->setText("Simulación en ejecución");
  } catch (const std::exception &e) {
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
  for (auto &c : snap.cargo)
    if (c.locationId != "DELIVERED")
      available << QString::fromStdString(c.id);
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
    for (auto &s : snap.scenario.shelves)
      shelves << QString::fromStdString(s.id);
    auto target = QInputDialog::getItem(this, "Reposición", "Estante destino",
                                        shelves, 0, false, &ok);
    if (!ok)
      return;
    destination = target.toStdString();
  } else {
    for (auto &s : snap.scenario.stations)
      if (s.kind == Terrain::Delivery) {
        destination = s.id;
        break;
      }
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
  startSimulation();
  if (engine_)
    engine_->state().addOrder(
        DeliveryOrderCreator().create("DEMO1", "K01", "D01", 2));
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
  constexpr int cell = 42;
  scene_->clear();
  scene_->setSceneRect(0, 0, s.scenario.width * cell, s.scenario.height * cell);
  for (int y = 0; y < s.scenario.height; ++y)
    for (int x = 0; x < s.scenario.width; ++x) {
      Position p{x, y};
      QColor color("#F8FAFC");
      QString name;
      for (auto q : s.scenario.obstacles)
        if (q == p) {
          color = QColor("#526174");
          name = "×";
        }
      for (auto &q : s.scenario.shelves)
        if (q.position == p) {
          color = QColor("#A3825D");
          name = "▣";
        }
      for (auto &q : s.scenario.stations)
        if (q.position == p) {
          if (q.kind == Terrain::Delivery) {
            color = QColor("#16A34A");
            name = "D";
          } else if (q.kind == Terrain::Reception) {
            color = QColor("#0891B2");
            name = "P";
          } else {
            color = QColor("#D97706");
            name = "⚡";
          }
        }
      scene_->addRect(x * cell, y * cell, cell, cell, QPen(QColor("#CBD5E1")),
                      QBrush(color));
      if (!name.isEmpty()) {
        auto *t = scene_->addText(name);
        t->setDefaultTextColor(Qt::white);
        t->setPos(x * cell + 11, y * cell + 6);
      }
    }
  auto robotViews = s.robots;
  if (robotViews.empty())
    for (auto &r : s.scenario.robots)
      robotViews.push_back({r.id, "", "", "", r.position, r.kind,
                            RobotStatus::Idle, r.battery,
                            r.kind == RobotKind::Light ? 100 : 150});
  for (auto &r : robotViews) {
    QColor color =
        r.kind == RobotKind::Light ? QColor("#2563EB") : QColor("#9333EA");
    scene_->addEllipse(r.position.x * cell + 4, r.position.y * cell + 4,
                       cell - 8, cell - 8, QPen(Qt::white, 2), QBrush(color));
    auto *t = scene_->addText(QString::fromStdString(r.id));
    t->setDefaultTextColor(Qt::white);
    t->setScale(.72);
    t->setPos(r.position.x * cell + 7, r.position.y * cell + 10);
  }
  robots_->setRowCount(int(s.robots.size()));
  for (int i = 0; i < int(s.robots.size()); ++i) {
    auto &r = s.robots[i];
    QStringList values = {
        QString::fromStdString(r.id), label(r.status),
        QString::number(r.battery) + "/" + QString::number(r.maxBattery),
        QString::fromStdString(r.carrying), QString::fromStdString(r.threadId)};
    for (int j = 0; j < values.size(); ++j)
      robots_->setItem(i, j, new QTableWidgetItem(values[j]));
  }
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
  int pending = 0;
  for (auto &o : s.orders)
    if (o.status == OrderStatus::Pending)
      ++pending;
  metrics_->setText(
      QString("Pedidos completados: %1\nPendientes: %2\nDistancia: %3 "
              "celdas\nEnergía consumida: %4 u.\nConflictos: %5\nCiclos de "
              "espera detectados: %6\nRecargas: %7\nTiempo real: %8 s\n"
              "Tiempo activo: %9 s\nAtención real media: %10 s\nEstado: %11")
          .arg(s.metrics.completed)
          .arg(pending)
          .arg(s.metrics.distance)
          .arg(s.metrics.energy)
          .arg(s.metrics.conflicts)
          .arg(s.metrics.cycles)
          .arg(s.metrics.recharges)
          .arg(QString::number(s.metrics.realMs / 1000.0, 'f', 1))
          .arg(QString::number(s.metrics.activeMs / 1000.0, 'f', 1))
          .arg(s.metrics.completed
                   ? QString::number(s.metrics.attentionTotalMs / 1000.0 /
                                         s.metrics.completed,
                                     'f', 1)
                   : "—")
          .arg(s.stopped  ? "Detenido"
               : s.paused ? "Pausado"
                          : "Activo"));
  view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}
bool MainWindow::eventFilter(QObject *object, QEvent *event) {
  if (object == view_->viewport() && editing_ && !engine_ &&
      event->type() == QEvent::MouseButtonPress) {
    auto *mouse = static_cast<QMouseEvent *>(event);
    auto scenePoint = view_->mapToScene(mouse->pos());
    Position p{int(scenePoint.x()) / 42, int(scenePoint.y()) / 42};
    if (p.x >= 0 && p.y >= 0 && p.x < scenario_.width && p.y < scenario_.height)
      editCell(p, mouse->button() == Qt::RightButton ? view_ : nullptr);
    return true;
  }
  return QMainWindow::eventFilter(object, event);
}
void MainWindow::editCell(Position p, QObject *eraseSignal) {
  auto removeAt = [&] {
    scenario_.obstacles.erase(
        std::remove(scenario_.obstacles.begin(), scenario_.obstacles.end(), p),
        scenario_.obstacles.end());
    for (auto it = scenario_.shelves.begin(); it != scenario_.shelves.end();) {
      if (it->position == p) {
        auto id = it->id;
        scenario_.cargo.erase(
            std::remove_if(scenario_.cargo.begin(), scenario_.cargo.end(),
                           [&](auto &c) { return c.locationId == id; }),
            scenario_.cargo.end());
        it = scenario_.shelves.erase(it);
      } else
        ++it;
    }
    for (auto it = scenario_.stations.begin();
         it != scenario_.stations.end();) {
      if (it->position == p) {
        auto id = it->id;
        scenario_.cargo.erase(
            std::remove_if(scenario_.cargo.begin(), scenario_.cargo.end(),
                           [&](auto &c) { return c.locationId == id; }),
            scenario_.cargo.end());
        it = scenario_.stations.erase(it);
      } else
        ++it;
    }
    scenario_.robots.erase(
        std::remove_if(scenario_.robots.begin(), scenario_.robots.end(),
                       [&](auto &r) { return r.position == p; }),
        scenario_.robots.end());
  };
  removeAt();
  auto choice = eraseSignal ? QString("Borrar") : tool_->currentText();
  auto uniqueId = [&](std::string prefix) {
    int i = 1;
    while (true) {
      auto id = prefix + std::to_string(i++);
      bool used = false;
      for (auto &s : scenario_.shelves)
        used |= s.id == id;
      for (auto &s : scenario_.stations)
        used |= s.id == id;
      for (auto &r : scenario_.robots)
        used |= r.id == id;
      if (!used)
        return id;
    }
  };
  if (choice == "Estante")
    scenario_.shelves.push_back({uniqueId("S"), p, 5});
  else if (choice == "Obstáculo")
    scenario_.obstacles.push_back(p);
  else if (choice == "Robot ligero")
    scenario_.robots.push_back({uniqueId("R"), RobotKind::Light, p, 100});
  else if (choice == "Robot pesado")
    scenario_.robots.push_back({uniqueId("R"), RobotKind::Heavy, p, 150});
  else if (choice == "Recarga ligera")
    scenario_.stations.push_back({uniqueId("C"), Terrain::ChargeLight, p});
  else if (choice == "Recarga industrial")
    scenario_.stations.push_back({uniqueId("C"), Terrain::ChargeHeavy, p});
  else if (choice == "Entrega")
    scenario_.stations.push_back({uniqueId("D"), Terrain::Delivery, p});
  else if (choice == "Recepción")
    scenario_.stations.push_back({uniqueId("P"), Terrain::Reception, p});
  render(Snapshot{scenario_});
  status_->setText(
      QString("Celda (%1,%2) editada. Usa Iniciar para validar el mapa.")
          .arg(p.x)
          .arg(p.y));
}
} // namespace simra
