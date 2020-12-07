
#include <iostream>

#include "mime_types.hxx"

using namespace Kleptic;

int main() {
  std::cout << Util::get_content_type("svg") << std::endl;

  return 0;
}
