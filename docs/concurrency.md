# Protocolo de concurrencia

El hilo principal de Qt solo invoca métodos públicos del motor y renderiza snapshots. Los trabajadores no reciben punteros a widgets. `WarehouseState` es el único propietario del mutex que protege las operaciones compuestas. BFS y el tiempo de movimiento ocurren fuera de la sección crítica.

```mermaid
sequenceDiagram
  participant R1 as Robot 1
  participant R2 as Robot 2
  participant S as WarehouseState
  R1->>S: move(celda B)
  Note over S: lock; comprobar libre; ocupar B; liberar A; unlock
  S-->>R1: éxito
  R2->>S: move(celda B)
  Note over S: lock; B ocupada; registrar espera; unlock
  S-->>R2: rechazo
  R2->>S: waitChange(versión, timeout)
  S-->>R2: notificación o timeout
  R2->>S: snapshot y nuevo BFS
```

`waitChange` y `waitStep` usan `std::unique_lock` y predicados ligados a la misma condición de pausa/parada. `sleep_until` se usa únicamente en el programa de comparación, fuera de los trabajadores. `get_id` produce los IDs reales mostrados en el monitor. `joinable()` evita unir un hilo inexistente. `std::thread::hardware_concurrency()` no se usa para limitar robots porque el escenario académico fija 1–12 y ese valor puede ser cero. Los trabajadores permanentes nunca se separan con `detach`: hacerlo permitiría destruir el estado todavía usado por ellos.

El logger tiene otro mutex, pero se llama fuera de las transacciones del almacén; no existe ruta de adquisición anidada de ambos locks. Los conflictos son rechazos por celda ocupada. Un ciclo registrado es una espera recíproca de dos robots, no un deadlock de mutex. La ruta dinámica y los reintentos pueden encontrar una alternativa; si no, la UI informa bloqueo operativo y la parada sigue siendo posible.
