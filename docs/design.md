# Diseño visual de SIMRA

La interfaz organiza la demostración alrededor de un plano técnico de almacén. El selector distribuye los cuatro escenarios a lo largo de una columna. A su lado se muestran el plano del escenario enfocado, las dimensiones, la cantidad de robots y la leyenda. Los fondos oscuros de la ventana, el plano y el panel lateral pertenecen a la misma escala.

## Paleta y reglas de uso

| Familia | Colores | Uso |
|---|---|---|
| Superficies | `#111217` y `#181B20` | Fondo de escritorio y paneles |
| Tinta | `#F4F4F4` y `#BEC5D0` | Títulos y texto de apoyo |
| Acción | `#0F62FE` | Control primario y selección |
| Plano | `#13161C` y `#313846` | Suelo y celdas |
| Flota | `#78A9FF` y `#D4BBFF` | Robot ligero y pesado, sus rutas e iconos |
| Mercancía | `#F1C21B` | Paquete en tránsito y marcador de carga |
| Energía | `#3DDBD9` | Estación de recarga, batería en carga y pulsos |
| Espera y bloqueo | `#FFB784` y `#FF8389` | Contención y bloqueo, acompañados de texto |
| Operaciones | `#9CDCEB` y `#ADDDA3` | Recepción y entrega |

El color se combina con dirección, pictograma y texto. Un cuadrado dorado viaja hacia el robot al recoger y sale de él al dejar una carga. La energía se representa con anillos y un pulso final. Los avisos destacan el resultado junto al lugar en que ocurrió.

Las capas de superficie y el azul de acción siguen la lógica de [IBM Carbon](https://www.carbondesignsystem.com/building-blocks/foundations/color/overview). El contraste moderado entre paneles y el uso localizado de acentos se inspiran en [Grafana](https://grafana.com/developers/plugin-tools/migration-guides/visual-design-refresh).

## Composición y responsabilidades

- `visual_system` define la paleta semántica, la escala de texto, los estilos de controles y la leyenda compartida. Su catálogo reúne los títulos y objetivos de los cuatro escenarios.
- `ScenarioHub` compone el selector y la vista previa. `MainWindow` actúa como controlador de la sesión mediante señales de Qt y entrega instantáneas a `WarehouseView`, `RobotMonitor` y `StatisticsPanel`. Las vistas presentan el estado del motor y notifican acciones del usuario.
- `assets/icons` contiene los símbolos SVG originales sobre una retícula de 24 unidades. Qt los empaqueta como recursos. La misma familia se usa en los controles, el plano y la leyenda.

## Guías aplicadas

- Dirección industrial, jerarquía tipográfica y revisión por capturas en [Frontend Design](https://github.com/anthropics/skills/tree/main/skills/frontend-design).
- Composición de vistas y tokens semánticos en [Building Components](https://github.com/vercel/components.build/tree/main/skills/building-components).
- Contraste, foco visible, controles nativos y movimiento reducido en [Web Design Guidelines](https://github.com/vercel-labs/agent-skills/tree/main/skills/web-design-guidelines), adaptada a Qt Widgets.

Se puede capturar cualquier escenario con `./build/simra --screenshot-demo 0 captura.png`, usando índices de 0 a 3 en el orden del selector.
