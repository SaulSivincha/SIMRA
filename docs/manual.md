# Manual de usuario

## Iniciar

Compila con los comandos del README y abre `./build/simra`. En Inicio elige uno de los cuatro escenarios: **Hilos y pedidos concurrentes**, **Herencia y polimorfismo**, **Mutex: pasillo compartido** o **Sincronización: recarga compartida**. Cada tarjeta lista los conceptos que se estudian antes de abrir el plano.

## Simular

**Iniciar demostración** crea un hilo por robot y solicita automáticamente una entrega y una reposición con la mercancía disponible. **Pausar** congela movimientos y generación; **Reanudar** prosigue. Para finalizar o cambiar de escenario usa **Inicio**: SIMRA detiene y une los hilos antes de volver a la selección. **Iniciar demostración** crea una ejecución limpia desde el escenario elegido.

El mapa usa azul para ligero, violeta para pesado, marrón para estantes, ámbar para recarga, verde para entrega y cian para recepción. Selecciona un robot en su tarjeta para ver la ruta disponible en línea discontinua. La pestaña Estadísticas muestra el estado técnico: hilos activos o en espera, buffer de trabajo, recursos exclusivos libres y reservados, conflictos de mutex, ciclos de espera, bloqueos, batería y recargas. Cuando todos los pedidos terminan, **Reiniciar demostración** crea una ejecución limpia.
