# Patrones y fundamentos del curso

La tabla vincula las diapositivas locales del docente con código que se ejecuta. Las diapositivas justifican los conceptos; las clases concretas son decisiones de SIMRA.

| Material | Concepto | Evidencia en código |
|---|---|
| `patrones01.pdf` | Clasificación y finalidad GoF | Selección de cinco patrones creacionales |
| `patrones02.pdf` | Factory Method | `OrderCreator::create` delega `make()` virtual a `DeliveryOrderCreator` o `RestockOrderCreator` |
| `patrones03.pdf` | Singleton | `SimulationLogger::instance`, copia prohibida y lock de eventos |
| `patrones04.pdf` | Builder | `ScenarioBuilder`, builders básico/industrial y `ScenarioDirector` |
| `patrones05.pdf` | Abstract Factory | `EquipmentFactory` crea familia `Robot` + `ChargingStation`; la estación comprueba compatibilidad |
| `patrones06.pdf` | Prototype | `Prototype<Scenario>::clone` devuelve copia independiente |
| `polimorfismo1.pdf` | Sobrecarga | `Position::operator+`, `operator==`, `operator<<` |
| `polimorfismo2.pdf` | Virtual/override | `Robot`, `Order` y `ChargingStation` con destructores virtuales; implementaciones concretas |
| `functor.pdf` | `operator()` | `RobotWorker`, `OrderPriorityComparator`, `RobotCompatibility` |
| `multihilo.pdf` | Hilos y exclusión | `std::thread`, mutex central, `std::lock_guard` |
| `multihilo2.pdf` | Vida y coordinación | `joinable`, `join`, `get_id`, `unique_lock`, `condition_variable` |
| `multihilo3.pdf` | Invocables | Worker functor y lambda del generador |

`Matrix<T>` abstrae una cuadrícula con acceso validado y `ThreadSafeQueue<T>` tiene `push`, `tryPop`, `waitPop`, `close` y `size`. La cola no decide asignaciones: pedidos se seleccionan por prioridad y compatibilidad bajo la transacción del estado.

Los productos de fábrica tienen comportamiento virtual real y se usan mediante punteros base. El Builder valida el producto al construirlo. El Prototype copia solo datos de configuración. El Singleton se limita al logger para no convertir todo el dominio en estado global.
