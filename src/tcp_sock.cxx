#include "tcp_sock.hxx"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "error.hxx"

namespace Kleptic {

/*
 * TCPSocket(int)
 *
 * creates a tcp socket with a socket file descriptor
 *
 */
TCPSocket::TCPSocket(int sfd) : Socket(sfd) {}

/*
 * stringstream read_all()
 *
 * returns a stringstream with the full contents
 * of the tcp data
 *
 */
std::stringstream TCPSocket::read_all() {
  std::stringstream ss;
  char buff[TCP_BUFF_SIZE];
  int read_size = sizeof(buff) - 1;
  int ret = 0;
  do {
    ret = read(buff, read_size);
    if (ret == 0) {
      break;  // EOF Detected
    }
    buff[ret] = '\0';
    ss << std::string(buff);
  } while (ret == read_size);
  return ss;
}

/*
 * int read(char * buff, const int size)
 *
 * reads size bytes into the buffer specified from
 * the tcp socket input.
 * returns the number of bytes read in.
 *
 */
int TCPSocket::read(char *buff, const int size) {
  int ret = recv(_socket_fd, buff, size, 0);
  if (ret < 0) {
    throw SocketException("unable to read character");
  }
  return ret;
}

/*
 * void write(string)
 *
 * writes the string into the tcp socket.
 *
 */
void TCPSocket::write(std::string const &data) { write(data.c_str(), data.size()); }

/*
 * void write(const char *, int len)
 *
 * writes len bytes from the buffer specified into the
 * tcp socket.
 *
 */
void TCPSocket::write(const char *buff, int len) {
  int ret = send(_socket_fd, buff, len, 0);
  if (ret < 0) {
    throw SocketException("failed to write characters due to : " + std::string(strerror(errno)));
  }
  if (ret < len) {
    throw SocketException("only managed to write " + std::to_string(ret) + "/" +
                          std::to_string(len) + " bytes.");
  }
}

/*
 * ~TCPSocket()
 *
 * closes socket on deconstruction
 */
TCPSocket::~TCPSocket() {
  // std::cout << "closing client connection" << std::endl;
  close(_socket_fd);
}

TCPAcceptor::TCPAcceptor(const std::string &ip, const int port) {
  int optval = 1;

  if ((_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    throw SocketException("Failed to Create Socket : " + std::string(strerror(errno)));
  }

  if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    throw SocketException("Failed to Set Socket Opt : " + std::string(strerror(errno)));
  }

  _addr.sin_family = AF_INET;
  inet_pton(AF_INET, ip.c_str(), &(_addr.sin_addr));
  _addr.sin_port = htons(port);

  if (bind(_server_fd, (struct sockaddr *) &_addr, sizeof(_addr)) < 0) {
    throw SocketException("Failed to Bind Socket : " + std::string(strerror(errno)));
  }

  if (listen(_server_fd, MAX_CONN_BACKLOG) < 0) {
    throw SocketException("Failed to Begin Listening : " + std::string(strerror(errno)));
  }
}

conn_t TCPAcceptor::accept_conn() const {
  struct sockaddr_in _addr_new = _addr;
  int addrlen = sizeof(_addr_new);
  int sock_fd;
  if ((sock_fd = accept(_server_fd, reinterpret_cast<struct sockaddr *>(&_addr_new),
          reinterpret_cast<socklen_t *>(&addrlen))) < 0) {
    throw SocketException("Failed to accept client : " + std::string(strerror(errno)));
  }
  conn_t c = std::make_unique<Conn>();
  c->socket = std::make_unique<TCPSocket>(sock_fd);
  c->addr = _addr_new;
  return c;
}

TCPAcceptor::~TCPAcceptor() noexcept { close(_server_fd); }
}  // namespace Kleptic
