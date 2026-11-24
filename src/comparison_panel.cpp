#include "simra/comparison_panel.hpp"
#include <QGridLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QVBoxLayout>

namespace simra {
ComparisonPanel::ComparisonPanel(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);
  auto *heading = new QLabel("COMPARACIÓN CONTROLADA");
  heading->setStyleSheet("font-size:15px;font-weight:700;color:#F1F5F9;");
  layout->addWidget(heading);
  description_ =
      new QLabel("Escenario básico · mismos tres pedidos · horizonte de 8 s");
  description_->setWordWrap(true);
  description_->setStyleSheet("color:#94A3B8;");
  layout->addWidget(description_);
  status_ = new QLabel("Pulsa Comparar 2/3 para ejecutar dos corridas reales.");
  status_->setWordWrap(true);
  layout->addWidget(status_);
  table_ = new QTableWidget(2, 5);
  table_->setHorizontalHeaderLabels(
      {"Robots", "Pedidos", "Conflictos", "Distancia", "Energía"});
  table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  table_->verticalHeader()->hide();
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->setMaximumHeight(112);
  layout->addWidget(table_);
  auto *bars = new QGridLayout;
  bars->addWidget(new QLabel("Pedidos · 2 robots"), 0, 0);
  completedA_ = new QProgressBar;
  bars->addWidget(completedA_, 0, 1);
  bars->addWidget(new QLabel("Pedidos · 3 robots"), 1, 0);
  completedB_ = new QProgressBar;
  bars->addWidget(completedB_, 1, 1);
  bars->addWidget(new QLabel("Conflictos · 2 robots"), 2, 0);
  conflictsA_ = new QProgressBar;
  bars->addWidget(conflictsA_, 2, 1);
  bars->addWidget(new QLabel("Conflictos · 3 robots"), 3, 0);
  conflictsB_ = new QProgressBar;
  bars->addWidget(conflictsB_, 3, 1);
  for (auto *bar : {completedA_, completedB_, conflictsA_, conflictsB_}) {
    bar->setRange(0, 3);
    bar->setValue(0);
    bar->setStyleSheet(
        "QProgressBar{background:#111827;border:0;border-radius:5px;"
        "height:20px;text-align:center;color:white;}"
        "QProgressBar::chunk{background:#2563EB;border-radius:5px;}");
  }
  conflictsA_->setStyleSheet(
      conflictsA_->styleSheet().replace("#2563EB", "#D97706"));
  conflictsB_->setStyleSheet(
      conflictsB_->styleSheet().replace("#2563EB", "#D97706"));
  layout->addLayout(bars);
  layout->addStretch();
}
void ComparisonPanel::showRunning() {
  status_->setText("Ejecutando: dos corridas de 8 segundos. La interfaz "
                   "permanece disponible.");
}
void ComparisonPanel::showError(const QString &message) {
  status_->setText("Error: " + message);
}
void ComparisonPanel::showResult(const QByteArray &json) {
  QJsonParseError error;
  auto document = QJsonDocument::fromJson(json, &error);
  if (error.error != QJsonParseError::NoError || !document.isObject()) {
    showError("la salida de comparación no es JSON válido");
    return;
  }
  auto root = document.object();
  auto runs = root.value("runs").toArray();
  if (runs.size() != 2) {
    showError("faltan resultados de alguna corrida");
    return;
  }
  description_->setText(
      QString("Semilla %1 · %2 pedidos iguales · horizonte %3 s")
          .arg(root.value("seed").toInt())
          .arg(root.value("orderCount").toInt())
          .arg(root.value("horizonSeconds").toInt()));
  auto a = runs[0].toObject(), b = runs[1].toObject();
  int maxCompleted = std::max(
      3, std::max(a.value("completed").toInt(), b.value("completed").toInt()));
  int maxConflicts = std::max(
      1, std::max(a.value("conflicts").toInt(), b.value("conflicts").toInt()));
  for (int row = 0; row < 2; ++row) {
    auto run = runs[row].toObject();
    QStringList values = {QString::number(run.value("robots").toInt()),
                          QString::number(run.value("completed").toInt()),
                          QString::number(run.value("conflicts").toInt()),
                          QString::number(run.value("distance").toInt()),
                          QString::number(run.value("energy").toInt())};
    for (int column = 0; column < values.size(); ++column)
      table_->setItem(row, column, new QTableWidgetItem(values[column]));
  }
  completedA_->setRange(0, maxCompleted);
  completedA_->setValue(a.value("completed").toInt());
  completedA_->setFormat(QString::number(a.value("completed").toInt()) +
                         " pedidos");
  completedB_->setRange(0, maxCompleted);
  completedB_->setValue(b.value("completed").toInt());
  completedB_->setFormat(QString::number(b.value("completed").toInt()) +
                         " pedidos");
  conflictsA_->setRange(0, maxConflicts);
  conflictsA_->setValue(a.value("conflicts").toInt());
  conflictsA_->setFormat(QString::number(a.value("conflicts").toInt()) +
                         " conflictos");
  conflictsB_->setRange(0, maxConflicts);
  conflictsB_->setValue(b.value("conflicts").toInt());
  conflictsB_->setFormat(QString::number(b.value("conflicts").toInt()) +
                         " conflictos");
  status_->setText("Corridas terminadas. El planificador de hilos puede "
                   "cambiar los resultados.");
}
} // namespace simra
