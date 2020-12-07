#include "logger.hxx"

#include <unistd.h>

#include <chrono>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include "error.hxx"

namespace Kleptic {

void LogEvent::on_start(std::function<void(LogEvent &)> listener) {
  on_start_listeners.push_back(std::move(listener));
}

void LogEvent::on_complete(std::function<void(LogEvent &)> listener) {
  on_complete_listeners.push_back(std::move(listener));
}

void LogEvent::start() {
  if (completed) {
    return;
  }
  start_t = std::chrono::system_clock::now();
  for (const auto &l : on_start_listeners) {
    l(*this);
  }
}

void LogEvent::end() {
  if (completed) {
    return;
  }
  completed = true;
  end_t = std::chrono::system_clock::now();
  for (const auto &l : on_complete_listeners) {
    l(*this);
  }
}

bool LogEvent::is_complete() { return completed; }

auto LogEvent::get_start_time() {
  if (deserialized) {
    std::time_t secsSinceEpoch = static_cast<int64_t>(num_data["EVT_START"]);
    return std::chrono::system_clock::from_time_t(secsSinceEpoch);
  }
  return start_t;
}

auto LogEvent::get_end_time() {
  if (deserialized) {
    std::time_t secsSinceEpoch = static_cast<int64_t>(num_data["EVT_END"]);
    return std::chrono::system_clock::from_time_t(secsSinceEpoch);
  }
  return end_t;
}

int64_t LogEvent::get_duration() {
  if (deserialized) {
    return static_cast<int64_t>(num_data["EVT_DUR"]);
  }
  auto dur = std::chrono::system_clock::now() - get_start_time();
  if (completed) {
    dur = get_end_time() - get_start_time();
  }
  return std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
}

std::stringstream LogEvent::serialize() {
  std::stringstream ss;
  ss << get_name() << std::endl;

  // unix epoch start time
  auto start_t_since_epoch =
      std::chrono::duration_cast<std::chrono::seconds>(start_t.time_since_epoch()).count();

  // unix epoch end time
  auto end_t_since_epoch =
      std::chrono::duration_cast<std::chrono::seconds>(end_t.time_since_epoch()).count();

  // duration in ms

  ss << "EVT_START=" << start_t_since_epoch << std::endl;
  ss << "EVT_END=" << end_t_since_epoch << std::endl;
  ss << "EVT_DUR=" << get_duration() << std::endl;
  std::string ev = (*this);
  ss << "EVT_STR="
     << "\"" << ev << "\"" << std::endl;

  for (const auto &[key, val] : str_data) {
    ss << key << "=\"" << val << "\"" << std::endl;
  }
  for (const auto &[key, val] : num_data) {
    ss << key << "=" << val << std::endl;
  }
  return ss;
}

// Deserializing constructor

LogEvent::LogEvent(std::stringstream ss) : completed(true), deserialized(true) {
  std::regex str_data_re("^\\s*(\\S+)\\s*=\\s*\"(.*)\"\\s*$");
  std::regex num_data_re("^\\s*(\\S+)\\s*=\\s*([+-]?(?:[0-9]*[.])?[0-9]+)\\s*$");
  if (!getline(ss, ev_name)) {
    throw ParseException("Logger Event has no data");
  }
  std::string line;
  std::smatch match_pair;
  while (getline(ss, line)) {
    if (std::regex_search(line, match_pair, str_data_re)) {
      auto key = match_pair[1];
      auto val = match_pair[2];
      str_data[key] = val;
    } else if (std::regex_search(line, match_pair, num_data_re)) {
      auto key = match_pair[1];
      auto val = match_pair[2];
      num_data[key] = std::stod(val);
    } else {
      // throw ParseException("Invalid key value format for logger event");
    }
  }
}

Logger::Logger(std::string p) : _origin_pid(getpid()), logfile(p) {
  if (p.empty()) {
    return;
  }
  log_fd = open(p.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
}

void Logger::on_ev_start(log_event_listener listener) {
  event_start_listeners.push_back(std::move(listener));
}

void Logger::on_ev_end(log_event_listener listener) {
  event_end_listeners.push_back(std::move(listener));
}

std::string Logger::get_str_data(std::string key) {
  if (_origin_pid != getpid()) {
    int c_id = IPC::create_client();
    IPC::send_ipc(c_id, ipc_id, GET_STR, key);
    auto resp = IPC::recv_ipc(c_id);
    std::string resp_body;
    std::cout << "Requesting Str Key " << key << std::endl;
    std::cout << resp_body << std::endl;
    std::tie(std::ignore, std::ignore, resp_body) = resp;
    IPC::close_queue(c_id);
    return resp_body;
  }
  return str_data[key];
}

double Logger::get_num_data(std::string key) {
  if (_origin_pid != getpid()) {
    int c_id = IPC::create_client();
    IPC::send_ipc(c_id, ipc_id, GET_NUM, key);
    auto resp = IPC::recv_ipc(c_id);
    std::string resp_body;
    std::cout << "Requesting Num Key " << key << std::endl;
    std::cout << resp_body << std::endl;
    std::tie(std::ignore, std::ignore, resp_body) = resp;
    IPC::close_queue(c_id);
    return std::stod(resp_body);
  }
  return num_data[key];
}

void Logger::start_mp() {
  ipc_id = IPC::get_queue(logfile, LOGGER_ID);

  pid_t pid = fork();
  if (pid < 0) {
    perror("Logger Fork");
    return;
  }
  if (pid > 0) {
    _origin_pid = pid;
    return;
  }
  _origin_pid = getpid();
  // child process stuff

  // potentially multithread this
  while (true) {
    auto client_req = IPC::recv_ipc(ipc_id);
    handle_ipc(client_req);
  }
}

void Logger::handle_ipc(std::tuple<int, int64_t, std::string> client_req) {
  int client_id;
  int64_t msg_type;
  std::string req_body;
  std::tie(client_id, msg_type, req_body) = client_req;
  // std::cout << req_body << std::endl;
  switch (msg_type) {
    case RECORD:
      record(req_body);
      break;
    case EVT_START: {
      auto ev = LogEvent(std::stringstream(req_body));
      for (const auto &listener : event_start_listeners) {
        listener(ev);
      }
    } break;
    case EVT_END: {
      auto ev = LogEvent(std::stringstream(req_body));
      for (const auto &listener : event_end_listeners) {
        listener(ev);
      }
    } break;
    case GET_STR: {
      auto val = get_str_data(req_body);
      IPC::send_ipc(ipc_id, client_id, GET_STR, val);
    } break;
    case GET_NUM: {
      auto val = std::to_string(get_num_data(req_body));
      IPC::send_ipc(ipc_id, client_id, GET_NUM, val);
    } break;
  }
}

void Logger::broadcast_ipc(int64_t msg_type, std::string s) {
  int c_id = IPC::create_client();
  IPC::send_ipc(c_id, ipc_id, msg_type, s);
  IPC::close_queue(c_id);
}

void Logger::record(std::string s) {
  // std::cout << "we're here" << std::endl;
  if (getpid() != _origin_pid) {
    broadcast_ipc(RECORD, s);
    return;
  }

  s += "\n";
  if (log_fd) {
    std::lock_guard<std::mutex> l(file_mut);
    // I don't know if I need this
    // i'm wondering if cross process locks should take care of this
    // lockf(log_fd, F_LOCK, 0);
    // fseek(log_fd, 0, SEEK_END); // I don't think this is necessary
    // Children and parent file descriptors point to same location
    // and cross thread support should be built in ... in theory

    write(log_fd, s.c_str(), s.size());
    // lockf(log_fd, F_ULOCK, 0);
  }
}

}  // namespace Kleptic
