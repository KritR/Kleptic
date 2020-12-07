#ifndef KLEPTIC_BQUEUE_HXX_
#define KLEPTIC_BQUEUE_HXX_

#include <future>
#include <queue>
#include <utility>

namespace Kleptic {

template <typename T>
class BlockingQueue {
 protected:
  std::queue<T> q;
  std::mutex _m;
  const size_t _max_size;
  std::condition_variable _full_var;
  std::condition_variable _empty_var;

 public:
  explicit BlockingQueue(int max_cap) : _max_size(max_cap) {}

  T pop() {
    std::unique_lock<std::mutex> l(_m);
    _empty_var.wait(l, [this] { return !q.empty(); });
    auto item = std::move(q.front());
    q.pop();
    l.unlock();
    _full_var.notify_one();
    return std::move(item);
  }

  void push(T item) {
    std::unique_lock<std::mutex> l(_m);
    _full_var.wait(l, [this] { return (q.size() < _max_size); });
    q.push(std::move(item));
    l.unlock();
    _empty_var.notify_one();
  }

  void emplace(T item) {
    std::unique_lock<std::mutex> l(_m);
    _full_var.wait(l, [this] { return (q.size() < _max_size); });
    q.emplace(std::forward(item));
    l.unlock();
    _empty_var.notify_one();
  }
};

}  // namespace Kleptic

#endif  // KLEPTIC_BQUEUE_HXX_
