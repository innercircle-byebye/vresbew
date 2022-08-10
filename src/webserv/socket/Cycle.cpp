#include "webserv/socket/Cycle.hpp"

namespace ft {
Cycle::Cycle(HttpConfig *httpconfig) {
  if (access("error.log", F_OK) == -1) {
    std::ofstream ofs("error.log");
    if (!ofs.is_open())
      throw FileOpenException();
    ofs.close();
  }

  kq_ = new Kqueue();
  sm_ = new SocketManager(httpconfig);
}

Cycle::~Cycle() {
  delete kq_;
  delete sm_;
}

void Cycle::webservCycle() {
  // 생성한 listening socket들 kqueue에 ADD
  for (size_t i = 0; i < sm_->getListeningSize(); ++i) {
    kq_->kqueueSetEvent(sm_->getListening()[i]->getListeningConnection(), EVFILT_READ, EV_ADD);
  }

  for (;;) {
    try {
      kq_->kqueueProcessEvents(sm_);
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }
}
}  // namespace ft
