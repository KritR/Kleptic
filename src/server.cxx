#include "server.hxx"

#include <future>
#include <memory>
#include <string>
#include <utility>

namespace Kleptic {
SocketServer::SocketServer(s_acceptor_t s, const Concurrency::runner_t &r)
    : _s_acceptor(std::move(s)), _runner(r) {}

TCPServer::TCPServer(std::string ip, int port, const Concurrency::runner_t &r)
    : SocketServer(std::make_unique<TCPAcceptor>(ip, port), r) {}

TLSServer::TLSServer(std::string ip, int port, const Concurrency::runner_t &r,
                     tls_cert_key_pair &conf)
    : SocketServer(std::make_unique<TLSAcceptor>(ip, port, conf), r) {}
}  // namespace Kleptic
