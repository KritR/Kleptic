#include "mime_types.hxx"

#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <utility>

namespace Kleptic::Util {

void load_mime_types();

std::map<const string, const string> mime_types;

const string &get_content_type(string ext) {
  static const string empty = "";
  if (mime_types.empty()) {
    load_mime_types();
  }
  auto it = mime_types.find(ext);
  if (it == mime_types.end()) {
    return empty;
  } else {
    return it->second;
  }
}

void load_mime_types() {
  std::regex text_regex(R"(\S+)");

  std::ifstream mime_file;
  mime_file.open(MIME_PATH);
  if (!mime_file.is_open()) {
    std::cerr << "Unable to open mime_file. No content types being assigned." << std::endl;
  }
  string line;
  std::sregex_iterator txt_end;
  while (getline(mime_file, line)) {
    if (line.size() <= 0 || line.front() == '#') {
      continue;
    }
    /* parsing mime_types */
    std::sregex_iterator txt_begin(line.begin(), line.end(), text_regex);
    if (std::distance(txt_begin, txt_end) < 2) {
      continue;
    }

    string m_type = txt_begin->str();
    txt_begin++;

    for (auto i = txt_begin; i != txt_end; ++i) {
      mime_types.insert(std::make_pair(i->str(), m_type));
    }
  }
  mime_file.close();
}
}  // namespace Kleptic::Util
