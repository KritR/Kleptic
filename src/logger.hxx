#ifndef KLEPTIC_LOGGER_HXX_
#define KLEPTIC_LOGGER_HXX_

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "sysv_ipc.hxx"

#define LOGGER_ID 'N'

namespace Kleptic {

class LogEvent {
 protected:
  bool completed = false;
  const bool deserialized = false;
  std::chrono::time_point<std::chrono::system_clock> start_t;
  std::chrono::time_point<std::chrono::system_clock> end_t;
  std::vector<std::function<void(LogEvent &)>> on_complete_listeners;
  std::vector<std::function<void(LogEvent &)>> on_start_listeners;

 public:
  std::map<std::string, std::string> str_data;
  std::map<std::string, double> num_data;
  std::string ev_name = "GENERIC_EVENT";
  LogEvent() = default;
  explicit LogEvent(std::stringstream ss);

  void on_start(std::function<void(LogEvent &)> listener);
  void on_complete(std::function<void(LogEvent &)> listener);

  void start();
  void end();

  bool is_complete();
  int64_t get_duration();
  auto get_start_time();
  auto get_end_time();

  operator std::string() const {
    if (deserialized) {
      return str_data.find("EVT_STRING")->second;
    } else {
      return "GENERIC_EVENT_STR";
    }
  }
  inline std::string get_name() const { return ev_name; }

  std::stringstream serialize();
  // HOW DOES EVENT SERIALIZATION WORK
  //
  // EVT NAME
  // EVT_START=INT
  // EVT_END=INT
  // OTHER_EVT_DATA ( number / str )
  // EVT_STRING="STR"
};

typedef std::shared_ptr<LogEvent> log_event_t;
typedef std::function<void(LogEvent &)> log_event_listener;

class Logger {
  enum IPC_MSG_TYPES { RECORD = 1, EVT_START = 2, EVT_END = 3, GET_STR = 4, GET_NUM = 5 };

 protected:
  int ipc_id = 0;
  pid_t _origin_pid;

  int log_fd = 0;
  const std::string logfile;

  std::vector<log_event_listener> event_start_listeners;
  std::vector<log_event_listener> event_end_listeners;

  std::map<std::string, double> num_data;
  std::map<std::string, std::string> str_data;

  std::mutex str_data_mut;
  std::mutex num_data_mut;

  std::mutex file_mut;

  void handle_ipc(std::tuple<int, int64_t, std::string> client_req);

 public:
  explicit Logger(std::string p = "");
  ~Logger() = default;
  void on_ev_start(log_event_listener listener);
  void on_ev_end(log_event_listener listener);

  void start_mp();

  // TODO CREATE EXCEPTIONS FOR THESE

  // can only be done in original process
  // typedef std::function<void(std::map<std::string, double>&)> num_data_accessor;
  template <typename F>
  void with_num_data(F fn) {
    if (_origin_pid != getpid()) {
      throw "BAD ACCESS OF LOGGER";
    }
    std::lock_guard<std::mutex> l(num_data_mut);
    fn(std::ref(num_data));
  }

  // typedef std::function<void(std::map<std::string, std::string>&)> str_data_accessor;
  template <typename F>
  void with_str_data(F fn) {
    if (_origin_pid != getpid()) {
      throw "BAD ACCESS OF LOGGER";
    }
    std::lock_guard<std::mutex> l(str_data_mut);
    fn(std::ref(str_data));
  }

  double get_num_data(std::string key);

  std::string get_str_data(std::string key);

  template <class E, typename... Args>
  std::shared_ptr<E> create_event(Args &&... args) {
    auto event = std::make_shared<E>(std::forward<Args>(args)...);

    event->on_start([this](LogEvent &ev) {
      if (getpid() != _origin_pid) {
        broadcast_ipc(EVT_START, ev.serialize().str());
        return;
      }
      for (const auto &listener : event_start_listeners) {
        listener(ev);
      }
    });

    event->on_complete([this](LogEvent &ev) {
      if (getpid() != _origin_pid) {
        broadcast_ipc(EVT_END, ev.serialize().str());
        return;
      }
      for (const auto &listener : event_end_listeners) {
        listener(ev);
      }
    });

    return event;
  }

  void broadcast_ipc(int64_t msg_type, std::string);

  void record(std::string s);
};
}  // namespace Kleptic

#endif  // KLEPTIC_LOGGER_HXX_
