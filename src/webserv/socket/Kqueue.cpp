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
        //std::cout << "accept chunked_chcker: " << c->chunked_checker_ << std::endl;
        ssize_t recv_len = recv(c->getFd(), c->buffer_, BUF_SIZE, 0);
        if (c->getRecvPhase() == MESSAGE_INTERRUPTED) {
          if (strchr(c->buffer_, ctrl_c[0])) {
            c->interrupted = true;
            c->setRecvPhase(MESSAGE_BODY_COMPLETE);
          }
        }
        // TODO: remove; for debug
        // std::cout << "=========c->buffer_=========" << std::endl;
        // std::cout << c->buffer_ << std::endl;
        // std::cout << "=========c->buffer_=========" << std::endl;
        if (c->getRecvPhase() == MESSAGE_START_LINE_INCOMPLETE ||
            c->getRecvPhase() == MESSAGE_START_LINE_COMPLETE ||
            c->getRecvPhase() == MESSAGE_HEADER_INCOMPLETE ||
            c->getRecvPhase() == MESSAGE_CHUNKED) {
          MessageHandler::handle_request_header(c);
        }
        if (c->getRecvPhase() == MESSAGE_HEADER_COMPLETE) {
          MessageHandler::check_request_header(c);
        } else if (c->getRecvPhase() == MESSAGE_BODY_INCOMING) {
          MessageHandler::handle_request_body(c);
        } else if (c->getRecvPhase() == MESSAGE_CGI_INCOMING) {
          CgiHandler::receive_cgi_process_body(c, recv_len);
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
        if (c->interrupted == true) {
          sm->closeConnection(c);
          c->clear();
        } else {
          if (c->getRecvPhase() == MESSAGE_CGI_COMPLETE) {
            CgiHandler::handle_cgi_header(c);
            if (c->getRequest().getMethod() == "POST" &&
                c->getRequest().getHeaderValue("Content-Length").empty()) {
              CgiHandler::send_chunked_cgi_response_to_client_and_close(c);
              // sm->closeConnection(c);때문에 여기에 놔둠
              if (!c->getResponse().getHeaderValue("Connection").compare("close") ||
                  !c->getRequest().getHttpVersion().compare("HTTP/1.0")) {
                sm->closeConnection(c);
              }
              c->clear();
              continue;
            } else
              CgiHandler::receive_cgi_body(c);
          }
          MessageHandler::execute_server_side(c);  // 서버가 실제 동작을 진행하는 부분
          MessageHandler::set_response_message(c);
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
}

}  // namespace ft
