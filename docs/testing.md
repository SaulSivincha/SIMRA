# Pruebas y evidencia

Entorno observado: GCC 16.2.1, CMake 4.5.1, Qt Widgets 6.12.0 en Linux. Comandos ejecutados:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Resultado observado en Release: 3/3 pruebas aprobadas. `simra_tests` cubre Position, Matrix, BFS, compatibilidad de familias, Factory Method, Builder, Prototype, clon independiente, asignación concurrente, cola, logger multihilo, JSON roundtrip y validación de los cuatro escenarios. `simra_integration` coordina dos movimientos a una celda, verifica reserva exclusiva y liberación de cargador y recepción, pausa sin avance, entrega y reposición concurrentes con custodia final y generación automática real.

`simra_gui` abre la ventana a 1280 × 720 con Qt en modo sin pantalla. Entra al editor, coloca y borra un obstáculo, comprueba el inspector, inicia la sesión, verifica dos IDs de hilo distintos, pausa, reanuda y detiene.

ASan/UBSan:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-asan -j4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan --output-on-failure
```

Resultado observado: 3/3 aprobadas. LeakSanitizer con `detect_leaks=1` falló por la restricción `ptrace` del entorno antes de producir un diagnóstico del programa; por eso se desactivó solo esa comprobación.

## ThreadSanitizer

```bash
cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-fsanitize=thread -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=thread'
cmake --build build-tsan --target simra_tests simra_integration -j3
ctest --test-dir build-tsan -R 'simra_tests|simra_integration' --output-on-failure
```

Pasaron las dos suites del motor bajo TSan. Este resultado corresponde a los casos ejecutados.

Las capturas provienen del ejecutable Qt. [Simulación](screenshot-simulation.png) · [Estadísticas](screenshot-statistics.png).
