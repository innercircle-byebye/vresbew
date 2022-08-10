#include "webserv/socket/SocketManager.hpp"

namespace ft {
SocketManager::SocketManager(HttpConfig *httpconfig) {
  std::multimap<in_port_t, in_addr_t> addrs = httpconfig->getMustListens();
  for (std::multimap<in_port_t, in_addr_t>::iterator it = addrs.begin(); it != addrs.end(); ++it) {
    Listening *ls = new Listening(it->first, it->second);
    listening_.push_back(ls);
  }

  //connection socket들 할당
  connection_n_ = DEFAULT_CONNECTIONS;
  connections_ = new Connection[connection_n_]();
  Connection *c = connections_;
  Connection *next = NULL;
  for (int i = connection_n_ - 1; i >= 0; --i) {
    c[i].setNext(next);
    c[i].setFd(-1);
    c[i].setHttpConfig(httpconfig);
    // c[i].passHttpConfigToMessageHandler(httpconfig);
    next = &c[i];
  }
  free_connections_ = next;
  free_connection_n_ = connection_n_;

  openListeningSockets();
}

SocketManager::~SocketManager() {
  closeListeningSockets();
  delete[] connections_;
}

void SocketManager::openListeningSockets() {
  for (size_t i = 0; i < listening_.size(); ++i) {
      listening_[i]->setSocketFd();
      listening_[i]->bindSocket();
      listening_[i]->listenSocket();
      listening_[i]->setListeningConnection(getConnection(listening_[i]->getFd()));
  }
}

void SocketManager::closeListeningSockets() {
  for (size_t i = 0; i < listening_.size(); ++i) {
    Listening *ls = listening_[i];
    Connection *c = ls->getListeningConnection();

    if (c) {
      freeConnection(c);
      ls->setListeningConnection(NULL);
    }
    if (closeSocket(ls->getFd()) == -1) {
      Logger::logError(LOG_EMERG, "close() socket %s failed", ls->getAddrText().c_str());
      throw CloseSocketException();
    }
    delete ls;
  }
}

// free_connections에 있는 connection 하나 return
Connection *SocketManager::getConnection(socket_t s) {
  Connection *c;

  c = free_connections_;
  if (c == NULL) {
    Logger::logError(LOG_ALERT, "%u worker_connections are not enough", connection_n_);
    throw ConnNotEnoughException();
  }
  free_connections_ = c->getNext();
  --free_connection_n_;
  c->setFd(s);
  return c;
}

void SocketManager::freeConnection(Connection *c) {
  c->setNext(free_connections_);
  free_connections_ = c;
  ++free_connection_n_;
}

void SocketManager::closeConnection(Connection *c) {
  socket_t fd;

  freeConnection(c);
  fd = c->getFd();
  c->setFd(-1);
  c->clear();
  std::cout << "close socket fd:" << fd << std::endl;
  if (closeSocket(fd) == -1) {
    Logger::logError(LOG_ALERT, "close() socket %d failed", fd);
    throw CloseSocketException();
  }
}

const std::vector<Listening *> &SocketManager::getListening() const {
  return listening_;
}

size_t SocketManager::getListeningSize() const {
  return listening_.size();
}
}  // namespace ft
