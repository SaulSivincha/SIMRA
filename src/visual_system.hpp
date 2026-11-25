#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <array>

class QLabel;
class QWidget;
class QFrame;
class QHBoxLayout;

namespace simra::visual {
inline constexpr auto canvas = "#111217";
inline constexpr auto surface = "#181B20";
inline constexpr auto ink = "#F4F4F4";
inline constexpr auto muted = "#BEC5D0";
inline constexpr auto border = "#373D47";
inline constexpr auto primary = "#0F62FE";
inline constexpr auto map = "#13161C";
inline constexpr auto grid = "#313846";
inline constexpr auto light = "#78A9FF";
inline constexpr auto heavy = "#D4BBFF";
inline constexpr auto cargo = "#F1C21B";
inline constexpr auto energy = "#3DDBD9";
inline constexpr auto waiting = "#FFB784";
inline constexpr auto blocked = "#FF8389";
inline constexpr auto receipt = "#9CDCEB";
inline constexpr auto delivery = "#ADDDA3";
inline constexpr auto storage = "#DCC39D";
inline constexpr auto onMarker = "#101820";

struct ScenarioPresentation {
  const char *id;
  const char *title;
  const char *symbol;
  const char *concepts;
  const char *observation;
};

inline constexpr std::array scenarios{
    ScenarioPresentation{"basic", "Hilos y pedidos concurrentes", "threads",
                         "Cola segura, hilos independientes y encapsulación",
                         "Observa cómo dos robots toman pedidos del mismo buffer."},
    ScenarioPresentation{"mixed", "Herencia y polimorfismo", "types",
                         "Herencia, polimorfismo y familias de productos",
                         "Compara la asignación de cargas de 5 y 25 kg."},
    ScenarioPresentation{"congestion", "Mutex: pasillo compartido", "mutex",
                         "Exclusión mutua, reservas y ciclos de espera",
                         "Sigue la espera y liberación de un recurso compartido."},
    ScenarioPresentation{"charging", "Sincronización: recarga compartida", "charge",
                         "Recurso compartido, reserva exclusiva y sincronización",
                         "Sigue los turnos de tres robots en un mismo cargador."}};

void applyTheme(QWidget &root);
QLabel *label(const QString &text, const char *role);
QFrame *panel();
QIcon icon(const char *name, const QColor &color = QColor(ink));
QPixmap symbol(const char *name, const QColor &color, int size);
QHBoxLayout *mapLegend(bool showRoutes);
} // namespace simra::visual
