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
  // kqueue 반복문
  for (int i = 0; i < events; ++i) {
    Connection *c = (Connection *)event_list_[i].udata;
    // evlist_flag를 통해 현재 kqueue 큐에 있는 이벤트의 상태를 확인

    // 해당 이벤트의 플래그가 EV_ERROR인 경우
    if (event_list_[i].flags & EV_ERROR) {
      Logger::logError(LOG_ALERT, "%d kevent() error on %d filter:%d", events, (int)event_list_[i].ident, (int)event_list_[i].filter);
      sm->closeConnection(c);
      continue;
      // 해당 이벤트의 플래그가 EVFILT_READ 인 경우 (해당 이벤트가 클라이언트로 부터 메세지 받아오는 상태)
    } else if (event_list_[i].filter == EVFILT_READ) {
      // EV_EOF == 클라이언트'가' 연결 종료의 의미인 EOF를 보냈을 때 ??
      // 3 hand shake랑 어떤 연관이 있나요?????
      if (event_list_[i].flags & EV_EOF) {
        Logger::logError(LOG_ALERT, "%d kevent() reported about an closed connection %d", events, (int)event_list_[i].ident);
        sm->closeConnection(c);
        continue;
      }
      // 아니라면 connection 내의 read_event를 실행
      c->process_read_event(this, sm);
      // 해당 이벤트의 플래그가 EVFILT_WRITE 인 경우 (해당 이벤트가 클라이언트로 메세지를 보내는 상태)
    } else if (event_list_[i].filter == EVFILT_WRITE) {
      // EV_EOF == 클라이언트'로' 연결종료의 의미인 EOF를 보냈을 때??
      // 3 hand shake랑 어떤 연관이 있나요?????
      if (event_list_[i].flags & EV_EOF) {
        Logger::logError(LOG_ALERT, "%d kevent() reported about an %d reader disconnects", events, (int)event_list_[i].ident);
        sm->closeConnection(c);
        continue;
      }
      // 아니라면 클라이언트로 쓰기 진행
      c->process_write_event(this, sm);
    }
  }
}

}  // namespace ft
