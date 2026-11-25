#include "simra/concurrency.hpp"
#include <sstream>
namespace simra {
void SimulationLogger::log(EventKind kind, std::string text,
                           std::string robotId,
                           std::optional<EventLocation> location) {
  std::ostringstream out;
  out << std::this_thread::get_id();
  Event event{kind, std::move(text), std::move(robotId), out.str(),
              std::chrono::system_clock::now(), location};
  {
    std::lock_guard lock(mutex_);
    events_.push_back(event);
    if (events_.size() > 2000)
      events_.pop_front();
  }
  pending_.push(std::move(event));
  if (pending_.size() > 2000)
    pending_.tryPop();
}
std::vector<Event> SimulationLogger::events() const {
  std::lock_guard lock(mutex_);
  return {events_.begin(), events_.end()};
}

} // namespace simra
