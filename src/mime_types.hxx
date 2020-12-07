#include <string>

#ifndef KLEPTIC_MIME_TYPES_HXX_
#define KLEPTIC_MIME_TYPES_HXX_

#define MIME_PATH "/etc/mime.types"

namespace Kleptic::Util {
using std::string;
const string &get_content_type(string ext);
}  // namespace Kleptic::Util

#endif  // KLEPTIC_MIME_TYPES_HXX_
