#ifndef KLEPTIC_SOCKET_HXX_
#define KLEPTIC_SOCKET_HXX_

#include <arpa/inet.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace Kleptic {
class Socket {
  /*protected:
    virtual Socket* clone_impl() const = 0;*/
 public:
  int _socket_fd;
  /*auto clone() const { return std::unique_ptr<Socket>(clone_impl()); }*/
  virtual ~Socket() = default;
  virtual std::stringstream read_all() = 0;
  virtual int read(char *buff, const int size) = 0;
  virtual void write(std::string const &data) = 0;
  virtual void write(const char *buff, int len) = 0;
  explicit Socket(int fd) : _socket_fd(fd) {}
};

typedef std::unique_ptr<Socket> sock_ptr;

struct Conn {
  sock_ptr socket;
  struct sockaddr_in addr;
  std::string getIP4();

  Conn() = default;
  ~Conn() = default;

  /*Conn(Conn const& other) : socket(other.socket->clone()) {}
  Conn(Conn && other) = default;
  Conn& operator=(Conn const& other) { socket = other.socket->clone(); return
  *this; } Conn& operator=(Conn && other) = default;*/

  /*
   * Returns the clients port ... but we've deemed it not useful for now.
   */
  /*
  uint16_t getPort() {
    return ntohs(addr.sin_port);
  }*/
};

typedef std::unique_ptr<Conn> conn_t;

class SockAcceptor {
 public:
  virtual conn_t accept_conn() const = 0;
  virtual ~SockAcceptor() = default;
};

typedef std::unique_ptr<SockAcceptor> s_acceptor_t;

}  // namespace Kleptic

#endif  // KLEPTIC_SOCKET_HXX_
