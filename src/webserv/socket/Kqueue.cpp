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

void Kqueue::kqueueProcessEvents(SocketManager *sm) {
  int events = kevent(kq_, change_list_, nchanges_, event_list_, nevents_, &ts_);

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
        if (c->getRequest().getRecvPhase() != MESSAGE_CGI_PROCESS ||
            c->getRequest().getRecvPhase() != MESSAGE_CGI_INCOMING) {
          MessageHandler::handle_request(c);
        }
        if (c->getRequest().getRecvPhase() == MESSAGE_CGI_PROCESS) {
          ServerConfig *serverconfig_test = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
          LocationConfig *locationconfig_test = serverconfig_test->getLocationConfig(c->getRequest().getUri());
          MessageHandler::handle_cgi(c, locationconfig_test);
        }
        else if (c->getRequest().getRecvPhase() == MESSAGE_CGI_INCOMING) {
          std::cout << "i'm here" << std::endl;
          char foo[BUF_SIZE];  // 추후 수정 필요!!!

          close(c->writepipe[0]);
          close(c->readpipe[1]);
          if (!c->getRequest().getMsg().empty()) {
            write(c->writepipe[1], c->getRequest().getMsg().c_str(), static_cast<size_t>(c->getRequest().getMsg().size()));
            c->getRequest().getMsg().clear();
          } else {
            size_t recv_len = recv(c->getFd(), c->buffer_, BUF_SIZE, 0);
            std::cout << "buffer: " << c->buffer_ << std::endl;
            write(c->writepipe[1], c->buffer_, BUF_SIZE);
            memset(c->buffer_, 0, recv_len);
          }
          int nbytes;
          int i = 0;
          while ((nbytes = read(c->readpipe[0], foo, sizeof(foo)))) {
            c->cgi_output_temp.append(foo);
            i++;
            memset(foo, 0, nbytes);
          }
          wait(NULL);
          MessageHandler::process_cgi_response(c);
        }
        if (c->getRequest().getRecvPhase() == MESSAGE_BODY_COMPLETE) {
          //TODO: 전반적인 정리가 필요하다
          kqueueSetEvent(c, EVFILT_WRITE, EV_ADD | EV_ONESHOT);
        }
      }
    } else if (event_list_[i].filter == EVFILT_WRITE) {
      if (event_list_[i].flags & EV_EOF) {
        Logger::logError(LOG_ALERT, "%d kevent() reported about an %d reader disconnects", events, (int)event_list_[i].ident);
        sm->closeConnection(c);
      } else {
        MessageHandler::handle_response(c);
        if (!c->getResponse().getHeaderValue("Connection").compare("close") ||
            !c->getRequest().getHttpVersion().compare("HTTP/1.0")) {
          sm->closeConnection(c);
        }
        c->getRequest().clear();
        c->getResponse().clear();
      }
    }
  }
}
}  // namespace ft
