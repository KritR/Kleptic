#include "socket.hxx"

#include <string>

namespace Kleptic {

std::string Conn::getIP4() {
  char ip_str[INET_ADDRSTRLEN];
  const char *ptr = inet_ntop(addr.sin_family, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);
  if (ptr == NULL) {
    throw "IPV4 ADDR UNDETERMINABLE";
  }
  return std::string(ip_str);
}

}  // namespace Kleptic
