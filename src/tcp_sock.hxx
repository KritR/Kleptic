#ifndef KLEPTIC_TCP_SOCK_HXX_
#define KLEPTIC_TCP_SOCK_HXX_

#include <sys/socket.h>

#include <string>

#include "socket.hxx"

#define TCP_BUFF_SIZE 4096
#define MAX_CONN_BACKLOG 50

namespace Kleptic {
class TCPSocket : public Socket {
 public:
  std::stringstream read_all() override;
  int read(char *buff, const int size) override;
  void write(std::string const &data) override;
  void write(const char *buff, const int len) override;
  ~TCPSocket();
  explicit TCPSocket(const int sfd);
  // protected:
  // virtual TCPSocket* clone_impl() const override { return new
  // TCPSocket(*this); }; const int _socket_fd;
};

class TCPAcceptor : public SockAcceptor {
 protected:
  struct sockaddr_in _addr;
  int _server_fd;

 public:
  TCPAcceptor(const std::string &ip, const int port);
  conn_t accept_conn() const override;
  ~TCPAcceptor() noexcept;
};
}  // namespace Kleptic

#endif  // KLEPTIC_TCP_SOCK_HXX_
