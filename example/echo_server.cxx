#include <iostream>
#include <memory>

#include "concurrency.hxx"
#include "server.hxx"
#include "socket.hxx"

using namespace Kleptic;

int main() {
  Concurrency::runner_t exec = std::make_unique<Concurrency::ThreadPoolRunner>(10);
  TCPServer server("127.0.0.1", 4858, exec);
  // TCPServer server("127.0.0.1", 4858, THR_SINGLE);
  server.run([](conn_t conn) {
    std::cout << "Recieved Packet From " << conn->getIP4() << std::endl;
    auto s = conn->socket->read_all();
    std::cout << s.str() << std::endl;
    conn->socket->write(s.str());
  });
}
