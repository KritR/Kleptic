#ifndef KLEPTIC_SERVER_HXX_
#define KLEPTIC_SERVER_HXX_

#include <arpa/inet.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "concurrency.hxx"
#include "socket.hxx"
#include "tcp_sock.hxx"
#include "tls_sock.hxx"

namespace Kleptic {

typedef std::function<void(conn_t)> ConnHandler;

class SocketServer {
 protected:
  const s_acceptor_t _s_acceptor;
  const Concurrency::runner_t &_runner;
  SocketServer(s_acceptor_t s, const Concurrency::runner_t &r);

 public:
  template <typename F>
  void run(F handle) {
    while (1) {
      conn_t c = _s_acceptor->accept_conn();
      // std::cout << "Accepted Connection" << std::endl;

      auto task_lambda = [&, c = std::move(c)]() mutable { handle(std::move(c)); };

      std::packaged_task<void()> task{std::move(task_lambda)};
      _runner->dispatch(std::move(task));
    }
  }
};

typedef std::unique_ptr<SocketServer> socket_server_t;

class TCPServer : public SocketServer {
 public:
  TCPServer(std::string ip, int port, const Concurrency::runner_t &r);
};

class TLSServer : public SocketServer {
 public:
  TLSServer(std::string ip, int port, const Concurrency::runner_t &r, tls_cert_key_pair &);
};

}  // namespace Kleptic

#endif  // KLEPTIC_SERVER_HXX_
