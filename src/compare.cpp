#include "simra/core.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <chrono>
#include <iostream>
#include <thread>
using namespace simra;
static Metrics run(Scenario scenario) {
  SimulationEngine engine(std::move(scenario));
  DeliveryOrderCreator delivery;
  RestockOrderCreator restock;
  engine.state().addOrder(delivery.create("O1", "K01", "D01", 1));
  engine.state().addOrder(delivery.create("O2", "K02", "D01", 1));
  engine.state().addOrder(restock.create("O3", "K_IN", "S04", 1));
  engine.start();
  auto end = std::chrono::steady_clock::now() + std::chrono::seconds(8);
  std::this_thread::sleep_until(end);
  engine.stop();
  return engine.state().snapshot().metrics;
}
int main(int argc, char **argv) {
  try {
    bool json = argc == 3 && std::string(argv[1]) == "--json";
    if (argc != 2 && !json)
      throw std::invalid_argument("usage: simra_compare [--json] basic.json");
    auto two = loadScenario(argv[json ? 2 : 1]);
    auto three = two;
    three.robots.push_back({"R03", RobotKind::Light, {0, 4}, 100});
    three.validate();
    auto a = run(two), b = run(three);
    if (json) {
      auto entry = [](int robots, const Metrics &m) {
        return QJsonObject{
            {"robots", robots},         {"completed", m.completed},
            {"conflicts", m.conflicts}, {"distance", m.distance},
            {"energy", m.energy},       {"activeMs", double(m.activeMs)}};
      };
      QJsonObject result{{"seed", two.seed},
                         {"orderCount", 3},
                         {"horizonSeconds", 8},
                         {"runs", QJsonArray{entry(2, a), entry(3, b)}}};
      std::cout
          << QJsonDocument(result).toJson(QJsonDocument::Compact).toStdString()
          << '\n';
      return 0;
    }
    std::cout << "Misma carga: O1/K01, O2/K02, O3/K_IN; mismo horizonte: 8 s; "
                 "semilla: "
              << two.seed << "\n";
    std::cout << "Robots | Completados | Conflictos | Distancia | Energía | "
                 "Tiempo activo (ms)\n";
    std::cout << "2 | " << a.completed << " | " << a.conflicts << " | "
              << a.distance << " | " << a.energy << " | " << a.activeMs << "\n";
    std::cout << "3 | " << b.completed << " | " << b.conflicts << " | "
              << b.distance << " | " << b.energy << " | " << b.activeMs << "\n";
    std::cout << "El orden de los hilos puede variar entre ejecuciones.\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
