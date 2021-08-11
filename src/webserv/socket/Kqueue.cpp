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
  if (filter == (u_short)EVFILT_PROC) {
    std::cout << "ssup: [" << c->cgi_pid << "]" << std::endl;
    EV_SET(&change_list_[nchanges_], c->cgi_pid, filter, flags, NOTE_EXIT | NOTE_FORK | NOTE_EXEC | NOTE_SIGNAL, 0, c);  // udata = Connection
    ++nchanges_;
  } else {
    EV_SET(&change_list_[nchanges_], c->getFd(), filter, flags, 0, 0, c);  // udata = Connection
    ++nchanges_;
  }
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
      sm->closeConnection(c);
      continue;
    } else if (event_list_[i].filter == EVFILT_READ) {
      if (event_list_[i].flags & EV_EOF) {
        Logger::logError(LOG_ALERT, "%d kevent() reported about an closed connection %d", events, (int)event_list_[i].ident);
        sm->closeConnection(c);
        continue;
      }
      c->process_read_event(this, sm);
    } else if (event_list_[i].filter == EVFILT_WRITE) {
      if (event_list_[i].flags & EV_EOF) {
        Logger::logError(LOG_ALERT, "%d kevent() reported about an %d reader disconnects", events, (int)event_list_[i].ident);
        sm->closeConnection(c);
        continue;
      }
      c->process_write_event(this, sm);
    } else if (event_list_[i].filter == EVFILT_PROC) {
      if (c->getBodyBuf().size() == 0) {  //자식 프로세스로 보낼 c->body_buf_ 가 비어있는 경우 파이프 닫음
        close(c->writepipe[1]);
        read(c->readpipe[0], c->buffer_, BUF_SIZE);
        c->temp.append(c->buffer_);
      } else {
        // // TODO: 수정 필요
        size_t size = c->getBodyBuf().size();
        for (size_t i = 0; i < size; i += BUF_SIZE) {
          // std::cout << "i: [" << i << "]" << std::endl;
          size_t j = std::min(size, BUF_SIZE + i);
          write(c->writepipe[1], c->getBodyBuf().substr(i, j).c_str(), j - i);
          // c->getBodyBuf().erase(i, BUF_SIZE + i);
          if (i == 0) {
            read(c->readpipe[0], c->buffer_, BUF_SIZE);
            c->temp.append(c->buffer_);
            memset(c->buffer_, 0, BUF_SIZE);
          }
          read(c->readpipe[0], c->buffer_, BUF_SIZE);
          c->temp.append(c->buffer_);
          memset(c->buffer_, 0, BUF_SIZE);
          // std::cout << c->buffer_ << std::endl;
        }
        //숫자 확인
        c->setStringBufferContentLength(-1);
        c->getBodyBuf().clear();  // 뒤에서 또 쓰일걸 대비해 혹시몰라 초기화.. #2
        close(c->writepipe[1]);
      }
      c->setRecvPhase(MESSAGE_CGI_COMPLETE);

      // if (c->getRecvPhase()== MESSAGE_CGI_COMPLETE) {
      CgiHandler::handle_cgi_header(c);
      CgiHandler::setup_cgi_message(c);
      // }
      kqueueSetEvent(c, EVFILT_PROC, EV_DELETE);
      kqueueSetEvent(c, EVFILT_WRITE, EV_ADD);
      // if (event_list_[i].flags & EV_EOF) {
      //   Logger::logError(LOG_ALERT, "cgi error", events, (int)event_list_[i].ident);
      //   sm->closeConnection(c);
      //   continue;
      // }
    }
  }
}

}  // namespace ft
