#include "simra/scenario_hub.hpp"
#include "simra/warehouse_view.hpp"
#include "visual_system.hpp"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace simra {
ScenarioHub::ScenarioHub(std::function<void(int)> openPreset, QWidget *parent)
    : QWidget(parent) {
  auto *page = new QVBoxLayout(this);
  page->setContentsMargins(26, 22, 26, 22);
  page->setSpacing(18);
  auto *brand = new QHBoxLayout;
  auto *mark = new QLabel;
  mark->setPixmap(visual::symbol("brand", QColor(visual::primary), 26));
  brand->addWidget(mark);
  brand->addWidget(visual::label("SIMRA", "wordmark"));
  brand->addStretch();
  page->addLayout(brand);

  auto *intro = new QVBoxLayout;
  intro->setSpacing(3);
  intro->addWidget(visual::label("Robots de almacén", "display"));
  intro->addWidget(visual::label(
      "Un simulador para observar concurrencia y programación orientada a objetos.", "muted"));
  page->addLayout(intro);

  auto *content = new QHBoxLayout;
  content->setSpacing(20);
  auto *catalog = new QVBoxLayout;
  catalog->setSpacing(8);
  catalog->addWidget(visual::label("Escenarios", "section"));
  for (int index = 0; index < int(visual::scenarios.size()); ++index) {
    const auto &scenario = visual::scenarios[index];
    auto *card = visual::panel();
    auto *body = new QVBoxLayout(card);
    body->setContentsMargins(9, 7, 9, 9);
    body->setSpacing(4);
    auto *button = new QPushButton(visual::icon(scenario.symbol), scenario.title);
    button->setObjectName(QString("preset%1").arg(index));
    button->setProperty("variant", "scenario");
    button->setIconSize(QSize(22, 22));
    button->setCursor(Qt::PointingHandCursor);
    button->setAccessibleDescription(scenario.observation);
    connect(button, &QPushButton::clicked, this,
            [openPreset, index] { openPreset(index); });
    body->addWidget(button);
    auto *concepts = visual::label(scenario.concepts, "caption");
    concepts->setObjectName(QString("scenarioConcept%1").arg(index));
    concepts->setWordWrap(true);
    concepts->setContentsMargins(5, 0, 5, 0);
    body->addWidget(concepts);
    catalog->addWidget(card);
  }
  catalog->addStretch();
  content->addLayout(catalog, 4);

  auto *mapPanel = visual::panel();
  auto *map = new QVBoxLayout(mapPanel);
  map->setContentsMargins(14, 12, 14, 12);
  map->addWidget(visual::label("Plano del almacén", "section"));
  description_ = visual::label({}, "caption");
  map->addWidget(description_);
  preview_ = new WarehouseView;
  preview_->setAccessibleName("Vista previa del plano del almacén");
  preview_->setFocusPolicy(Qt::NoFocus);
  map->addWidget(preview_, 1);
  map->addLayout(visual::mapLegend(false));
  content->addWidget(mapPanel, 6);
  page->addLayout(content, 1);
}
void ScenarioHub::preview(const Scenario &scenario) {
  description_->setText(QString("Vista previa del primer escenario  /  %1 × %2 celdas  /  %3 robots")
                            .arg(scenario.width)
                            .arg(scenario.height)
                            .arg(scenario.robots.size()));
  preview_->render(Snapshot{scenario});
}
} // namespace simra
