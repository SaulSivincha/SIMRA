# Manual de usuario

## Iniciar

Compila con los comandos del README y abre `./build/simra`. Usa la lista superior para seleccionar **basic**, **mixed**, **congestion** o **charging**. **Cargar escenario** reemplaza la configuración actual por un ejemplo validado. **Abrir JSON** carga una configuración propia; se rechaza si tiene IDs repetidos, posiciones inválidas, carga sin ubicación o estación incompatible.

## Editar y guardar

Antes de iniciar, pulsa **Editar mapa**, elige una herramienta y haz clic en una celda. Clic derecho borra el elemento y su mercancía asociada. **Guardar JSON** valida antes de escribir. **Clonar** produce una configuración independiente para modificar. Los cambios estructurales no se admiten mientras el motor existe; cargar un escenario reinicia esa sesión.

## Simular

**Iniciar** crea un hilo por robot. **Nuevo pedido** pregunta mercancía, destino de reposición cuando corresponde y prioridad de 0 a 3. Una carga con pedido activo se rechaza. **Auto: sí** activa un hilo de solicitudes a la cadencia `generationIntervalMs` del JSON. **Pausar** congela movimientos y generación; **Reanudar** prosigue. **Detener** espera a que terminen los hilos y conserva el snapshot final. **Iniciar** de nuevo crea una ejecución limpia desde el escenario inicial.

El mapa usa azul para ligero, violeta para pesado, marrón para estantes, ámbar para recarga, verde para entrega y cian para recepción. La tabla Robots e hilos muestra batería, carga, estado operativo e ID real del hilo. Estadísticas se calculan con transacciones del motor; los eventos tienen hora e ID de hilo.

## Comparar

**Comparar 2/3** ejecuta sin bloquear la ventana dos corridas de la receta básica con la misma carga de pedidos y el mismo horizonte. Consulta la pestaña Comparación. El resultado puede cambiar por planificación de hilos. Para reproducir en terminal: `./build/simra_compare scenarios/basic.json`.
