# Guion breve de sustentación

1. Presentar el problema académico: robots heterogéneos compiten por pedidos, celdas y cargadores; explicar la originalidad didáctica.
2. Ejecutar `basic`, crear una entrega y una reposición. Señalar la custodia de la carga antes y después y los IDs reales de los hilos.
3. Mostrar `WarehouseState::assign` y `move`: cálculo BFS, revalidación, mutex, ocupación exclusiva y batería. Pausar y detener para demostrar finalización cooperativa.
4. Recorrer los cinco patrones en `docs/patterns.md` y la jerarquía virtual; mostrar `Matrix<T>`, `ThreadSafeQueue<T>` y `RobotWorker::operator()`.
5. Cargar `charging` para explicar la reserva exclusiva. Consultar conflictos y ciclos instrumentados sin afirmar resolución global.
6. Usar **Comparar 2/3** y explicar carga/horizonte comunes y variabilidad de planificación. Cerrar con CTest, ASan/UBSan y límites conocidos.
