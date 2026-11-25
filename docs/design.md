# Diseño visual de SIMRA

La interfaz organiza la demostración alrededor de un plano técnico de almacén. El fondo claro contiene títulos, controles y datos. El plano oscuro concentra el movimiento, las rutas y los hitos. El color identifica tipos de robot y estados de espera. La silueta y el texto aportan referencias adicionales para distinguirlos.

## Paleta y reglas de uso

| Familia | Colores | Uso |
|---|---|---|
| Superficies | `#F4F5F8` y `#FFFFFF` | Fondo de escritorio y paneles, respectivamente |
| Tinta | `#182638` y `#46566D` | Títulos y texto de apoyo |
| Acción | `#3153A4` | Controles primarios y navegación |
| Plano | `#172334` y `#304259` | Suelo y celdas |
| Flota | `#8DCCFF` y `#D5B9FF` | Robot ligero y pesado, y sus rutas |
| Mercancía | `#FFD275` | Paquete en tránsito y marcador de carga |
| Energía | `#78E4CD` | Estación de recarga y pulsos de energía |
| Espera y bloqueo | `#FFB079` y `#FF9DAA` | Contención y bloqueo, acompañados de texto |
| Operaciones | `#9CDCEB` y `#ADDDA3` | Recepción y entrega |

El color se combina con dirección, pictograma y texto. Un cuadrado dorado viaja hacia el robot al recoger y sale de él al dejar una carga. La energía se representa con anillos y un pulso final. Los avisos destacan el resultado junto al lugar en que ocurrió.

## Composición y responsabilidades

- `visual_system` define la paleta semántica, la escala de texto, los estilos de controles y la leyenda compartida. Su catálogo reúne los títulos y objetivos de los cuatro escenarios.
- `ScenarioHub` compone el selector y la vista previa. `MainWindow` actúa como controlador de la sesión mediante señales de Qt y entrega instantáneas a `WarehouseView`, `RobotMonitor` y `StatisticsPanel`. Las vistas presentan el estado del motor y notifican acciones del usuario.
- `assets/icons` contiene los símbolos SVG originales sobre una retícula de 24 unidades. Qt los empaqueta como recursos. La misma familia se usa en los controles, el plano y la leyenda.

## Guías aplicadas

- Dirección industrial, jerarquía tipográfica y revisión por capturas en [Frontend Design](https://github.com/anthropics/skills/tree/main/skills/frontend-design).
- Composición de vistas y tokens semánticos en [Building Components](https://github.com/vercel/components.build/tree/main/skills/building-components).
- Contraste, foco visible, controles nativos y movimiento reducido en [Web Design Guidelines](https://github.com/vercel-labs/agent-skills/tree/main/skills/web-design-guidelines), adaptada a Qt Widgets.

Se puede capturar cualquier escenario con `./build/simra --screenshot-demo 0 captura.png`, usando índices de 0 a 3 en el orden del selector.
