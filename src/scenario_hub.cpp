#include "simra/scenario_hub.hpp"
#include "simra/warehouse_view.hpp"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <array>

namespace simra {
ScenarioHub::ScenarioHub(std::function<void(int, bool)> openPreset,
                         QWidget *parent)
    : QWidget(parent) {
  auto *page = new QVBoxLayout(this);
  page->setContentsMargins(36, 30, 36, 30);
  page->setSpacing(22);
  auto *title = new QLabel("SIMRA");
  title->setStyleSheet("font-size:42px;font-weight:800;color:#EFF6FF;");
  page->addWidget(title);
  auto *subtitle =
      new QLabel("Simulador interactivo multihilo de robots de almacén");
  subtitle->setStyleSheet("font-size:19px;color:#9CB1CC;");
  page->addWidget(subtitle);
  auto *instruction = new QLabel(
      "Elige una demostración. Cada una muestra un concepto distinto de "
      "programación concurrente y objetos.");
  instruction->setStyleSheet("color:#A8B8CE;");
  instruction->setWordWrap(true);
  page->addWidget(instruction);
  auto *content = new QHBoxLayout;
  content->setSpacing(24);
  auto *options = new QVBoxLayout;
  options->setSpacing(12);
  const std::array<QString, 4> names = {"Almacén básico", "Flota mixta",
                                        "Pasillo congestionado",
                                        "Competencia por recarga"};
  const std::array<QString, 4> descriptions = {
      "<b>Demuestra: multihilo real.</b> Dos robots avanzan y atienden "
      "pedidos al mismo tiempo.",
      "<b>Demuestra: herencia y polimorfismo.</b> Un robot ligero y uno "
      "pesado responden distinto ante la misma tarea.",
      "<b>Demuestra: mutex y exclusión mutua.</b> Los robots esperan su "
      "turno para cruzar zonas compartidas.",
      "<b>Demuestra: sincronización de recursos.</b> Varios robots reservan "
      "y comparten un cargador."};
  for (int index = 0; index < int(names.size()); ++index) {
    auto *card = new QFrame;
    card->setStyleSheet("QFrame{background:#1E293B;border:1px solid "
                        "#334155;border-radius:10px;}"
                        "QLabel{border:0;background:transparent;}");
    auto *cardLayout = new QVBoxLayout(card);
    auto *button = new QPushButton(names[index]);
    button->setObjectName(QString("preset%1").arg(index));
    button->setStyleSheet("text-align:left;font-size:15px;font-weight:700;");
    connect(button, &QPushButton::clicked, this,
            [openPreset, index] { openPreset(index, false); });
    cardLayout->addWidget(button);
    auto *text = new QLabel(descriptions[index]);
    text->setObjectName(QString("scenarioConcept%1").arg(index));
    text->setStyleSheet("color:#A8B8CE;");
    text->setWordWrap(true);
    cardLayout->addWidget(text);
    options->addWidget(card);
  }
  options->addStretch();
  content->addLayout(options, 4);

  auto *map = new QVBoxLayout;
  description_ = new QLabel;
  description_->setStyleSheet("font-weight:700;color:#E2E8F0;");
  map->addWidget(description_);
  preview_ = new WarehouseView;
  map->addWidget(preview_, 1);
  auto *legend = new QLabel("E Entrega     R Recepción     C Recarga");
  legend->setStyleSheet("color:#9CB1CC;");
  map->addWidget(legend);
  content->addLayout(map, 8);
  page->addLayout(content, 1);
}
void ScenarioHub::preview(const Scenario &scenario) {
  description_->setText(QString("Vista previa · %1 × %2 · %3 robots")
                            .arg(scenario.width)
                            .arg(scenario.height)
                            .arg(scenario.robots.size()));
  preview_->render(Snapshot{scenario});
}
} // namespace simra
