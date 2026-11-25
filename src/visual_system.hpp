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
inline constexpr auto canvas = "#F4F5F8";
inline constexpr auto surface = "#FFFFFF";
inline constexpr auto ink = "#182638";
inline constexpr auto muted = "#46566D";
inline constexpr auto border = "#D5DEEC";
inline constexpr auto primary = "#3153A4";
inline constexpr auto map = "#172334";
inline constexpr auto grid = "#304259";
inline constexpr auto light = "#8DCCFF";
inline constexpr auto heavy = "#D5B9FF";
inline constexpr auto cargo = "#FFD275";
inline constexpr auto energy = "#78E4CD";
inline constexpr auto waiting = "#FFB079";
inline constexpr auto blocked = "#FF9DAA";
inline constexpr auto receipt = "#9CDCEB";
inline constexpr auto delivery = "#ADDDA3";
inline constexpr auto storage = "#DCC39D";

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
