# Límites conocidos

- La detección de bloqueo operativo reconoce esperas mutuas de dos robots y espera prolongada. El robot de mayor ID intenta ceder a una celda libre desde la que aún existe ruta; todo movimiento pasa por la transacción normal. Esta heurística no garantiza romper ciclos mayores ni toda congestión.
- La interfaz se limita a demostraciones guiadas; no expone edición manual de mapas, configuración ni importación de escenarios.
- Las estaciones de entrega, recepción y recarga usan reserva lógica y ocupación exclusiva. Una reserva puede mantenerse durante una espera prolongada del robot propietario hasta que este salga o se detenga la sesión; no hay expiración automática de reservas.
- Las estadísticas se enfocan en indicadores de concurrencia y recursos. No hay utilización porcentual por estación ni exportación CSV.
- Los cargadores se usan a capacidad 100 %. El reloj lógico adapta las esperas pendientes al cambio de velocidad y conserva el tiempo restante durante una pausa. El algoritmo no simula fallos de hardware ni movimiento físico.
- La validación comprueba geometría, IDs, capacidad, estaciones compatibles y accesibilidad de los recursos y acceso de cada robot a un cargador de su familia. No prueba de manera exhaustiva que toda combinación futura de pedidos y posiciones de robots sea resoluble.
