#include "simra/core.hpp"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace simra {
static Position position(QJsonValue v) {
  auto o = v.toObject();
  return {o.value("x").toInt(-1), o.value("y").toInt(-1)};
}
static QJsonObject jsonPosition(Position p) { return {{"x", p.x}, {"y", p.y}}; }
Scenario loadScenario(const std::string &filename) {
  QFile file(QString::fromStdString(filename));
  if (!file.open(QIODevice::ReadOnly))
    throw std::runtime_error("cannot open scenario");
  QJsonParseError error;
  auto document = QJsonDocument::fromJson(file.readAll(), &error);
  if (error.error != QJsonParseError::NoError || !document.isObject())
    throw std::runtime_error("invalid JSON");
  auto root = document.object();
  if (root.value("version").toInt() != 1)
    throw std::runtime_error("unsupported scenario version");
  Scenario s;
  s.name = root.value("name").toString().toStdString();
  s.width = root.value("width").toInt();
  s.height = root.value("height").toInt();
  s.seed = root.value("seed").toInt(42);
  s.generationIntervalMs = root.value("generationIntervalMs").toInt(1800);
  for (auto v : root.value("shelves").toArray()) {
    auto o = v.toObject();
    s.shelves.push_back({o.value("id").toString().toStdString(),
                         position(o.value("position")),
                         o.value("capacity").toInt()});
  }
  for (auto v : root.value("obstacles").toArray())
    s.obstacles.push_back(position(v));
  for (auto v : root.value("robots").toArray()) {
    auto o = v.toObject();
    auto type = o.value("type").toString();
    if (type != "LIGHT" && type != "HEAVY")
      throw std::runtime_error("robot type");
    s.robots.push_back({o.value("id").toString().toStdString(),
                        type == "LIGHT" ? RobotKind::Light : RobotKind::Heavy,
                        position(o.value("position")),
                        o.value("battery").toInt(-1)});
  }
  auto stations = [&](const char *key, Terrain fallback) {
    for (auto v : root.value(key).toArray()) {
      auto o = v.toObject();
      auto type = o.value("type").toString();
      auto kind = fallback;
      if (fallback == Terrain::ChargeLight) {
        if (type == "INDUSTRIAL")
          kind = Terrain::ChargeHeavy;
        else if (type != "LIGHT")
          throw std::runtime_error("charger type");
      }
      s.stations.push_back({o.value("id").toString().toStdString(), kind,
                            position(o.value("position"))});
    }
  };
  stations("chargingStations", Terrain::ChargeLight);
  stations("deliveryStations", Terrain::Delivery);
  stations("receptionStations", Terrain::Reception);
  for (auto v : root.value("cargo").toArray()) {
    auto o = v.toObject();
    s.cargo.push_back(
        {o.value("id").toString().toStdString(), o.value("weightKg").toInt(),
         o.value("location").toObject().value("id").toString().toStdString()});
  }
  s.validate();
  return s;
}
void saveScenario(const Scenario &s, const std::string &filename) {
  s.validate();
  QJsonObject root{
      {"version", 1},     {"name", QString::fromStdString(s.name)},
      {"width", s.width}, {"height", s.height},
      {"seed", s.seed},   {"generationIntervalMs", s.generationIntervalMs}};
  QJsonArray shelves, obstacles, robots, charges, deliveries, receptions, cargo;
  for (auto &x : s.shelves)
    shelves.append(QJsonObject{{"id", QString::fromStdString(x.id)},
                               {"position", jsonPosition(x.position)},
                               {"capacity", x.capacity}});
  for (auto x : s.obstacles)
    obstacles.append(jsonPosition(x));
  for (auto &x : s.robots)
    robots.append(
        QJsonObject{{"id", QString::fromStdString(x.id)},
                    {"type", x.kind == RobotKind::Light ? "LIGHT" : "HEAVY"},
                    {"position", jsonPosition(x.position)},
                    {"battery", x.battery}});
  for (auto &x : s.stations) {
    QJsonObject obj{{"id", QString::fromStdString(x.id)},
                    {"position", jsonPosition(x.position)},
                    {"capacity", 1}};
    if (x.kind == Terrain::ChargeLight || x.kind == Terrain::ChargeHeavy) {
      obj["type"] = x.kind == Terrain::ChargeLight ? "LIGHT" : "INDUSTRIAL";
      charges.append(obj);
    } else if (x.kind == Terrain::Delivery)
      deliveries.append(obj);
    else
      receptions.append(obj);
  }
  for (auto &x : s.cargo) {
    QString type = "SHELF";
    for (auto &st : s.stations)
      if (st.id == x.locationId)
        type = "RECEPTION";
    cargo.append(QJsonObject{
        {"id", QString::fromStdString(x.id)},
        {"weightKg", x.weight},
        {"location",
         QJsonObject{{"type", type},
                     {"id", QString::fromStdString(x.locationId)}}}});
  }
  root["shelves"] = shelves;
  root["obstacles"] = obstacles;
  root["robots"] = robots;
  root["chargingStations"] = charges;
  root["deliveryStations"] = deliveries;
  root["receptionStations"] = receptions;
  root["cargo"] = cargo;
  QSaveFile file(QString::fromStdString(filename));
  if (!file.open(QIODevice::WriteOnly))
    throw std::runtime_error("cannot save scenario");
  file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
  if (!file.commit())
    throw std::runtime_error("save failed");
}
} // namespace simra
