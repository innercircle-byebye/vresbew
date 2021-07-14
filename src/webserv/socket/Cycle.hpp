#ifndef CYCLE_HPP
#define CYCLE_HPP

#include "webserv/config/HttpConfig.hpp"
#include "webserv/socket/Kqueue.hpp"
#include "webserv/socket/SocketManager.hpp"
#include "webserv/webserv.hpp"

namespace ft {
class Cycle {
 private:
  Kqueue *kq_;
  SocketManager *sm_;

 public:
  Cycle(HttpConfig *httpconfig);
  ~Cycle();

  void webservCycle();
};
}  // namespace ft
#endif
