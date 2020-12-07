#ifndef KLEPTIC_APP_HXX_
#define KLEPTIC_APP_HXX_

#include "http.hxx"
#include "router.hxx"

namespace Kleptic {
class App {
  App() = default;

 public:
  Router r;
  HTTPConnHandler get_handler();
};
}  // namespace Kleptic

#endif  // KLEPTIC_APP_HXX_
