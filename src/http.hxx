#ifndef KLEPTIC_HTTP_HXX_
#define KLEPTIC_HTTP_HXX_

#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "concurrency.hxx"
#include "logger.hxx"
#include "server.hxx"

#define KLEPTIC_LOCALHOST "127.0.0.1"
#define KLEPTIC_ANYADDR "0.0.0.0"
#define KLEPTIC_HTTP_PORT 80
#define KLEPTIC_HTTPS_PORT 80
#define KLEPTIC_HTTP_LOGFILE "kleptic_http.log"
#define KLEPTIC_HTTP_SERVER_NAME "KlepticHTTP"

namespace Kleptic {

using std::string;
using std::stringstream;

struct HTTPConn {
  static std::map<const int, const string> default_status_reasons;
  enum ConnStatus { UNSET, SET };

  string host_ip;
  int host_port;

  /* Request Data */
  string remote_ip;

  /* start line */
  string method;
  string uri;
  string http_ver;

  /* extracted */
  string host;
  string req_path;
  string http_protocol;

  string route_unparsed_path;

  /* headers */
  std::map<string, string> req_headers;

  /* query params */
  string raw_query_string;
  std::map<string, string> query_params;

  /* body params */
  std::map<string, string> body_params;

  /* request data */
  stringstream req_body;

  /* response status */
  int resp_status = 200;

  /* response headers */
  std::map<string, string> resp_headers = {};

  /* response body */
  stringstream resp_body;

  /* if authenticated */
  string auth_type;
  string user;

  void parse(stringstream &ss);
  void set_resp_code(int);

  string get_request();
  string get_response();

  void send();
  void send(string s);

  bool is_set() const;

 protected:
  ConnStatus status = UNSET;
  string final_resp;
};

class HTTPRequestEv : public LogEvent {
 public:
  HTTPRequestEv();
  static std::string to_string(LogEvent &e) {
    stringstream ss;
    // todo add error checking for this
    std::string ip = e.str_data["ip"];
    std::string path = e.str_data["req_path"];
    std::string code = std::to_string(static_cast<int>(e.num_data["code"]));
    ss << ip << " " << path << " " << code;
    return ss.str();
  }
};

typedef std::function<void(HTTPConn &)> HTTPConnHandler;

class HTTPServer {
 protected:
  HTTPConn upgrade_http(const conn_t &conn);
  std::unique_ptr<SocketServer> s;
  HTTPServer(socket_server_t server, std::string logfile);

 public:
  std::chrono::time_point<std::chrono::system_clock> start_t;
  std::string ip;
  int port;
  Logger logger;
  static void sigpipe_handler(int);
  static const Concurrency::runner_t default_runner;
  HTTPServer(std::string ip = KLEPTIC_ANYADDR, int port = KLEPTIC_HTTP_PORT,
             string logfile = KLEPTIC_HTTP_LOGFILE);
  HTTPServer(std::string ip, int port, const Concurrency::runner_t &r, std::string logfile);
  void run(HTTPConnHandler);
};

class SecureHTTPServer : public HTTPServer {
 public:
  SecureHTTPServer(tls_cert_key_pair conf, std::string ip = KLEPTIC_ANYADDR,
                   int port = KLEPTIC_HTTPS_PORT, string logfile = KLEPTIC_HTTP_LOGFILE);
  SecureHTTPServer(tls_cert_key_pair conf, std::string ip, int port, const Concurrency::runner_t &r,
                   std::string logfile);
};

}  // namespace Kleptic

#endif  // KLEPTIC_HTTP_HXX_
