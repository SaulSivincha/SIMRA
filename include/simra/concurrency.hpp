#pragma once
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>
namespace simra {
template <class T> class ThreadSafeQueue {
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<T> items_;
  bool closed_{};

public:
  bool push(T item) {
    {
      std::lock_guard lock(mutex_);
      if (closed_)
        return false;
      items_.push_back(std::move(item));
    }
    cv_.notify_one();
    return true;
  }
  std::optional<T> waitPop() {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [&] { return closed_ || !items_.empty(); });
    if (items_.empty())
      return {};
    T item = std::move(items_.front());
    items_.pop_front();
    return item;
  }
  std::optional<T> tryPop() {
    std::lock_guard lock(mutex_);
    if (items_.empty())
      return {};
    T item = std::move(items_.front());
    items_.pop_front();
    return item;
  }
  size_t size() const {
    std::lock_guard lock(mutex_);
    return items_.size();
  }
  void close() {
    {
      std::lock_guard lock(mutex_);
      closed_ = true;
    }
    cv_.notify_all();
  }
};

struct Event {
  std::string text, robotId, threadId;
  std::chrono::system_clock::time_point when;
};
class SimulationLogger {
  mutable std::mutex mutex_;
  std::deque<Event> events_;
  ThreadSafeQueue<Event> pending_;
  SimulationLogger() = default;

public:
  SimulationLogger(const SimulationLogger &) = delete;
  SimulationLogger &operator=(const SimulationLogger &) = delete;
  static SimulationLogger &instance() {
    static SimulationLogger value;
    return value;
  }
  void log(std::string text, std::string robotId = {});
  std::vector<Event> events() const;
  std::optional<Event> tryNextEvent() { return pending_.tryPop(); }
};

} // namespace simra
