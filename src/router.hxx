#ifndef KLEPTIC_ROUTER_HXX_
#define KLEPTIC_ROUTER_HXX_

#include <map>
#include <string>
#include <vector>

#include "handler.hxx"
#include "http.hxx"

namespace Kleptic {

struct RouteNode {
  std::map<std::string, RouteNode> next;
  HTTPConnHandler get_handler = Handler::not_found_handler;
  HTTPConnHandler put_handler = Handler::not_found_handler;
  HTTPConnHandler post_handler = Handler::not_found_handler;
  HTTPConnHandler delete_handler = Handler::not_found_handler;
  bool endpoint = false;
  RouteNode() = default;
};

typedef std::vector<std::reference_wrapper<RouteNode>> route_matches;
typedef std::vector<std::string>::iterator path_iter;
class Router {
  std::vector<HTTPConnHandler> middleware;

  route_matches search_route(std::string);
  void search_route(std::vector<std::string>::iterator,
                               std::vector<std::string>::iterator,
                               std::reference_wrapper<RouteNode> r,
                               route_matches& matching);

  std::reference_wrapper<RouteNode> create_route(std::string);
  std::reference_wrapper<RouteNode> create_route(path_iter, path_iter,
                                                 std::reference_wrapper<RouteNode> r);
  RouteNode _root_node = {};

 public:
  Router() = default;
  // Router(RouteNode node);
  template <typename H>
  void r_get(std::string rt, H handler) {
    auto route = create_route(rt);
    route.get().get_handler = Handler::derive_http_handler(handler);
  }
  template <typename H>
  void r_put(std::string rt, H handler) {
    auto route = create_route(rt);
    route.get().put_handler = Handler::derive_http_handler(handler);
  }
  template <typename H>
  void r_post(std::string rt, H handler) {
    auto route = create_route(rt);
    route.get().post_handler = Handler::derive_http_handler(handler);
  }
  template <typename H>
  void r_delete(std::string rt, H handler) {
    auto route = create_route(rt);
    route.get().delete_handler = Handler::derive_http_handler(handler);
  }
  template <typename H>
  void add_middleware(H handler) {
    middleware.push_back(Handler::derive_http_handler(handler));
  }

  // TODO : Middleware handling

  HTTPConnHandler h_get(std::string);
  HTTPConnHandler h_put(std::string);
  HTTPConnHandler h_post(std::string);
  HTTPConnHandler h_delete(std::string);

  // TODO : Populate router unparsed path in conn
  void handle(HTTPConn&);
};
}  // namespace Kleptic

#endif  // KLEPTIC_ROUTER_HXX_
