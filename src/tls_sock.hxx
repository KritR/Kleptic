#ifndef KLEPTIC_TLS_SOCK_HXX_
#define KLEPTIC_TLS_SOCK_HXX_

#include <openssl/ssl.h>

#include <memory>
#include <string>
#include <utility>

#include "tcp_sock.hxx"

namespace Kleptic {

typedef std::unique_ptr<SSL_CTX, decltype((SSL_CTX_free))> ssl_ctx_t;
typedef std::unique_ptr<SSL, decltype((SSL_free))> ssl_t;
typedef std::pair<std::string, std::string> tls_cert_key_pair;

class TLSSocket : public TCPSocket {
  const ssl_t ssl;

 public:
  int read(char *buff, const int size) override;
  void write(const char *buff, const int len) override;
  TLSSocket(const int sfd, const ssl_ctx_t &ctx);
  // protected:
  // virtual TLSSocket* clone_impl() const override { return new
  // TLSSocket(*this); };
};

class TLSAcceptor : public TCPAcceptor {
 protected:
  const ssl_ctx_t ctx;

 public:
  static void init_ssl();
  static void cleanup_ssl();
  TLSAcceptor(const std::string &ip, const int port, tls_cert_key_pair &);
  conn_t accept_conn() const override;
  // ~TLSAcceptor();
};
}  // namespace Kleptic

#endif  // KLEPTIC_TLS_SOCK_HXX_
