#ifndef KLEPTIC_ERROR_HXX_
#define KLEPTIC_ERROR_HXX_

// TODO MOVE INTO CXX

#include <exception>
#include <string>

namespace Kleptic {
class SocketException : public std::exception {
 protected:
  std::string err_msg;

 public:
  explicit SocketException(std::string msg) : err_msg(msg) {}

  virtual const char *what() const throw() { return err_msg.c_str(); }
};

class ParseException : public std::exception {
 protected:
  std::string err_msg;

 public:
  int err_code;
  explicit ParseException(std::string msg, int http_code = 400)
      : err_msg(msg), err_code(http_code) {}

  virtual const char *what() const throw() { return err_msg.c_str(); }
};

class TemplateException : public std::exception {
 protected:
  std::string err_msg;
  std::string template_path;

 public:
  int err_code;
  TemplateException(std::string msg, std::string path) : err_msg(msg), template_path(path) {}

  virtual const char *what() const throw() {
    return ("Template " + template_path + " : " + err_msg).c_str();
  }
};

}  // namespace Kleptic

#endif  // KLEPTIC_ERROR_HXX_
