#include "tls_sock.hxx"

#include <openssl/err.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "error.hxx"

namespace Kleptic {

TLSAcceptor::TLSAcceptor(const std::string &ip, const int port, tls_cert_key_pair &conf)
    : TCPAcceptor(ip, port), ctx((init_ssl(), SSL_CTX_new(SSLv23_server_method())), SSL_CTX_free) {
  /* configure SSL certificate */
  SSL_CTX_set_ecdh_auto(ctx.get(), 1);
  if (SSL_CTX_use_certificate_file(ctx.get(), conf.first.c_str(), SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }
  if (SSL_CTX_use_PrivateKey_file(ctx.get(), conf.second.c_str(), SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }
}

conn_t TLSAcceptor::accept_conn() const {
  auto conn = TCPAcceptor::accept_conn();
  conn->socket = std::make_unique<TLSSocket>(dup(conn->socket->_socket_fd), ctx);
  return conn;
}

void TLSAcceptor::init_ssl() {
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();
}

void TLSAcceptor::cleanup_ssl() { EVP_cleanup(); }

TLSSocket::TLSSocket(const int sfd, const ssl_ctx_t &ctx)
    : TCPSocket(sfd), ssl(SSL_new(ctx.get()), SSL_free) {
  SSL_set_fd(ssl.get(), _socket_fd);
  if (SSL_accept(ssl.get()) <= 0) {
    ERR_print_errors_fp(stderr);
  }
}

/*
 * int read(char * buff, const int size)
 *
 * reads size bytes into the buffer specified from
 * the tcp socket input.
 * returns the number of bytes read in.
 *
 */
int TLSSocket::read(char *buff, const int size) {
  int ret = SSL_read(ssl.get(), buff, size);
  if (ret < 0) {
    throw SocketException("unable to read character");
  }
  return ret;
}

/*
 * void write(const char *, int len)
 *
 * writes len bytes from the buffer specified into the
 * tcp socket.
 *
 */
void TLSSocket::write(const char *buff, int len) {
  int ret = SSL_write(ssl.get(), buff, len);
  if (ret < 0) {
    throw SocketException("failed to write characters due to : " + std::string(strerror(errno)));
  }
  if (ret < len) {
    throw SocketException("only managed to write " + std::to_string(ret) + "/" +
                          std::to_string(len) + " bytes.");
  }
}

}  // namespace Kleptic
