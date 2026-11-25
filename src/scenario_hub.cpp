#include "simra/scenario_hub.hpp"
#include "simra/warehouse_view.hpp"
#include "visual_system.hpp"
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace simra {
ScenarioHub::ScenarioHub(std::function<void(int)> openPreset,
                         std::function<Scenario(int)> readScenario,
                         QWidget *parent)
    : QWidget(parent), readScenario_(std::move(readScenario)) {
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
  auto *hint = visual::label("Explora los planos y abre el escenario que quieras simular.", "muted");
  hint->setWordWrap(true);
  list->addWidget(hint);
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
    button->setProperty("previewIndex", index);
    button->installEventFilter(this);
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
  auto *key = visual::panel();
  auto *keyLayout = new QVBoxLayout(key);
  keyLayout->setContentsMargins(12, 9, 12, 9);
  keyLayout->setSpacing(4);
  keyLayout->addWidget(visual::label("Cómo leer la simulación", "section"));
  const auto addHint = [&](const char *symbol, const char *color, const char *text) {
    auto *row = new QHBoxLayout;
    auto *glyph = new QLabel;
    glyph->setPixmap(visual::symbol(symbol, QColor(color), 18));
    auto *message = visual::label(text, "caption");
    message->setWordWrap(true);
    row->addWidget(glyph);
    row->addWidget(message, 1);
    keyLayout->addLayout(row);
  };
  addHint("route", visual::light, "Línea punteada: recorrido previsto.");
  addHint("cargo", visual::cargo, "Cuadrado dorado: paquete que entra o sale del robot.");
  addHint("charge", visual::energy, "Pulso turquesa: transferencia de energía.");
  list->addWidget(key);
  list->addStretch();
  columns->addWidget(catalog, 4);

  auto *guide = visual::panel();
  auto *guideBody = new QVBoxLayout(guide);
  guideBody->setContentsMargins(12, 10, 12, 10);
  guideBody->setSpacing(8);
  previewTitle_ = visual::label({}, "section");
  previewDetails_ = visual::label({}, "caption");
  previewDetails_->setWordWrap(true);
  guideBody->addWidget(previewTitle_);
  guideBody->addWidget(previewDetails_);
  preview_ = new WarehouseView;
  preview_->setObjectName("scenarioPreview");
  preview_->setFocusPolicy(Qt::NoFocus);
  preview_->setAccessibleName("Vista previa del escenario");
  guideBody->addWidget(preview_, 1);
  guideBody->addLayout(visual::mapLegend(false));
  columns->addWidget(guide, 7);
  shell->addLayout(columns, 1);
  preview(0);
}

bool ScenarioHub::eventFilter(QObject *watched, QEvent *event) {
  if (preview_ && (event->type() == QEvent::Enter || event->type() == QEvent::FocusIn)) {
    const auto index = watched->property("previewIndex");
    if (index.isValid())
      preview(index.toInt());
  }
  return QWidget::eventFilter(watched, event);
}

void ScenarioHub::preview(int index) {
  if (index == previewIndex_)
    return;
  previewIndex_ = index;
  previewTitle_->setText(visual::scenarios.at(index).title);
  try {
    auto &scenario = scenarios_.at(index);
    if (!scenario)
      scenario = readScenario_(index);
    previewDetails_->setText(QString("%1 robots   ·   %2 × %3 celdas")
                                 .arg(scenario->robots.size())
                                 .arg(scenario->width)
                                 .arg(scenario->height));
    preview_->render(Snapshot{*scenario});
    preview_->show();
  } catch (const std::exception &) {
    previewDetails_->setText("Vista previa no disponible.");
    preview_->hide();
  }
}
} // namespace simra
