#include "concurrency.hxx"

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

#include <thread>

namespace Kleptic::Concurrency {

void SingleRunner::dispatch(task_t task) { task(); }

extern "C" void handle_zombie(int) {
  pid_t pid;
  while ((pid = waitpid(-1, 0, WNOHANG)) > 0) {}
}

void register_sigchld_handler() {
  struct sigaction zombie_handler;
  zombie_handler.sa_handler = handle_zombie;
  sigemptyset(&zombie_handler.sa_mask);
  zombie_handler.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &zombie_handler, NULL)) {
    perror("Zombie Handler Error");
    exit(2);
  }
}

void ThreadRunner::dispatch(task_t task) {
  std::thread t(std::move(task));
  t.detach();
}

ForkRunner::ForkRunner() {
  static bool handler_registered = false;
  if (!handler_registered) {
    register_sigchld_handler();
    handler_registered = true;
  }
}

void ForkRunner::dispatch(task_t task) {
  pid_t pid = fork();
  if (pid == 0) {
    // child run task & exit
    task();
    exit(0);
  } else if (pid > 0) {
    // parent return
    return;
  } else {
    // perror("fork");
    // was gonna make it an error ... but we might as well just block
    // handle it ourselves until we can fork again
    task();
  }
}

ThreadPoolRunner::ThreadPoolRunner(int worker_threads) : q(worker_threads) {
  for (int i = 0; i < worker_threads; ++i) {
    std::thread t([this] {
      while (1) {
        (q.pop())();
      }
    });
    t.detach();
  }
}
void ThreadPoolRunner::dispatch(task_t task) { q.push(std::move(task)); }
}  // namespace Kleptic::Concurrency
