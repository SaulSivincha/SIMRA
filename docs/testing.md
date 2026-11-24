# Pruebas y evidencia

Entorno observado: GCC 16.2.1, CMake 4.5.1, Qt Widgets 6.12.0 en Linux. Comandos ejecutados:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Resultado observado: 2/2 pruebas aprobadas. `simra_tests` cubre Position, Matrix, BFS, compatibilidad de familias, Factory Method, Builder, Prototype, clon independiente, asignación concurrente, cola, logger multihilo, JSON roundtrip y validación de los cuatro escenarios. `simra_integration` coordina dos movimientos a una celda, verifica reserva exclusiva y liberación de cargador, pausa sin avance, entrega y reposición concurrentes con custodia final y generación automática real.

ASan/UBSan:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-asan -j4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan --output-on-failure
```

Resultado observado: 2/2 aprobadas. LeakSanitizer con `detect_leaks=1` falló por la restricción `ptrace` del entorno antes de producir un diagnóstico del programa; por eso se desactivó solo esa comprobación. No se afirma ausencia absoluta de carreras: TSan no se ejecutó aquí.

La [captura real](screenshot-simulation.png) se generó con el ejecutable Qt, no con las maquetas. [La comparación observada](comparison-observed.txt) es una salida auténtica de `simra_compare`; sus números son una muestra y no un benchmark generalizable.
