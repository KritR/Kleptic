#ifndef KLEPTIC_CONCURRENCY_HXX_
#define KLEPTIC_CONCURRENCY_HXX_

#include <functional>
#include <future>
#include <memory>

#include "bqueue.hxx"

namespace Kleptic::Concurrency {

enum ConcurrencyMode { THR_SINGLE, THR_POOL, THR_PER_REQ, FORK_PER_REQ };

typedef std::packaged_task<void()> task_t;

class ConcurrentRunner {
 public:
  virtual void dispatch(task_t) = 0;
  virtual ~ConcurrentRunner() = default;
};

typedef std::unique_ptr<ConcurrentRunner> runner_t;

class SingleRunner : public ConcurrentRunner {
 public:
  SingleRunner() = default;
  void dispatch(task_t) override;
  ~SingleRunner() = default;
};

class ThreadRunner : public ConcurrentRunner {
 public:
  ThreadRunner() = default;
  void dispatch(task_t) override;
  ~ThreadRunner() = default;
};

class ThreadPoolRunner : public ConcurrentRunner {
 protected:
  BlockingQueue<task_t> q;

 public:
  explicit ThreadPoolRunner(int worker_threads = std::thread::hardware_concurrency());
  void dispatch(task_t) override;
  ~ThreadPoolRunner() = default;
};

class ForkRunner : public ConcurrentRunner {
 public:
  ForkRunner();
  void dispatch(task_t) override;
  ~ForkRunner() = default;
};

}  // namespace Kleptic::Concurrency

#endif  // KLEPTIC_CONCURRENCY_HXX_
