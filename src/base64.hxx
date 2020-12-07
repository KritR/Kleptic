#ifndef KLEPTIC_BASE64_HXX_
#define KLEPTIC_BASE64_HXX_

#include <string>
#include <vector>
/* USING https://stackoverflow.com/a/34571089
 * C++ header for encoding and decoding base64
 */

namespace Kleptic::Util {
std::string base64_encode(const std::string &in);
std::string base64_decode(const std::string &in);
}  // namespace Kleptic::Util
#endif  // KLEPTIC_BASE64_HXX_
