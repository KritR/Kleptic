#include "router.hxx"

#include <experimental/filesystem>

#include <algorithm>
#include <string>
#include <vector>
#include <utility>

namespace fs = std::experimental::filesystem;

namespace Kleptic {

// Router::Router(RouteNode node) : _root_node(node) {}

auto route_to_vec(std::string rt) {
  if (rt.back() == '/') {
    rt.pop_back();
  }

  fs::path r_path(rt);

  std::vector<std::string> path_vec;
  while (r_path != fs::path()) {
    path_vec.push_back(r_path.filename());
    r_path = r_path.parent_path();
  }
  if (path_vec.size() > 0) {
    path_vec.pop_back();  // remove extraneous "/"
  }
  std::reverse(path_vec.begin(), path_vec.end());

  return path_vec;
}

route_matches Router::search_route(std::string rt) {
  auto path_vec = route_to_vec(rt);
  route_matches matching;
  search_route(path_vec.begin(), path_vec.end(), std::ref(_root_node),
                          matching);
  return matching;
}

void Router::search_route(std::vector<std::string>::iterator path,
                                     std::vector<std::string>::iterator path_end,
                                     std::reference_wrapper<RouteNode> node,
                                     route_matches& matching) {
  if (node.get().endpoint) {
    matching.push_back(node);
  }

  if (path == path_end) {
    return;
  }

  if (node.get().next.find(*path) == node.get().next.end()) {
    return;
  }

  search_route(path + 1, path_end, std::ref(node.get().next[*path]), matching);
}

std::reference_wrapper<RouteNode> Router::create_route(std::string rt) {
  auto path_vec = route_to_vec(rt);
  return create_route(path_vec.begin(), path_vec.end(), std::ref(_root_node));
}

std::reference_wrapper<RouteNode> Router::create_route(path_iter path, path_iter path_end,
                                                       std::reference_wrapper<RouteNode> node) {
  if (path == path_end) {
    node.get().endpoint = true;
    return node;
  }
  return create_route(path + 1, path_end, node.get().next[*path]);
}

HTTPConnHandler Router::h_get(std::string rt) {
  auto matches = search_route(rt);
  if (matches.size() == 0) {
    return Handler::not_found_handler;
  }
  return matches.back().get().get_handler;
}

HTTPConnHandler Router::h_put(std::string rt) {
  auto matches = search_route(rt);
  if (matches.size() == 0) {
    return Handler::not_found_handler;
  }
  return matches.back().get().put_handler;
}

HTTPConnHandler Router::h_post(std::string rt) {
  auto matches = search_route(rt);
  if (matches.size() == 0) {
    return Handler::not_found_handler;
  }
  return matches.back().get().post_handler;
}

HTTPConnHandler Router::h_delete(std::string rt) {
  auto matches = search_route(rt);
  if (matches.size() == 0) {
    return Handler::not_found_handler;
  }
  return matches.back().get().delete_handler;
}

void Router::handle(HTTPConn& c) {
  if (c.is_set()) {
    return;
  }
  for (const auto& h : middleware) {
    h(c);
    if (c.is_set()) {
      return;
    }
  }
  if (!c.method.compare("GET")) {
    h_get(c.req_path)(c);
  } else if (!c.method.compare("POST")) {
    h_post(c.req_path)(c);
  } else if (!c.method.compare("PUT")) {
    h_put(c.req_path)(c);
  } else if (!c.method.compare("DELETE")) {
    h_delete(c.req_path)(c);
  }
}

}  // namespace Kleptic
