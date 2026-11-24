# Manual de usuario

## Iniciar

Compila con los comandos del README y abre `./build/simra`. En Inicio elige el almacén básico, la flota mixta, la congestión o la competencia por recarga. **Cargar escenario** cambia de plantilla desde el área de trabajo. **Abrir JSON** carga una configuración propia; se rechaza si tiene IDs repetidos, posiciones inválidas, carga sin ubicación o estación incompatible.

## Editar y guardar

Antes de iniciar, pulsa **Editar mapa**, elige una herramienta y haz clic en una celda. Clic derecho borra el elemento y su mercancía asociada. **Guardar JSON** valida antes de escribir. **Clonar** produce una configuración independiente para modificar. Con **Seleccionar** puedes cambiar la capacidad de un estante o la batería de un robot. Usa **Mercancía** sobre un estante o una recepción para añadir inventario, o haz doble clic en la tabla Inventario para cambiar el peso. **Configurar** ajusta dimensiones, semilla e intervalo. **Validar** revisa el escenario. Para editar una sesión iniciada, primero detenla.

## Simular

**Iniciar demostración** crea un hilo por robot y solicita una entrega y una reposición con la mercancía disponible. **Nuevo pedido** pregunta mercancía, destino de reposición cuando corresponde y prioridad de 0 a 3. Una carga con pedido activo se rechaza. La demostración utiliza la velocidad normal. El motor mantiene la generación automática y la velocidad variable para experimentos mediante su API. **Pausar** congela movimientos y generación; **Reanudar** prosigue. **Detener** espera a que terminen los hilos y conserva el snapshot final. **Iniciar** de nuevo crea una ejecución limpia desde el escenario inicial.

El mapa usa azul para ligero, violeta para pesado, marrón para estantes, ámbar para recarga, verde para entrega y cian para recepción. Selecciona un robot en su tarjeta o en la tabla Concurrencia para ver la ruta disponible en línea discontinua. Concurrencia muestra el ID de cada hilo, el recurso esperado y las reservas de estación. Estadísticas se calculan con transacciones del motor; los eventos tienen hora e ID de hilo.

## Comparar

**Herramientas → Comparar 2/3** ejecuta sin bloquear la ventana dos corridas de la receta básica con la misma carga de pedidos y el mismo horizonte. Consulta la pestaña Comparación. El resultado puede cambiar por planificación de hilos. Para reproducir en terminal: `./build/simra_compare scenarios/basic.json`.

Las operaciones de configuración, archivos y pedidos manuales están en el menú Herramientas. El recorrido principal consiste en elegir la demostración, iniciarla y observar el plano.
