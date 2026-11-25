#include "simra/scenario_hub.hpp"
#include "visual_system.hpp"
#include <QFrame>
#include <QGridLayout>
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
  page->addStretch(1);

  auto *hero = new QHBoxLayout;
  hero->setSpacing(24);
  auto *intro = new QVBoxLayout;
  intro->setAlignment(Qt::AlignVCenter);
  intro->setSpacing(7);
  intro->addWidget(visual::label("Robots de almacén", "display"));
  auto *summary = visual::label(
      "Sigue un paquete desde su recogida hasta la entrega y observa cómo los robots comparten el almacén.",
      "muted");
  summary->setMaximumWidth(490);
  summary->setWordWrap(true);
  intro->addWidget(summary);
  hero->addLayout(intro, 5);

  auto *guide = visual::panel();
  auto *guideBody = new QVBoxLayout(guide);
  guideBody->setContentsMargins(18, 14, 18, 14);
  guideBody->addWidget(visual::label("Cómo leer el plano", "section"));
  const auto addHint = [&](const char *symbol, const char *color, const char *text) {
    auto *row = new QHBoxLayout;
    auto *glyph = new QLabel;
    glyph->setPixmap(visual::symbol(symbol, QColor(color).darker(215), 20));
    auto *message = visual::label(text, "muted");
    message->setWordWrap(true);
    row->addWidget(glyph);
    row->addWidget(message, 1);
    guideBody->addLayout(row);
  };
  addHint("route", visual::light, "Línea punteada: próximo recorrido del robot.");
  addHint("cargo", visual::cargo, "Cuadrado dorado: un paquete entra o sale del robot.");
  addHint("charge", visual::energy, "Pulso verde menta: el cargador transfiere energía.");
  hero->addWidget(guide, 4);
  page->addLayout(hero);
  page->addWidget(visual::label("Elige un escenario", "title"));

  auto *catalog = new QGridLayout;
  catalog->setSpacing(13);
  catalog->setColumnStretch(0, 1);
  catalog->setColumnStretch(1, 1);
  for (int index = 0; index < int(visual::scenarios.size()); ++index) {
    const auto &scenario = visual::scenarios[index];
    auto *card = visual::panel();
    auto *body = new QVBoxLayout(card);
    body->setContentsMargins(16, 13, 16, 17);
    body->setSpacing(6);
    auto *button = new QPushButton(visual::icon(scenario.symbol), scenario.title);
    button->setObjectName(QString("preset%1").arg(index));
    button->setProperty("variant", "scenario");
    button->setIconSize(QSize(24, 24));
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
    catalog->addWidget(card, index / 2, index % 2);
  }
  page->addLayout(catalog);
  page->addStretch(2);
}
} // namespace simra
