#include "handler.hxx"

#include <dlfcn.h>
#include <wait.h>

#include <chrono>
#include <fstream>
#include <map>
#include <set>
#include <string>

#include "mime_types.hxx"
#include "template.hxx"

#define KLEPTIC_CGI_VER "CGI/1.1"

#define KLEPTIC_CGI_PIPE_BUFFER_SIZE 1024

namespace Kleptic::Handler {

HTTPConnHandler create_basic_auth_handler(const std::string realm,
                                          const std::map<const string, const string> usr_pass) {
  std::set<string> auth_vals;

  for (const auto &[key, val] : usr_pass) {
    string encoded_credentials = Kleptic::Util::base64_encode(key + ":" + val);
    auth_vals.insert(encoded_credentials);
  }

  return [realm, auth_vals](HTTPConn &c) {
    std::stringstream auth_stream;
    auth_stream << c.req_headers["Authorization"];
    std::string auth_type;
    std::string credentials;
    auth_stream >> auth_type >> credentials;
    const bool auth_valid = auth_vals.find(credentials) != auth_vals.end();
    const bool is_basic = (!auth_type.compare("Basic"));

    if (is_basic && auth_valid) {
      // do nothing. credentials are valid
      c.auth_type = auth_type;
      auto decoded_creds = Util::base64_decode(credentials);
      c.user = decoded_creds.substr(0, decoded_creds.find(":"));
      return;
    }

    c.resp_status = 401;
    c.resp_headers["WWW-Authenticate"] = "Basic realm=\"" + realm + "\"";
    c.send();
  };
}

bool is_subdirectory(const fs::path &parent, const fs::path &child) {
  fs::path iter_path = child;
  /* check if path is child of path */
  while (!iter_path.empty()) {
    if (fs::equivalent(iter_path, parent)) {
      return true;
    }
    iter_path = iter_path.parent_path();
  }
  return false;
}

std::string render_dir_page(const fs::path &dir, const fs::path &req_root,
                            const fs::path &template_file) {
  std::map<std::string, std::string> page_inject;
  page_inject["dir_name"] = dir.filename();
  std::stringstream dir_lines;
  for (const auto &p : fs::directory_iterator(dir)) {
    auto filename = p.path().filename();
    auto ftime = fs::last_write_time(p);
    std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
    auto date_modified = std::asctime(std::localtime(&cftime));
    auto filesize = 0;
    std::string full_path = req_root / filename;
    if (fs::is_regular_file(p)) {
      filesize = fs::file_size(p);
    }
    dir_lines << "<tr><td><a href='" << full_path << "'>" << filename << "</a></td>";
    dir_lines << "<td>" << date_modified << "</td>";
    dir_lines << "<td>" << filesize << "</td></tr>";
  }
  page_inject["dirs"] = dir_lines.str();

  return Template::render_page(template_file, page_inject);
}

HTTPConnFSHandler create_static_handler(const std::string root_dir,
                                        const std::string template_file) {
  return [root_dir, template_file](HTTPConn &c) {
    fs::path root_path(root_dir);
    fs::path full_req_path(root_dir);
    full_req_path /= c.req_path;

    // std::cout << full_req_path << std::endl;

    if (!is_subdirectory(root_path, full_req_path)) {
      c.resp_status = 403;
      c.send();
      return fs::path();
    }

    // std::cout << c.req_path << std::endl;
    if (fs::is_directory(full_req_path)) {
      if (c.req_path.back() != '/') {
        // std::cout << "Rendering directory page" << std::endl;
        c.resp_headers["Content-Type"] = "text/html";
        c.resp_body << render_dir_page(full_req_path, c.req_path, template_file);
        c.send();
      } else if (fs::exists(full_req_path / "index.html")) {
        // std::cout << "Rendering index page" << std::endl;
        return (full_req_path / "index.html");
      }
    }

    return full_req_path;
  };
}

HTTPConnHandler create_cgi_handler(const std::string root_dir) {
  return [root_dir](HTTPConn &c) {
    fs::path root_path(root_dir);
    fs::path full_req_path(root_dir);
    full_req_path /= c.req_path;
    // std::cout << "Handling CGI" << std::endl;

    if (!is_subdirectory(root_path, full_req_path)) {
      c.resp_status = 403;
      c.send();
      return;
    }

    if (!fs::is_regular_file(full_req_path)) {
      not_found_handler(c);
      return;
    }

    std::stringstream ss;

    // Setup Header
    ss << c.http_ver << " " << c.resp_status << HTTPConn::default_status_reasons[c.resp_status]
       << "\r\n";

    const std::map<const std::string, const std::string> cgi_args = {
        {"SERVER_SOFTWARE", KLEPTIC_HTTP_SERVER_NAME},
        {"SERVER_NAME", c.host_ip},
        {"GATEWAY_INTERFACE", "CGI/1.1"},
        {"SERVER_PROTOCOL", c.http_protocol},
        {"SERVER_PORT", std::to_string(c.host_port)},
        {"REQUEST_METHOD", c.method},
        {"HTTP_ACCEPT", c.req_headers["Accept"]},
        {"PATH_INFO", ""},        // TODO ADD PATH INFO
        {"PATH_TRANSLATED", ""},  // TODO ADD PATH TRANSLATED
        {"SCRIPT_NAME", full_req_path.filename()},
        {"QUERY_STRING", c.raw_query_string},
        {"REMOTE_HOST", ""},  // TODO Reverse DNS Lookup
        {"REMOTE_ADDR", c.remote_ip},
        {"REMOTE_USER", c.user},
        {"AUTH_TYPE", c.auth_type},
        {"CONTENT_TYPE", c.req_headers["Content-Type"]},
        {"CONTENT_LENGTH", std::to_string(c.req_body.str().size())}
    };

    int pipe_sock[2];
    socketpair(PF_LOCAL, SOCK_STREAM, 0, pipe_sock);

    pid_t pid = fork();
    if (pid == 0) {
      // in child process
      for (auto &[key, val] : cgi_args) {
        if (val.compare("")) {
          setenv(key.c_str(), val.c_str(), true);
        }
      }
      close(pipe_sock[0]);
      dup2(pipe_sock[1], STDOUT_FILENO);
      dup2(pipe_sock[1], STDIN_FILENO);
      close(pipe_sock[1]);

      if (!full_req_path.extension().compare(".so")) {
        void *lib = dlopen(full_req_path.c_str(), RTLD_LAZY);

        if (lib == NULL) {
          perror("dlopen");
          exit(1);
        }

        cgi_func_ptr fn_handler = reinterpret_cast<cgi_func_ptr>(dlsym(lib, "httprun"));

        fn_handler(STDOUT_FILENO, c.raw_query_string.c_str());

      } else {
        dup2(pipe_sock[1], STDOUT_FILENO);
        close(pipe_sock[1]);
        execl(full_req_path.c_str(), full_req_path.filename().c_str());
      }
      exit(0);
    } else if (pid > 0) {
      close(pipe_sock[1]);

      write(pipe_sock[0], c.req_body.str().c_str(), c.req_body.str().size());
      char buffer[KLEPTIC_CGI_PIPE_BUFFER_SIZE];
      int read_size = KLEPTIC_CGI_PIPE_BUFFER_SIZE - 1;
      int ret = 0;

      do {
        ret = read(pipe_sock[0], buffer, read_size);
        if (ret == 0) {
          break;
        }
        buffer[ret] = '\0';
        // std::cout << buffer << std::endl;
        ss << std::string(buffer);
      } while (true);

      close(pipe_sock[0]);
    } else {
      perror("fork");
    }
    waitpid(pid, NULL, 0);

    c.send(ss.str());
  };
}

HTTPConnHandler derive_http_handler(HTTPConnFSHandler fshandler) {
  return [fshandler](HTTPConn &c) {
    auto path = fshandler(c);

    if (c.is_set()) {
      return;
    }

    if (!fs::exists(path)) {
      not_found_handler(c);
      return;
    }

    if (path.has_extension()) {
      std::string ext = path.extension();
      c.resp_headers["Content-Type"] = Util::get_content_type(ext.erase(0, 1));
    }

    std::ifstream file(path, std::ios::binary);
    c.resp_body << file.rdbuf();
    file.close();
  };
}
HTTPConnHandler derive_http_handler(HTTPConnHandler h) { return h; }

void not_found_handler(HTTPConn &c) {
  c.resp_status = 404;
  c.resp_headers["Content-Type"] = "text/plain";
  c.resp_body << "File not found";
  c.send();
}

}  // namespace Kleptic::Handler
