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
        ssize_t recv_len = recv(c->getFd(), c->buffer_, BUF_SIZE, 0);
        if (c->getRecvPhase() == MESSAGE_START_LINE_INCOMPLETE ||
            c->getRecvPhase() == MESSAGE_START_LINE_COMPLETE ||
            c->getRecvPhase() == MESSAGE_HEADER_INCOMPLETE ||
            c->getRecvPhase() == MESSAGE_HEADER_COMPLETE) {
          MessageHandler::handle_request_header(c);
        }
        if (c->getRecvPhase() == MESSAGE_HEADER_PARSED) {
          if (!c->getRequest().getHeaderValue("Content-Length").empty())
            c->setStringBufferContentLength(stoi(c->getRequest().getHeaderValue("Content-Length")));
          MessageHandler::check_cgi_request(c);
          if (c->getRecvPhase() == MESSAGE_CGI_PROCESS)
            CgiHandler::init_cgi_child(c);
          else
            MessageHandler::check_body_status(c);
        } else if (c->getRecvPhase() == MESSAGE_BODY_INCOMING) {
          MessageHandler::handle_request_body(c);
        } else if (c->getRecvPhase() == MESSAGE_CGI_INCOMING) {
          CgiHandler::handle_cgi_body(c, recv_len);
        }
        if (c->getRecvPhase() == MESSAGE_BODY_COMPLETE || c->getRecvPhase() == MESSAGE_CGI_COMPLETE) {
          kqueueSetEvent(c, EVFILT_WRITE, EV_ADD | EV_ONESHOT);
        }
        memset(c->buffer_, 0, recv_len);
      }
    } else if (event_list_[i].filter == EVFILT_WRITE) {
      if (event_list_[i].flags & EV_EOF) {
        Logger::logError(LOG_ALERT, "%d kevent() reported about an %d reader disconnects", events, (int)event_list_[i].ident);
        sm->closeConnection(c);
      } else {
        // TODO: CGI 프로세스의 결과값을 읽어 오는 부분을
        // cgi 자식 프로세스에서 header만 읽어 와 처리
        if (c->getRecvPhase() == MESSAGE_CGI_COMPLETE) {
          std::cout << "am i even working" << std::endl;
          size_t nbytes;
          nbytes = read(c->readpipe[0], c->buffer_, BUF_SIZE);
          c->appendBodyBuf(c->buffer_);
          CgiHandler::handle_cgi_header(c);
          memset(c->buffer_, 0, nbytes);
        }
        MessageHandler::set_response_header(c);
        MessageHandler::set_response_body(c);
        MessageHandler::send_response_to_client(c);
        if (!c->getResponse().getHeaderValue("Connection").compare("close") ||
            !c->getRequest().getHttpVersion().compare("HTTP/1.0")) {
          sm->closeConnection(c);
        }
        c->clear();
      }
    }
  }
}

}  // namespace ft
