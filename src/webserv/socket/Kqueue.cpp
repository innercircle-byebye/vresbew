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
        if (c->getRequest().getRecvPhase() == MESSAGE_START_LINE_INCOMPLETE ||
            c->getRequest().getRecvPhase() == MESSAGE_START_LINE_COMPLETE ||
            c->getRequest().getRecvPhase() == MESSAGE_HEADER_INCOMPLETE ||
            c->getRequest().getRecvPhase() == MESSAGE_HEADER_COMPLETE ||
            c->getRequest().getRecvPhase() == MESSAGE_BODY_INCOMING ||
            c->getRequest().getRecvPhase() == MESSAGE_BODY_COMPLETE) {
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

          size_t recv_len;

          if (static_cast<int>(recv_len = recv(c->getFd(), c->buffer_, BUF_SIZE, 0)) == -1)
            break;
          // Transfer-Encoding : chunked 아닐 때 (= Content-Length가 있을 때)
          if (!c->getRequest().getHeaderValue("Content-Length").empty()) {
            if (c->getRequest().getBufferContentLength() > static_cast<int>(recv_len)) {
              // std::cout << "content-length before: " << c->getRequest().getBufferContentLength() << std::endl;
              c->getRequest().setBufferContentLength(c->getRequest().getBufferContentLength() - recv_len);
              write(c->writepipe[1], c->buffer_, recv_len);
              // std::cout << "content-length after: " << c->getRequest().getBufferContentLength() << std::endl;
            } else {
              // std::cout << "==========one============ " << std::endl;
              // std::cout << "content-length: " << c->getRequest().getBufferContentLength() << std::endl;
              // std::cout << "buffer: " << c->buffer_ << std::endl;
              // std::cout << "recv_len: " << static_cast<int>(recv_len) << std::endl;
              // std::cout << "==========one============ " << std::endl;
              write(c->writepipe[1], c->buffer_, recv_len);
              c->getRequest().setBufferContentLength(0);
              c->getRequest().setRecvPhase(MESSAGE_CGI_COMPLETE);
              close(c->writepipe[1]);
            }
          }
          // Request에서 Content-Length 헤더가 주어지지 않을 때
          // (= Transfer-Encoding: chunked 헤더가 주어 젔을 때)
          else {
            write(c->writepipe[1], c->buffer_, recv_len);

            std::cout << "buffer: " << c->buffer_ << std::endl;
            if (!strcmp(c->buffer_, "0\r\n")) {
              std::cout << "yatta" << std::endl;
              c->chunked_checker = CHUNKED_ZERO_RN_RN;
              std::cout << "chunked_checker: " << c->chunked_checker << std::endl;
            } else if (c->chunked_checker == CHUNKED_ZERO_RN_RN && !strcmp(c->buffer_, "\r\n")) {
              c->getRequest().setRecvPhase(MESSAGE_CGI_COMPLETE);
              close(c->writepipe[1]);
            } else
              c->chunked_checker = CHUNKED_KEEP_COMING;
          }
          memset(c->buffer_, 0, recv_len);
        }
        if (c->getRequest().getRecvPhase() == MESSAGE_BODY_COMPLETE || c->getRequest().getRecvPhase() == MESSAGE_CGI_COMPLETE) {
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
        // TODO: CGI 프로세스의 결과값을 읽어 오는 부분을
        // event queue, fd 와 연계해서 처리
        // responsehandler와 현재 엮여 있어 정신 맑을때 해야함...
        if (c->getRequest().getRecvPhase() == MESSAGE_CGI_COMPLETE) {
          int nbytes;
          int i = 0;
          while ((nbytes = read(c->readpipe[0], c->buffer_, BUF_SIZE))) {
            c->cgi_output_temp.append(c->buffer_);
            i++;
            memset(c->buffer_, 0, nbytes);
          }
          wait(NULL);
          MessageHandler::process_cgi_response(c);
        }
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
