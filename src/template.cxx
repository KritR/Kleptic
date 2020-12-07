#include "template.hxx"

#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>

#include "error.hxx"

namespace Kleptic::Template {

std::string render_page(std::string path, std::map<std::string, std::string> page_injection) {
  std::ifstream page;
  page.open(path);
  if (!page.is_open()) {
    throw TemplateException("Template File Not Found", path);
  }
  std::stringstream ss;
  ss << page.rdbuf();
  std::string page_str = ss.str();
  for (const auto& [key, injection] : page_injection) {
    std::regex re("%\\{\\{" + key + "\\}\\}");
    page_str = std::regex_replace(page_str, re, injection);
  }
  // std::cout << page_str << std::endl;
  return page_str;
}
}  // namespace Kleptic::Template
