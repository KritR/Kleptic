/**
 * This file parses the command line arguments and correctly
 * starts your server. You should not need to edit this file
 */

#include <sys/resource.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "concurrency.hxx"
#include "handler.hxx"
#include "http.hxx"
#include "logger.hxx"
#include "router.hxx"

#define LOGFILE "httpd.log"
namespace k = Kleptic;

enum concurrency_mode {
  E_NO_CONCURRENCY = 0,
  E_FORK_PER_REQUEST = 'f',
  E_THREAD_PER_REQUEST = 't',
  E_THREAD_POOL = 'p'
};

extern "C" void signal_handler(int) { exit(0); }

int main(int argc, char **argv) {
  /*struct rlimit mem_limit = { .rlim_cur = 40960000, .rlim_max = 91280000 };
  struct rlimit cpu_limit = { .rlim_cur = 300, .rlim_max = 600 };
  struct rlimit nproc_limit = { .rlim_cur = 50, .rlim_max = 100 };
  if (setrlimit(RLIMIT_AS, &mem_limit)) {
      perror("Couldn't set memory limit\n");
  }
  if (setrlimit(RLIMIT_CPU, &cpu_limit)) {
      perror("Couldn't set CPU limit\n");
  }
  if (setrlimit(RLIMIT_NPROC, &nproc_limit)) {
      perror("Couldn't set NPROC limit\n");
  }*/

  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  enum concurrency_mode mode = E_NO_CONCURRENCY;
  char use_https = 0;
  int port_no = 0;
  int num_threads = 0;  // for use when running in pool of threads mode

  char usage[] = "USAGE: myhttpd [-f|-t|-pNUM_THREADS] [-s] [-h] PORT_NO\n";

  if (argc == 1) {
    fputs(usage, stdout);
    return 0;
  }

  int c;
  while ((c = getopt(argc, argv, "hftp:s")) != -1) {
    switch (c) {
      case 'h':
        fputs(usage, stdout);
        return 0;
      case 'f':
      case 't':
      case 'p':
        if (mode != E_NO_CONCURRENCY) {
          fputs("Multiple concurrency modes specified\n", stdout);
          fputs(usage, stderr);
          return -1;
        }
        mode = (enum concurrency_mode) c;
        if (mode == E_THREAD_POOL) {
          num_threads = stoi(std::string(optarg));
        }
        break;
      case 's':
        use_https = 1;
        break;
      case '?':
        if (isprint(optopt)) {
          std::cerr << "Unknown option: -" << static_cast<char>(optopt) << std::endl;
        } else {
          std::cerr << "Unknown option" << std::endl;
        }
        // Fall through
      default:
        fputs(usage, stderr);
        return 1;
    }
  }

  if (optind > argc) {
    std::cerr << "Extra arguments were specified" << std::endl;
    fputs(usage, stderr);
    return 1;
  } else if (optind == argc) {
    std::cerr << "Port number must be specified" << std::endl;
    return 1;
  }

  port_no = atoi(argv[optind]);
  printf("%d %d %d %d\n", mode, use_https, port_no, num_threads);

  k::Concurrency::runner_t exec;
  switch (mode) {
    case E_FORK_PER_REQUEST:
      exec = std::make_unique<k::Concurrency::ForkRunner>();
      break;
    case E_THREAD_PER_REQUEST:
      exec = std::make_unique<k::Concurrency::ThreadRunner>();
      break;
    case E_THREAD_POOL:
      exec = std::make_unique<k::Concurrency::ThreadPoolRunner>(num_threads);
      break;
    default:
      exec = std::make_unique<k::Concurrency::SingleRunner>();
      break;
  }
  std::unique_ptr<k::HTTPServer> server;

  if (use_https) {
    server = std::make_unique<k::SecureHTTPServer>(
        k::tls_cert_key_pair("priv/cert.pem", "priv/key.pem"), "0.0.0.0", port_no, exec, LOGFILE);
  } else {
    server = std::make_unique<k::HTTPServer>("0.0.0.0", port_no, exec, LOGFILE);
  }

  k::Logger &logger = server->logger;

  logger.on_ev_end([&](k::LogEvent &e) {
    // std::cout << e.get_name() << std::endl;

    if (e.get_name().compare("HTTP_REQ_EV")) {
      return;
    }

    // std::cout << " hiya " << std::endl;
    logger.with_num_data([&](auto nd) {
      nd.get()["NUM_REQ"] += 1;
      if (nd.get().find("MAX_REQ_TIME") == nd.get().end()) {
        nd.get()["MAX_REQ_TIME"] = e.get_duration();

        logger.with_str_data([&](auto sd) { sd.get()["MAX_REQ"] = e.str_data["req_path"]; });
      } else if (e.get_duration() > nd.get()["MAX_REQ_TIME"]) {
        nd.get()["MAX_REQ_TIME"] = e.get_duration();

        logger.with_str_data([&](auto sd) { sd.get()["MAX_REQ"] = e.str_data["req_path"]; });
      }
      if (nd.get()["MIN_REQ_TIME"] == 0) {
        nd.get()["MIN_REQ_TIME"] = e.get_duration();

        logger.with_str_data([&](auto sd) { sd.get()["MIN_REQ"] = e.str_data["req_path"]; });
      } else if (e.get_duration() < nd.get()["MIN_REQ_TIME"]) {
        nd.get()["MIN_REQ_TIME"] = e.get_duration();

        logger.with_str_data([&](auto sd) { sd.get()["MIN_REQ"] = e.str_data["req_path"]; });
      }
    });
  });

  const std::map<const std::string, const std::string> auth = {{"rao96", "thisismine"}};

  auto auth_handle = k::Handler::create_basic_auth_handler("http-auth", auth);

  auto static_handle = k::Handler::create_static_handler("./http-root-dir/htdocs");

  k::Handler::HTTPConnFSHandler log_handler = [](k::HTTPConn &) { return fs::path(LOGFILE); };

  auto cgi_handler = k::Handler::create_cgi_handler("./http-root-dir/");

  auto stats_handler = [&](k::HTTPConn &c) {
    c.req_headers["Content-Type"] = "text/plain";
    c.resp_body << "Krithik Rao" << std::endl;
    auto curr_t = std::chrono::system_clock::now();
    auto dur = curr_t - server->start_t;
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(dur).count();
    c.resp_body << "Uptime (s): " << uptime << std::endl;
    c.resp_body << "Num Requests: " << logger.get_num_data("NUM_REQ") << std::endl;
    c.resp_body << "Max Srvc Time: " << logger.get_num_data("MAX_REQ_TIME") << "ms ; "
                << logger.get_str_data("MAX_REQ") << std::endl;
    c.resp_body << "Min Srvc Time: " << logger.get_num_data("MIN_REQ_TIME") << "ms ; "
                << logger.get_str_data("MIN_REQ") << std::endl;
    c.send();
  };

  k::Router r;

  r.add_middleware(auth_handle);

  r.r_get("/", static_handle);

  r.r_get("/logs", log_handler);
  r.r_get("/stats", stats_handler);

  r.r_get("/cgi-bin", cgi_handler);
  r.r_post("/cgi-bin", cgi_handler);
  // r.r_get("/stats", stat_handler);
  //
  if (mode == E_FORK_PER_REQUEST) {
    logger.start_mp();
  }

  server->run([&](k::HTTPConn &c) {
    std::cout << c.get_request() << std::endl;
    r.handle(c);
  });
}
