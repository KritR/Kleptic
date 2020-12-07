#ifndef KLEPTIC_HANDLER_HXX_
#define KLEPTIC_HANDLER_HXX_

#include <experimental/filesystem>
#include <functional>
#include <map>
#include <string>

#define KLEPTIC_STATIC_DIR_TEMPLATE_PATH "templates/static.html.ktf"

#include "base64.hxx"
#include "http.hxx"

namespace fs = std::experimental::filesystem;

namespace Kleptic::Handler {

using std::string;

typedef std::function<fs::path(HTTPConn &)> HTTPConnFSHandler;
HTTPConnHandler create_basic_auth_handler(const std::string realm,
                                          const std::map<const string, const string> usr_pass);
HTTPConnFSHandler create_static_handler(
    const std::string path, const std::string template_path = KLEPTIC_STATIC_DIR_TEMPLATE_PATH);

typedef std::function<void(int ssock, const char *querystring)> cgi_func;
typedef void (*cgi_func_ptr)(int ssock, const char* querystring);
HTTPConnHandler create_cgi_handler(const std::string path);

void not_found_handler(HTTPConn &);

HTTPConnHandler derive_http_handler(HTTPConnFSHandler);
HTTPConnHandler derive_http_handler(HTTPConnHandler);

}  // namespace Kleptic::Handler

#endif  // KLEPTIC_HANDLER_HXX_
