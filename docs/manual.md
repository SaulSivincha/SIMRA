# Manual de usuario

## Iniciar

Compila con los comandos del README y abre `./build/simra`. En Inicio elige uno de los cuatro escenarios: **Hilos y pedidos concurrentes**, **Herencia y polimorfismo**, **Mutex: pasillo compartido** o **Sincronización: recarga compartida**. Cada tarjeta lista los conceptos que se estudian antes de abrir el plano.

- **Hilos y pedidos concurrentes** combina reposición y entregas con prioridades. Los pedidos pendientes se reflejan en el buffer mientras los robots atienden los asignados.
- **Herencia y polimorfismo** combina cargas de 5 y 25 kg. El aviso de asignación muestra el peso y la capacidad del robot seleccionado.
- **Mutex: pasillo compartido** incorpora seis entregas a una misma estación y una reposición. Los avisos señalan la espera, la reserva y la liberación del recurso. Los ciclos dependen de la interacción de los hilos.
- **Sincronización: recarga compartida** parte de tres robots que necesitan usar la misma estación. Tras los turnos de recarga, la flota continúa con entregas y reposición.

## Simular

**Iniciar demostración** carga los pedidos del escenario y crea un hilo por robot. El reloj de las demostraciones funciona al 38 % de la velocidad normal. **Pausar** congela el avance y el desplazamiento visual. **Reanudar** continúa desde el mismo punto. **Escenarios** detiene y une los hilos antes de volver al selector.

La vista interpola los movimientos confirmados por el motor a 30 FPS. Los avisos flotantes señalan al robot implicado mediante un contorno y un conector. Duran hasta seis segundos y conservan un mínimo de lectura de aproximadamente 2,5 segundos cuando llegan otros hitos. En Estadísticas, el buffer cuenta los pedidos pendientes de asignar. **Reiniciar demostración** se habilita cuando todos los pedidos terminan y las estaciones quedan libres.

La línea punteada de cada robot muestra el trayecto pendiente y sigue su posición animada. Durante la espera por una estación, señala el destino previsto mientras se obtiene la reserva. Selecciona un robot en el panel lateral para destacar su recorrido.

Los símbolos del plano coinciden con los de la leyenda. El borde de una estación se hace más grueso durante su reserva. Usa **Tab** para recorrer los controles, **Espacio** para activar un botón y las flechas para moverte entre pestañas. Para reducir el movimiento visual, inicia la aplicación con `./build/simra --reduced-motion`.
