#ifndef KLEPTIC_STRUTIL_HXX_
#define KLEPTIC_STRUTIL_HXX_

#include <algorithm>
#include <cctype>
#include <locale>
#include <string>
#include <utility>

namespace Kleptic::Util {
// trim from start (in place)
static inline std::string ltrim(std::string s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
  return s;
}

// trim from end (in place)
static inline std::string rtrim(std::string s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
          s.end());
  return s;
}

// trim from both ends (in place)
static inline std::string trim(std::string s) {
  s = ltrim(s);
  s = rtrim(s);
  return s;
}
}  // namespace Kleptic::Util

#endif  // KLEPTIC_STRUTIL_HXX_
