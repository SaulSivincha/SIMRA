# Límites conocidos

- La detección de bloqueo operativo reconoce esperas mutuas de dos robots y espera prolongada. Replanifica rutas con ocupación dinámica, pero no implementa búsqueda formal de refugio ni garantiza romper todos los ciclos.
- El editor cambia elementos por clic, pero no expone aún formularios para dimensiones, capacidad, peso o batería. Esos campos se pueden cambiar en JSON y se validan al cargar o iniciar.
- La comparación de la interfaz es una receta fija del escenario básico con 2 y 3 robots, tres pedidos al inicio y horizonte de 8 segundos. La semilla se conserva en la configuración, pero no determina el orden de ejecución de los hilos. La salida varía entre corridas; el archivo de evidencia contiene una sola observación.
- Las estaciones de entrega y recepción tienen ocupación física exclusiva. La reserva lógica anticipada se implementa para recarga; no se implementa aún para esos dos tipos de estación.
- Las estadísticas muestran tiempo real y tiempo activo sin pausas. La atención media de pedidos usa tiempo real monotónico y puede incluir una pausa; se etiqueta como tal. No hay utilización porcentual ni exportación CSV.
- Los cargadores se usan a capacidad 100 %. No hay control de velocidad en la GUI. El algoritmo no simula fallos de hardware ni movimiento físico.
- La validación comprueba geometría, IDs, capacidad, estaciones compatibles y accesibilidad desde al menos un robot. No prueba de manera exhaustiva que toda combinación futura de pedidos y posiciones de robots sea resoluble.
