#include "http.hxx"

#include <signal.h>

#include <ctime>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <utility>

#include "error.hxx"
#include "strutil.hxx"

namespace Kleptic {

void HTTPConn::parse(std::stringstream &ss) {
  using std::regex;

  string start_line;
  if (!std::getline(ss, start_line)) {
    throw ParseException("Malformed Request Start Line");
  }
  std::stringstream start_line_stream(start_line);

  start_line_stream >> method >> uri >> http_ver;
  method = Util::trim(method);
  uri = Util::trim(uri);
  http_ver = Util::trim(http_ver);

  /* validate method */
  regex method_re("^(OPTIONS)|(GET)|(HEAD)|(POST)|(PUT)|(DELETE)|(TRACE)|(CONNECT)$");
  if (!std::regex_match(method, method_re)) {
    throw ParseException("HTTP Method not Supported : " + method, 405);
  }

  /* validate protocol */
  /*if(http_ver.compare("HTTP/1.1")) {
    throw ParseException("HTTP Version Not Supported", 505);
  }*/

  regex abs_uri_re("^([^:\\/?#]+):\\/\\/([^\\/?#]*)([^?#]*)(?:\\?([^#]*))?$");
  regex rel_uri_re("^(\\/[^?#]*)(?:\\?([^#]*))?$");
  std::smatch uri_match;

  /* parse and validate uri */
  if (std::regex_search(uri, uri_match, abs_uri_re)) {
    string uri_protocol = uri_match[1];
    http_protocol = uri_protocol;

    if (uri_protocol.compare("http") && uri_protocol.compare("https")) {
      throw ParseException("Absolute URI protocol not supported");
    }

    host = uri_match[2];
    req_path = uri_match[3];
    if (uri_match.size() > 4) {
      raw_query_string = uri_match[4];
    }
  } else if (std::regex_search(uri, uri_match, rel_uri_re)) {
    req_path = uri_match[1];
    raw_query_string = uri_match[2];
  } else {
    throw ParseException("Invalid Request URI");
  }

  /* parse parameters */
  stringstream query_stream(raw_query_string);
  std::smatch query_match;
  regex query_re("^(?:([^=]+)=([^&]+)&)*([^=]+)=([^&]+)$");
  // Get all the query strings
  if (std::regex_search(raw_query_string, query_match, query_re)) {
    for (size_t i = 1; i < query_match.size(); i += 2) {
      query_params.insert(std::pair<string, string>(query_match[i], query_match[i + 1]));
    }
  }
  string hdr_line;

  /* parse header lines */
  while (std::getline(ss, hdr_line)) {
    if (!hdr_line.compare("\r")) {
      break;
    }
    if (hdr_line.find(':') == string::npos) {
      throw ParseException("Bad HTTP Header Field");
    }
    string hdr_key;
    string hdr_val;
    stringstream line_ss(hdr_line);
    std::getline(line_ss, hdr_key, ':');
    std::getline(line_ss, hdr_val);
    hdr_key = Util::trim(hdr_key);
    hdr_val = Util::trim(hdr_val);
    req_headers.insert(std::make_pair(hdr_key, hdr_val));
    // match field-name:field-value
  }

  /* store request data */
  req_body << ss.rdbuf();
}

string HTTPConn::get_request() {
  stringstream ss;
  ss << "\\\\==////REQ\\\\\\\\==////" << std::endl;

  ss << "Method: {" << method << "}" << std::endl;
  ss << "Request URI: {" << uri << "}" << std::endl;
  ss << "Query string: {" << raw_query_string << "}" << std::endl;
  ss << "HTTP Version: {" << http_ver << "}" << std::endl;

  ss << "Headers: " << std::endl;
  for (auto const &[key, val] : req_headers) {
    ss << "field-name: " << key << "; field-value: " << val << std::endl;
  }

  string req_str = req_body.str();

  /* auto content_length =
   * std::distance(std::istream_iterator<std::string>(req_body),
   * std::istream_iterator<std::string>()); */

  ss << "Message body length: " << req_str.size() << std::endl;
  ss << req_str << std::endl;

  // Magic string to help with autograder
  ss << "//==\\\\\\\\REQ////==\\\\" << std::endl;

  return ss.str();
}

string HTTPConn::get_response() {
  if (is_set()) {
    return final_resp;
  }
  stringstream ss;
  ss << http_ver << " " << resp_status << " " << HTTPConn::default_status_reasons[resp_status]
     << "\r\n";
  std::time_t now = std::time(0);
  std::tm *now_tm = std::gmtime(&now);
  const int RFC1123_TIME_LEN = 29;
  const string date_format_str = "%a, %d %b %Y %H:%M:%S GMT";
  char date_buff[RFC1123_TIME_LEN + 1];
  strftime(date_buff, RFC1123_TIME_LEN + 1, date_format_str.c_str(), now_tm);

  resp_headers["Date"] = std::string(date_buff);
  /*auto content_length =
   * std::distance(std::istream_iterator<std::string>(resp_body),
   * std::istream_iterator<std::string>()); */
  string resp_str = resp_body.str();
  resp_headers["Content-Length"] = std::to_string(resp_str.size());
  for (auto const &[key, val] : resp_headers) {
    ss << key << ": " << val << "\r\n";
  }
  ss << "\r\n";
  ss << resp_str;
  return ss.str();
}

void HTTPConn::send() { send(get_response()); }

void HTTPConn::send(string s) {
  if (is_set()) {
    return;
  }
  status = HTTPConn::SET;
  final_resp = s;
}

bool HTTPConn::is_set() const { return (status == HTTPConn::SET); }

void HTTPServer::sigpipe_handler(int) {}

HTTPConn HTTPServer::upgrade_http(const conn_t &conn) {
  HTTPConn c;
  stringstream ss = conn->socket->read_all();
  c.remote_ip = conn->getIP4();
  try {
    c.parse(ss);
  } catch (ParseException &ex) {
    c.resp_status = ex.err_code;
    c.send();
  }
  return c;
}

HTTPServer::HTTPServer(std::string ip, int port, string logfile)
    : HTTPServer(ip, port, HTTPServer::default_runner, logfile) {}

HTTPServer::HTTPServer(std::string ip, int port, const Concurrency::runner_t &r,
                       std::string logfile)
    : HTTPServer(std::make_unique<TCPServer>(ip, port, std::ref(r)), logfile) {
  this->ip = ip;
  this->port = port;
}

HTTPServer::HTTPServer(socket_server_t server, std::string logfile)
    : s(std::move(server)), logger(logfile) {
  signal(SIGPIPE, sigpipe_handler);
  logger.on_ev_end([this](LogEvent &e) {
    if (!e.get_name().compare("HTTP_REQ_EV")) {
      logger.record(HTTPRequestEv::to_string(e));
    }
  });
}

void HTTPServer::run(HTTPConnHandler handle) {
  start_t = std::chrono::system_clock::now();
  s->run([this, handle](conn_t conn) {
    auto ev = logger.create_event<HTTPRequestEv>();
    ev->start();
    HTTPConn hconn = upgrade_http(conn);
    hconn.host_ip = ip;
    hconn.host_port = port;
    ev->str_data["ip"] = hconn.remote_ip;
    ev->num_data["code"] = hconn.resp_status;
    ev->str_data["req_path"] = hconn.req_path;
    if (!hconn.is_set()) {
      handle(hconn);
    }
    conn->socket->write(hconn.get_response());
    ev->end();
  });
}

SecureHTTPServer::SecureHTTPServer(tls_cert_key_pair conf, std::string ip, int port, string logfile)
    : SecureHTTPServer(conf, ip, port, default_runner, logfile) {}
SecureHTTPServer::SecureHTTPServer(tls_cert_key_pair conf, std::string ip, int port,
                                   const Concurrency::runner_t &r, std::string logfile)
    : HTTPServer(std::make_unique<TLSServer>(ip, port, std::ref(r), conf), logfile) {
  this->ip = ip;
  this->port = port;
}

HTTPRequestEv::HTTPRequestEv() { ev_name = "HTTP_REQ_EV"; }
// std::string HTTPRequestEv::get_name() const { return "HTTP_REQ_EV"; }

const Concurrency::runner_t HTTPServer::default_runner =
    std::make_unique<Concurrency::SingleRunner>();

std::map<const int, const string> HTTPConn::default_status_reasons = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Time-out"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Request Entity Too Large"},
    {414, "Request-URI Too Large"},
    {415, "Unsupported Media Type"},
    {416, "Requested range not satisfiable"},
    {417, "Expectation Failed"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Time-out"},
    {505, "HTTP Version not supported"}};

}  // namespace Kleptic
