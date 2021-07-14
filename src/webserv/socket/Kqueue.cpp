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
  int events;
  size_t recv_len;

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
        recv_len = recv(event_list_[i].ident, c->buffer_, BUF_SIZE, 0);

        if (c->message_handler_.recv_phase == MESSAGE_HEADER_INCOMPLETE) {
          c->message_handler_.checkBufferForHeader(c->buffer_);
        }
        if (c->message_handler_.recv_phase == MESSAGE_HEADER_COMPLETE ||
            c->message_handler_.recv_phase == MESSAGE_BODY_INCOMING) {
          std::cout << c->buffer_ << std::endl;
          c->message_handler_.handleBody(c->buffer_);
        }
        if (c->message_handler_.recv_phase == MESSAGE_BODY_NO_NEED)
          kqueueSetEvent(c, EVFILT_WRITE, EV_ADD | EV_ONESHOT);
        if (c->message_handler_.recv_phase == MESSAGE_BODY_COMPLETE) {
          c->message_handler_.setContentLengthToZero();
          c->message_handler_.checkRemainderBufferForHeader(c->buffer_);
          kqueueSetEvent(c, EVFILT_WRITE, EV_ADD | EV_ONESHOT);
        }
        memset(c->buffer_, 0, BUF_SIZE);
      }
    } else if (event_list_[i].filter == EVFILT_WRITE) {
      if (event_list_[i].flags & EV_EOF) {
        Logger::logError(LOG_ALERT, "%d kevent() reported about an %d reader disconnects", events, (int)event_list_[i].ident);
        sm->closeConnection(c);
      } else {
        if (c->request_.uri.size() > 0) {
          std::cout << c->getFd() << " can write!" << std::endl;
          c->message_handler_.createResponse();
          send(c->getFd(), (*c->message_handler_.response_header_buf_).c_str(),
               static_cast<size_t>((*c->message_handler_.response_header_buf_).size()), 0);
          // TODO: 언제 삭제해야하는지 적절한 시기를 확인해야함
          c->request_.headers.clear();
          c->request_.uri.clear();
          c->request_.method.clear();
          std::cout << c->getFd() << " write end." << std::endl;
          if (c->message_handler_.getResponseStatus() == "404" || c->request_.version == "HTTP/1.0")
            sm->closeConnection(c);
          else {
            c->message_handler_.clearMsgHeaderBuf();
            c->message_handler_.clearMsgBodyBuf();
            c->message_handler_.clearResponseHeaderBuf();
            c->message_handler_.recv_phase = MESSAGE_HEADER_INCOMPLETE;
            kqueueSetEvent(c, EVFILT_READ, EV_ADD);
          }
        }
      }
    }
  }
}
}  // namespace ft
