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
  auto *page = new QHBoxLayout(this);
  page->setContentsMargins(24, 18, 24, 18);
  auto *content = new QWidget;
  content->setMaximumWidth(1520);
  page->addWidget(content, 1);
  auto *shell = new QVBoxLayout(content);
  shell->setContentsMargins(0, 0, 0, 0);
  shell->setSpacing(14);

  auto *brand = new QHBoxLayout;
  auto *mark = new QLabel;
  mark->setPixmap(visual::symbol("brand", QColor(visual::light), 24));
  brand->addWidget(mark);
  brand->addWidget(visual::label("SIMRA", "wordmark"));
  brand->addStretch();
  shell->addLayout(brand);

  auto *columns = new QHBoxLayout;
  columns->setSpacing(16);
  auto *catalog = new QWidget;
  catalog->setMaximumWidth(470);
  auto *list = new QVBoxLayout(catalog);
  list->setContentsMargins(0, 0, 0, 0);
  list->setSpacing(8);
  list->addWidget(visual::label("Robots de almacén", "title"));
  list->addWidget(visual::label("Elige un escenario", "muted"));
  const std::array accent{visual::light, visual::heavy, visual::waiting, visual::energy};
  for (int index = 0; index < int(visual::scenarios.size()); ++index) {
    const auto &scenario = visual::scenarios[index];
    auto *card = visual::panel();
    auto *body = new QVBoxLayout(card);
    body->setContentsMargins(8, 5, 8, 9);
    body->setSpacing(2);
    auto *button = new QPushButton(visual::icon(scenario.symbol, QColor(accent[index])),
                                   scenario.title);
    button->setObjectName(QString("preset%1").arg(index));
    button->setProperty("variant", "scenario");
    button->setIconSize(QSize(22, 22));
    button->setCursor(Qt::PointingHandCursor);
    button->setAccessibleDescription(scenario.observation);
    connect(button, &QPushButton::clicked, this,
            [openPreset, index] { openPreset(index); });
    body->addWidget(button);
    auto *observation = visual::label(scenario.observation, "muted");
    observation->setWordWrap(true);
    observation->setContentsMargins(9, 0, 9, 0);
    body->addWidget(observation);
    auto *concepts = visual::label(scenario.concepts, "caption");
    concepts->setObjectName(QString("scenarioConcept%1").arg(index));
    concepts->setWordWrap(true);
    concepts->setContentsMargins(9, 0, 9, 0);
    body->addWidget(concepts);
    list->addWidget(card);
  }
  list->addStretch();
  columns->addWidget(catalog, 4);

  auto *guide = visual::panel();
  auto *guideBody = new QVBoxLayout(guide);
  guideBody->setContentsMargins(12, 10, 12, 10);
  guideBody->setSpacing(8);
  guideBody->addWidget(visual::label("Plano de ejemplo", "section"));
  preview_ = new WarehouseView;
  preview_->setFocusPolicy(Qt::NoFocus);
  preview_->setAccessibleName("Plano de ejemplo del almacén");
  guideBody->addWidget(preview_, 1);
  const auto addHint = [&](const char *symbol, const char *color, const char *text) {
    auto *row = new QHBoxLayout;
    auto *glyph = new QLabel;
    glyph->setPixmap(visual::symbol(symbol, QColor(color), 18));
    auto *message = visual::label(text, "caption");
    message->setWordWrap(true);
    row->addWidget(glyph);
    row->addWidget(message, 1);
    guideBody->addLayout(row);
  };
  addHint("route", visual::light, "Línea punteada: recorrido previsto.");
  addHint("cargo", visual::cargo, "Cuadrado dorado: paquete que entra o sale del robot.");
  addHint("charge", visual::energy, "Pulso turquesa: transferencia de energía.");
  columns->addWidget(guide, 7);
  shell->addLayout(columns, 1);
}

void ScenarioHub::preview(const Scenario &scenario) {
  preview_->render(Snapshot{scenario});
}
} // namespace simra
