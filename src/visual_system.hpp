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
inline constexpr auto canvas = "#F3F5EF";
inline constexpr auto surface = "#FFFFFF";
inline constexpr auto ink = "#172D2C";
inline constexpr auto muted = "#49615E";
inline constexpr auto border = "#CFD9D1";
inline constexpr auto primary = "#145549";
inline constexpr auto map = "#172B2D";
inline constexpr auto grid = "#2C4748";
inline constexpr auto light = "#8BCBF4";
inline constexpr auto heavy = "#CFB8F4";
inline constexpr auto waiting = "#F0C36D";
inline constexpr auto blocked = "#FFA398";
inline constexpr auto receipt = "#8FE1D2";
inline constexpr auto delivery = "#AFDB9C";
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
