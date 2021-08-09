#ifndef KQUEUE_HPP
#define KQUEUE_HPP

#include <cstdlib>
#include <map>
#include <string>

#include "webserv/message/handler/CgiHandler.hpp"
#include "webserv/message/handler/MessageHandler.hpp"
#include "webserv/socket/Connection.hpp"
#include "webserv/socket/SocketManager.hpp"
#include "webserv/webserv.hpp"

namespace ft {
class Kqueue {
 private:
  int kq_, max_changes_, nchanges_, nevents_;
  struct kevent *change_list_;
  struct kevent *event_list_;
  struct timespec ts_;

 public:
  Kqueue();
  ~Kqueue();

  void kqueueSetEvent(Connection *c, u_short filter, u_int flags);
  void kqueueProcessEvents(SocketManager *sm);
};
}  // namespace ft
#endif
