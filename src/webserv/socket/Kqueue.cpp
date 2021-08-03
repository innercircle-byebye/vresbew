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
            c->getRecvPhase() == MESSAGE_HEADER_COMPLETE) {
          MessageHandler::handle_request_header(c);
        }
        if (c->getRecvPhase() == MESSAGE_HEADER_PARSED) {
          // LOCATIONCONFIG 선언도 여기에서
          // 적어도 400번대 에러는 여기에서 모두 처리 할 수 있도록!!!!
          // (함수는 원하는대로 생성 하고 리팩토링은 이후에...)
          // *** 필수 헤더에 따라서 어떤 동작을 해야되는지 파악 해야함
          // (i.e. Content-Language, Content-type, Transfer-Encoding  등등... ) <- 어떻게 해야 할지 조사 필요
          // 적어도 필수 헤더는 다 파악이 필요함. <- 어떻게 해야 할지 조사 필요

          // REQUEST_CHECK #6
          // #2~#4 의 과정을 request header 까지 완성 한 후 현재 시점에서 진행
          // + locationconfig에서 확인 할 수 있는 정보를 통해 조건 비교
          // + responsehandler에서 이전 해 와야 할 부분들 확인 필요

          // REQUEST_CHECK #7
          // 헤더가 완성 된 이후에 request_message를
          // 전체적으로 확인하는 코드가 있으면 한번에 관리하는게 나을듯 합니다.
          if (!c->getRequest().getHeaderValue("Content-Length").empty())
            c->setStringBufferContentLength(stoi(c->getRequest().getHeaderValue("Content-Length")));
          // TODO: remove
          // 현재 시점에 유지 해주세요
          if (c->interrupted == true)
            c->setRecvPhase(MESSAGE_INTERRUPTED);
          else {
            // REQUEST_CHECK #7
            // 헤더가 완성 된 이후에 request_message를
            // 전체적으로 확인하는 코드가 있으면 한번에 관리하는게 나을듯 합니다.
            // 아래 부분은 구현은 잘 되어 있으니 따로 건드릴 필요는 없음!!!
            MessageHandler::check_cgi_request(c);
            if (c->getRecvPhase() == MESSAGE_CGI_PROCESS)
              CgiHandler::init_cgi_child(c);
            else
              MessageHandler::check_body_status(c);
          }
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
          c->clear();
          sm->closeConnection(c);
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
          MessageHandler::set_response_header(c);  // 서버가 실제 동작을 진행하는 부분
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
