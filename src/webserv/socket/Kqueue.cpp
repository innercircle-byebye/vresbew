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
            c->getRequest().getRecvPhase() != MESSAGE_CGI_INCOMING ||
            c->getRequest().getRecvPhase() != MESSAGE_CGI_COMPLETE) {
          MessageHandler::handle_request(c);
        }
        if (c->getRequest().getRecvPhase() == MESSAGE_CGI_PROCESS) {
          ServerConfig *serverconfig_test = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
          LocationConfig *locationconfig_test = serverconfig_test->getLocationConfig(c->getRequest().getUri());
          MessageHandler::handle_cgi(c, locationconfig_test);
        }
        if (c->getRequest().getRecvPhase() == MESSAGE_CGI_INCOMING) {
          std::cout << "i'm here" << std::endl;
          // 한번의 버퍼 안에 전체 메세지가 다 들어 올 경우
          // 한번의 클라이언트 kevent에서는 버퍼를 읽어 올 게 없음...
          if (strlen(c->buffer_) < 0)
            return;
          else {
            size_t recv_len = recv(c->getFd(), c->buffer_, BUF_SIZE, 0);

            std::cout << "content-length: " << c->getRequest().getBufferContentLength() << std::endl;
            std::cout << "buffer: " << c->buffer_ << std::endl;
            std::cout << "recv_len: " << static_cast<int>(recv_len) << std::endl;
            if ((size_t)c->getRequest().getBufferContentLength() <= recv_len) {
              write(c->writepipe[1], c->buffer_, recv_len - c->getRequest().getBufferContentLength());
              c->getRequest().setBufferContentLength(0);
              c->getRequest().setRecvPhase(MESSAGE_CGI_COMPLETE);
            } else {
              c->getRequest().setBufferContentLength(c->getRequest().getBufferContentLength() - recv_len);
              write(c->writepipe[1], c->buffer_, recv_len);
            }
            memset(c->buffer_, 0, recv_len);
          }
          // }
        }
        if (c->getRequest().getRecvPhase() == MESSAGE_CGI_COMPLETE) {
          char foo[BUF_SIZE];
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
          memset(c->buffer_, 0, sizeof(c->buffer_));
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
