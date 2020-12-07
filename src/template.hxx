#ifndef KLEPTIC_TEMPLATE_HXX_
#define KLEPTIC_TEMPLATE_HXX_

#include <map>
#include <string>

namespace Kleptic::Template {

std::string render_page(std::string path, std::map<std::string, std::string> page_injection);
}

#endif  // KLEPTIC_TEMPLATE_HXX_
