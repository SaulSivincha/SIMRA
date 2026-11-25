# Diseño visual de SIMRA

La interfaz organiza la demostración alrededor de un plano técnico de almacén. El fondo claro contiene títulos, controles y datos. El plano oscuro concentra el movimiento, las rutas y los hitos. El color identifica tipos de robot y estados de espera. La silueta y el texto aportan referencias adicionales para distinguirlos.

## Composición y responsabilidades

- `visual_system` define la paleta semántica, la escala de texto, los estilos de controles y la leyenda compartida. Su catálogo reúne los títulos y objetivos de los cuatro escenarios.
- `ScenarioHub` compone el selector y la vista previa. `MainWindow` actúa como controlador de la sesión mediante señales de Qt y entrega instantáneas a `WarehouseView`, `RobotMonitor` y `StatisticsPanel`. Las vistas presentan el estado del motor y notifican acciones del usuario.
- `assets/icons` contiene los símbolos SVG originales sobre una retícula de 24 unidades. Qt los empaqueta como recursos. La misma familia se usa en los controles, el plano y la leyenda.

## Guías aplicadas

- Dirección industrial, jerarquía tipográfica y revisión por capturas en [Frontend Design](https://github.com/anthropics/skills/tree/main/skills/frontend-design).
- Composición de vistas y tokens semánticos en [Building Components](https://github.com/vercel/components.build/tree/main/skills/building-components).
- Contraste, foco visible, controles nativos y movimiento reducido en [Web Design Guidelines](https://github.com/vercel-labs/agent-skills/tree/main/skills/web-design-guidelines), adaptada a Qt Widgets.

Se puede capturar cualquier escenario con `./build/simra --screenshot-demo 0 captura.png`, usando índices de 0 a 3 en el orden del selector.
