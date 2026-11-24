# SIMRA

**Simulador Interactivo Multihilo de Robots de Almacén** para Tecnología de Objetos, Ingeniería de Sistemas, UNSA.

SIMRA permite observar cómo robots ligeros y pesados compiten por pedidos, celdas y estaciones de recarga. El problema reúne polimorfismo, patrones creacionales, programación genérica, sincronización y una interfaz de escritorio que muestra el estado real de los trabajadores. El alcance es académico: no pretende modelar física ni optimización industrial.

## Compilación

Requiere C++20, CMake 3.21+, Qt 6 Core/Widgets y un compilador con `std::thread`. En Arch Linux: `sudo pacman -S cmake gcc qt6-base`.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/simra
```

En algunos escritorios KDE, la captura sin pantalla requiere `QT_QPA_PLATFORMTHEME= QT_QPA_PLATFORM=offscreen ./build/simra --screenshot-demo docs/screenshot-simulation.png`.

## Uso rápido

1. Abre una de las cuatro plantillas desde Inicio. En el área de trabajo puedes cambiar de plantilla con **Cargar escenario**.
2. Pulsa **Ejecutar ejemplo** para probar una entrega y una reposición. También puedes pulsar **Iniciar** y usar **Nuevo pedido** para escoger mercancía, destino y prioridad. **Auto: sí** crea solicitudes desde un hilo propio a la cadencia del JSON.
3. Observa la cuadrícula, las tablas de pedidos y robots, los IDs de hilo, eventos y estadísticas. Ajusta velocidad de `0.25×` a `4×`. **Pausar/Reanudar** detiene los avances lógicos; **Detener** une los hilos.
4. **Editar mapa** permite poner, inspeccionar o borrar elementos por clic. **Configurar** ajusta el tamaño y la generación automática. **Guardar JSON**, **Abrir JSON** y **Clonar** trabajan con la configuración, nunca con mutex o hilos.
5. **Comparar 2/3** ejecuta en segundo plano dos experimentos del escenario básico, con los mismos tres pedidos iniciales y ocho segundos de horizonte. El resultado aparece en la pestaña Comparación.

El ejecutable de comparación también se puede usar en terminal:

```bash
./build/simra_compare scenarios/basic.json
```

## Estructura

- `simra_core`: dominio, patrones, planificación BFS, estado compartido, motor y edición de escenarios en C++20.
- `simra_json`: lectura y guardado de configuraciones, con validación y escritura atómica.
- `simra_widgets`: inicio, plano del almacén, robots, concurrencia, estadísticas y comparación.
- `scenarios/`: básico, mixto, congestión y competencia energética.
- `tests/`: pruebas unitarias y de integración concurrente.
- `docs/`: arquitectura, requisitos, patrones, concurrencia, pruebas, manual y evidencia.

La especificación original y los doce PDF del curso se consultaron localmente. Se excluyen del repositorio con `.gitignore` porque el material docente no se debe publicar automáticamente. El código no incorpora una licencia por defecto.

## Estado de alcance

P0 tiene núcleo funcional: dos tipos de robot, hilos reales, entrega y reposición, inventario, BFS, ocupación exclusiva, batería/recarga, controles, cinco patrones y pruebas. P1 incluye editor básico, cuatro escenarios, JSON, clonación, prioridades con envejecimiento, generación automática, detección de espera mutua, maniobra de cesión, monitor, métricas y comparación controlada. Las limitaciones exactas están en [docs/limitations.md](docs/limitations.md); P2 queda sin implementar.

## Capturas

[Inicio](docs/screenshot-home.png) · [Editor](docs/screenshot-editor.png) · [Simulación](docs/screenshot-simulation.png) · [Estadísticas](docs/screenshot-statistics.png) · [Comparación](docs/screenshot-comparison.png)
