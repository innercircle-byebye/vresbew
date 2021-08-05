#include "webserv/socket/Kqueue.hpp"
namespace ft {
Kqueue::Kqueue() {
  kq_ = kqueue();
  if (kq_ == -1) {
    Logger::logError(LOG_EMERG, "kqueue() failed");
    throw KqueueException();
  }
  nchanges_ = 0;
  max_changes_ = DEFAULT_CONNECTIONS;
  nevents_ = DEFAULT_CONNECTIONS;
  change_list_ = new struct kevent[max_changes_]();
  if (change_list_ == NULL) {
    Logger::logError(LOG_EMERG, "malloc(%d) failed", max_changes_);
    throw std::bad_alloc();
  }
  event_list_ = new struct kevent[max_changes_]();
  if (event_list_ == NULL) {
    Logger::logError(LOG_EMERG, "malloc(%d) failed", max_changes_);
    throw std::bad_alloc();
  }
  ts_.tv_sec = 5;
  ts_.tv_nsec = 0;
}
Kqueue::~Kqueue() {
  if (close(kq_) == -1) {
    Logger::logError(LOG_ALERT, "kqueue close() failed");
  }
  kq_ = -1;
  delete[] change_list_;
  delete[] event_list_;
}
void Kqueue::kqueueSetEvent(Connection *c, u_short filter, u_int flags) {
  EV_SET(&change_list_[nchanges_], c->getFd(), filter, flags, 0, 0, c);  // udata = Connection
  ++nchanges_;
}
#define READ 0
#define WRITE 1
void Kqueue::kqueueProcessEvents(SocketManager *sm) {
  int events;
  MessageHandler message_handler;
  /* add start */
  pid_t pid;
  int fd_a[2], fd_b[2];
  /* add end */
  events = kevent(kq_, change_list_, nchanges_, event_list_, nevents_, &ts_);
  nchanges_ = 0;
  if (events == -1) {
    Logger::logError(LOG_ALERT, "kevent() failed");
    throw KeventException();
  }
  for (int i = 0; i < events; ++i) {
    Connection *c = (Connection *)event_list_[i].udata;
    if (event_list_[i].flags & EV_ERROR) {
      Logger::logError(LOG_ALERT, "%d kevent() error on %d filter:%d", events, (int)event_list_[i].ident, (int)event_list_[i].filter);
      continue;
    } else if (event_list_[i].filter == EVFILT_READ) {
      if (c->getListen()) {
        Connection *conn = c->eventAccept(sm);  // throw
        kqueueSetEvent(conn, EVFILT_READ, EV_ADD);
      } else if (event_list_[i].flags & EV_EOF) {
        Logger::logError(LOG_ALERT, "%d kevent() reported about an closed connection %d", events, (int)event_list_[i].ident);
        sm->closeConnection(c);
      } else {
        message_handler.handle_request(c);
        if ((c->getRequest().getRecvPhase() == MESSAGE_BODY_NO_NEED) || (c->getRequest().getRecvPhase() == MESSAGE_BODY_COMPLETE)) {
          kqueueSetEvent(c, EVFILT_WRITE, EV_ADD | EV_ONESHOT);
        }
      }
    } else if (event_list_[i].filter == EVFILT_WRITE) {
      if (event_list_[i].flags & EV_EOF) {
        Logger::logError(LOG_ALERT, "%d kevent() reported about an %d reader disconnects", events, (int)event_list_[i].ident);
        sm->closeConnection(c);
      } else {
        if (c->getRequest().getUri().size() > 0) {
          message_handler.handle_response(c);
          if (!c->getResponse().getStatusCode().compare("404") || !c->getRequest().getHttpVersion().compare("HTTP/1.0"))
            sm->closeConnection(c);
        }
      }
    }
    /* add start */
    if (c->getRequest().getRecvPhase() == MESSAGE_CGI) {
      if (pipe(fd_a) < 0) {
        Logger::log_error("pipe error");
        throw PipeException();
      }
      if (pipe(fd_b) < 0) {
        Logger::log_error("pipe error");
        throw PipeException();
      }
      if ((pid = fork()) < 0) {
        Logger::log_error("fork error");
        throw ForkException();
      }
      if (pid > 0) {
        close(fd_a[READ]);
        close(fd_b[WRITE]);
      }
      else {
        close(fd_a[WRITE]);
        close(fd_b[READ]);
      }
    }
    /* add end */
  }
}
}  // namespace ft
