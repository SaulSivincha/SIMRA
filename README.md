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

1. Elige una demostración desde Inicio.
2. Pulsa **Iniciar demostración**. Se preparan una entrega y una reposición con la mercancía disponible.
3. Observa el desplazamiento, la ruta discontinua, la caja amarilla y la batería de cada robot. La explicación sobre el mapa indica el concepto del escenario.
4. Usa **Pausar/Reanudar** para examinar un estado. Vuelve a **Inicio** para elegir otra demostración; la ejecución anterior termina de forma segura.


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

P0 tiene núcleo funcional: dos tipos de robot, hilos reales, entrega y reposición, inventario, BFS, ocupación exclusiva, batería/recarga, controles, cinco patrones y pruebas. P1 incluye cuatro escenarios guiados, prioridades con envejecimiento, generación automática, detección de espera mutua, maniobra de cesión, monitor, métricas y comparación controlada por terminal. Las limitaciones exactas están en [docs/limitations.md](docs/limitations.md); P2 queda sin implementar.

## Capturas

[Inicio](docs/screenshot-home.png) · [Simulación](docs/screenshot-simulation.png) · [Estadísticas](docs/screenshot-statistics.png) · [Comparación](docs/screenshot-comparison.png)
