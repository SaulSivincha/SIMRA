# Contexto y trazabilidad

El curso pide un proyecto original orientado a objetos que reúna POO, patrones, GUI, programación genérica y multihilo. SIMRA usa un almacén virtual porque varios robots heterogéneos compiten por recursos limitados. La originalidad propuesta es hacer visibles esas decisiones de coordinación para sustentarlas, no afirmar que el simulador de almacenes sea una idea inédita.

| ID | Prioridad | Implementación | Evidencia |
|---|---|---|---|
| RF01, RF05 | P0 | `Matrix`, `bfs`, `WarehouseState::move` | `simra_tests`, `simra_integration` |
| RF02, RF06 | P0 | `Robot`, dos familias de equipo y reserva de cargador | pruebas de capacidad y recarga |
| RF03, RF04 | P0 | `OrderCreator`, `assign`, `pickUp`, `deliver` | entrega/reposición concurrentes, una asignación |
| RF07, RF08 | P0 | `MainWindow`, `SimulationEngine` | prueba Qt a 1280 × 720, pausa y parada en integración |
| RF09 | P0 | `SimulationLogger` | prueba multihilo de 80 eventos |
| RF10, RF11 | P1 | editor e inspector, JSON v1, `clone` | validación de cuatro escenarios y roundtrip |
| RF12, RF13 | P1 | generador `std::thread`, comparador con envejecimiento | pruebas de generación y orden |
| RF14, RF15 | P1 | detección recíproca, cesión y tabla de hilos y reservas | integración y monitor Qt |
| RF16, RF17 | P1 | métricas transaccionales, `simra_compare`, cuatro JSON | salida de comparación y CTest |

RF18/P2 permanece fuera de esta versión. Los ajustes y límites de cada cobertura se declaran en [limitations.md](limitations.md).
