#include "visual_system.hpp"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmapCache>
#include <QSvgRenderer>

// Referencing the resource explicitly also links it from the static library.
static void initVisualResources() { Q_INIT_RESOURCE(simra_visual); }

namespace simra::visual {
void applyTheme(QWidget &root) {
  auto palette = root.palette();
  palette.setColor(QPalette::Window, QColor(canvas));
  palette.setColor(QPalette::WindowText, QColor(ink));
  palette.setColor(QPalette::Base, QColor(surface));
  palette.setColor(QPalette::Text, QColor(ink));
  palette.setColor(QPalette::Highlight, QColor(primary));
  palette.setColor(QPalette::HighlightedText, Qt::white);
  root.setPalette(palette);
  root.setStyleSheet(QString(R"QSS(
    QWidget { color: %1; font-family: "Noto Sans", sans-serif; font-size: 14px; }
    QMainWindow, QStackedWidget { background: %2; }
    QFrame[visualPanel="true"] { background: %3; border: 1px solid %4; border-radius: 12px; }
    QLabel[role="display"] { font-size: 42px; font-weight: 700; }
    QLabel[role="title"] { font-size: 26px; font-weight: 700; }
    QLabel[role="section"] { font-size: 16px; font-weight: 700; }
    QLabel[role="metric"] { font-size: 25px; font-weight: 700; }
    QLabel[role="muted"] { color: %5; }
    QLabel[role="caption"] { color: %5; font-size: 12px; }
    QLabel[role="wordmark"] { color: %6; font-size: 16px; font-weight: 700; }
    QLabel[role="identifier"] { color: %5; font-family: monospace; font-size: 12px; }
    QPushButton { background: %3; border: 1px solid %4; border-radius: 8px;
                  min-height: 24px; padding: 8px 13px; font-weight: 600; }
    QPushButton:hover { background: #EAF2EB; border-color: #74998C; }
    QPushButton:focus { border: 2px solid #217466; padding: 7px 12px; }
    QPushButton[variant="primary"] { background: %6; color: white; border-color: %6; }
    QPushButton[variant="primary"]:hover { background: #1C695A; }
    QPushButton[variant="primary"]:focus { border-color: #84C9AC; }
    QPushButton:disabled, QPushButton[variant="primary"]:disabled {
      background: #E6ECE7; color: %5; border-color: %4;
    }
    QPushButton[variant="scenario"], QPushButton[variant="robot"] { text-align:left; }
    QPushButton[variant="scenario"] { font-size: 16px; border-color: transparent; }
    QPushButton[variant="scenario"]:hover { border-color: #74998C; }
    QPushButton[variant="scenario"]:focus { border-color: #217466; }
    QPushButton[variant="robot"]:checked { background: #E2F0E7; border-color: #4C8875; }
    QTabWidget::pane { border: 1px solid %4; background: %3; border-radius: 8px; }
    QTabBar::tab { background: #E6ECE7; color: %5; margin-right: 4px;
                  padding: 9px 13px; border: 1px solid %4;
                  border-top-left-radius: 7px; border-top-right-radius: 7px; }
    QTabBar::tab:selected { background: %3; color: %1; font-weight: 700; }
    QTabBar::tab:focus { border-color: #217466; }
    QSplitter::handle { background: %2; }
    QScrollArea { border: 0; background: %3; }
    QProgressBar { border: 0; border-radius: 3px; background: #DDE8DF; }
    QProgressBar::chunk { background: %6; border-radius: 3px; }
  )QSS").arg(ink, canvas, surface, border, muted, primary));
}

QLabel *label(const QString &text, const char *role) {
  auto *result = new QLabel(text);
  result->setProperty("role", role);
  return result;
}

QFrame *panel() {
  auto *result = new QFrame;
  result->setProperty("visualPanel", true);
  return result;
}

QPixmap symbol(const char *name, const QColor &color, int size) {
  static const bool initialized = [] { initVisualResources(); return true; }();
  Q_UNUSED(initialized);
  const auto key = QString("simra/%1/%2/%3").arg(name, color.name(QColor::HexArgb)).arg(size);
  QPixmap cached;
  if (QPixmapCache::find(key, &cached))
    return cached;
  QSvgRenderer svg(QString(":/simra/icons/%1.svg").arg(name));
  if (!svg.isValid())
    return {};
  QPixmap pixmap(size, size);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  svg.render(&painter);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.fillRect(pixmap.rect(), color);
  painter.end();
  QPixmapCache::insert(key, pixmap);
  return pixmap;
}

QIcon icon(const char *name, const QColor &color) {
  QIcon result;
  result.addPixmap(symbol(name, color, 24));
  auto retina = symbol(name, color, 48);
  retina.setDevicePixelRatio(2);
  result.addPixmap(retina);
  return result;
}

QHBoxLayout *mapLegend(bool showRoutes) {
  auto *layout = new QHBoxLayout;
  layout->setSpacing(7);
  const auto item = [&](const char *name, const char *text) {
    auto *glyph = new QLabel;
    glyph->setPixmap(symbol(name, QColor(primary), 17));
    layout->addWidget(glyph);
    layout->addWidget(label(text, "caption"));
    layout->addSpacing(6);
  };
  item("shelf", "Estante");
  item("receive", "Recepción");
  item("deliver", "Entrega");
  item("charge", "Recarga");
  if (showRoutes) {
    item("route", "Ruta");
    item("cargo", "Carga");
  }
  layout->addStretch();
  return layout;
}
} // namespace simra::visual
