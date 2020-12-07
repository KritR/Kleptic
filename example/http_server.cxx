#include <iostream>
#include <map>

#include "concurrency.hxx"
#include "handler.hxx"
#include "http.hxx"

using namespace Kleptic;

int main() {
  Concurrency::runner_t exec = std::make_unique<Concurrency::ThreadPoolRunner>(20);
  HTTPServer server("0.0.0.0", 4858, exec, "http_example.log");

  server.run([](HTTPConn &c) {
    /*std::cout << c.get_request()  << std::endl;*/

    const std::map<const std::string, const std::string> auth = {{"cricket", "thisismine"}};

    auto auth_handle = Handler::create_basic_auth_handler("http", auth);

    auto static_handle = Handler::derive_http_handler(
        Handler::create_static_handler("./static_files"));

    auth_handle(c);
    if (!c.is_set()) {
      static_handle(c);
    }

    /*std::cout << c.get_response() << std::endl;*/
  });
}
