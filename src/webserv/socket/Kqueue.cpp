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
        // 1. recv
        size_t recv_len = recv(c->getFd(), c->buffer_, BUF_SIZE, 0);
        if (c->getRecvPhase() == MESSAGE_START_LINE_INCOMPLETE ||
            c->getRecvPhase() == MESSAGE_START_LINE_COMPLETE ||
            c->getRecvPhase() == MESSAGE_HEADER_INCOMPLETE ||
            c->getRecvPhase() == MESSAGE_HEADER_COMPLETE) {
          MessageHandler::handle_request_header(c);
          if (c->getRecvPhase() == MESSAGE_HEADER_PARSED) {
            if (!c->getRequest().getHeaderValue("Content-Length").empty())
              c->setStringBufferContentLength(stoi(c->getRequest().getHeaderValue("Content-Length")));
            MessageHandler::check_cgi_request(c);
            if (c->getRecvPhase() == MESSAGE_CGI_PROCESS)
              MessageHandler::init_cgi_child(c);
            else
              MessageHandler::check_body_status(c);
          }
        } else if (c->getRecvPhase() == MESSAGE_BODY_INCOMING) {
          MessageHandler::handle_request_body(c);
        } else if (c->getRecvPhase() == MESSAGE_CGI_INCOMING) {
          std::cout << "i'm here" << std::endl;
          // 한번의 버퍼 안에 전체 메세지가 다 들어 올 경우

          if (static_cast<int>(recv_len) == -1)
            break;
          // Transfer-Encoding : chunked 아닐 때 (= Content-Length가 있을 때)
          if (c->getStringBufferContentLength() > 0) {
            if (c->getStringBufferContentLength() > static_cast<int>(recv_len)) {
              c->setStringBufferContentLength(c->getStringBufferContentLength() - recv_len);
              write(c->writepipe[1], c->buffer_, recv_len);
            } else {
              write(c->writepipe[1], c->buffer_, recv_len);
              c->setStringBufferContentLength(0);
              c->setRecvPhase(MESSAGE_CGI_COMPLETE);
              close(c->writepipe[1]);
            }
          }
          // Request에서 Content-Length 헤더가 주어지지 않을 때
          // (= Transfer-Encoding: chunked 헤더가 주어 젔을 때)
          else {
            if (!c->getRequest().getMsg().empty()) {
              std::cout << "hihi" << std::endl;
              size_t pos;

              while (c->getRequest().getMsg().find("\r\n") != std::string::npos) {
                pos = c->getRequest().getMsg().find("\r\n");
                std::string temp_msg = c->getRequest().getMsg().substr(0, pos + 2);
                std::cout << "tempsize: " << pos << std::endl;

                std::cout << "before: " << temp_msg << std::endl;
                write(c->writepipe[1], temp_msg.c_str(), (size_t)temp_msg.size());
                if (temp_msg == "0\r\n") {
                  c->chunked_checker = CHUNKED_ZERO_RN_RN;
                  // } else if (c->getRequest().getMsg().substr(0, pos) == "0\r\n\r\n") {
                  //   c->getRequest().setRecvPhase(MESSAGE_CGI_COMPLETE);
                  //   close(c->writepipe[1]);
                } else if (c->chunked_checker == CHUNKED_ZERO_RN_RN && temp_msg == "\r\n") {
                  c->setRecvPhase(MESSAGE_CGI_COMPLETE);
                  close(c->writepipe[1]);
                } else {
                  c->chunked_checker = CHUNKED_KEEP_COMING;
                }
                c->getRequest().getMsg().erase(0, pos + 2);
                temp_msg.clear();
                // std::cout << "after: " << c->getRequest().getMsg() << std::endl;
              }
              c->getRequest().getMsg().clear();
            }
            if (c->chunked_checker == CHUNKED_ZERO_RN_RN) {
              std::cout << "hi" << std::endl;
            }
            std::cout << "aftermath: " << c->buffer_ << std::endl;
            write(c->writepipe[1], c->buffer_, recv_len);
            if (!strcmp(c->buffer_, "0\r\n")) {
              c->chunked_checker = CHUNKED_ZERO_RN_RN;
              // } else if (!strcmp(c->buffer_, "0\r\n\r\n")) {
              //   c->getRequest().setRecvPhase(MESSAGE_CGI_COMPLETE);
              //   close(c->writepipe[1]);
            } else if (c->chunked_checker == CHUNKED_ZERO_RN_RN && !strcmp(c->buffer_, "\r\n")) {
              c->setRecvPhase(MESSAGE_CGI_COMPLETE);
              close(c->writepipe[1]);
            } else
              c->chunked_checker = CHUNKED_KEEP_COMING;
          }
        }
        if (c->getRecvPhase() == MESSAGE_BODY_COMPLETE || c->getRecvPhase() == MESSAGE_CGI_COMPLETE) {
          //TODO: 전반적인 정리가 필요하다
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
          MessageHandler::process_cgi_header(c);
          // ***********************
          // cgi 에러 처리를 여기서??
          // **********************
          memset(c->buffer_, 0, nbytes);
        }
        // MessageHandler::handle_response(c);
        // TODO: 리네임 (이 함수에서 서버 내부 동작도 처리함)
        MessageHandler::set_response_header(c);

        // TODO:
        MessageHandler::send_response_to_client(c);

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
