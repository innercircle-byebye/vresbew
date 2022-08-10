#include "webserv/socket/Listening.hpp"

namespace ft {
Listening::Listening(in_port_t port, in_addr_t ipaddr)
    : fd_(-1), type_(SOCK_STREAM), backlog_(LISTEN_BACKLOG), connection_(NULL) {
  sockaddr_.sin_family = AF_INET;
  sockaddr_.sin_port = port;
  sockaddr_.sin_addr.s_addr = ipaddr;

  socklen_ = sizeof(sockaddr_);

  std::stringstream ss;
  char buf[32] = {
      0,
  };
  ss << inet_ntop(AF_INET, &ipaddr, buf, sizeof(buf));
  ss << ":";
  ss << ntohs(port);
  addr_text_ = ss.str();
}

Listening::~Listening() {}

void Listening::setSocketFd() {
  fd_ = socket(sockaddr_.sin_family, type_, 0);
  if (fd_ < 0) {
    Logger::logError(LOG_EMERG, "socket() %s failed", addr_text_.c_str());
    throw SocketException();
  }

  int sock_optval = 1;
  if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &sock_optval, sizeof(sock_optval)) == -1) {
    Logger::logError(LOG_EMERG, "setsockopt() to %s failed", addr_text_.c_str());
  }
}

void Listening::bindSocket() {
  if (bind(fd_, (struct sockaddr *)&sockaddr_, socklen_) == -1) {
    Logger::logError(LOG_EMERG, "bind() to %s failed", addr_text_.c_str());
    if (closeSocket(fd_) == -1) {
      Logger::logError(LOG_EMERG, "close() socket %s failed", addr_text_.c_str());
      throw CloseSocketException();
    }
    throw BindException();
  }
}

void Listening::listenSocket() {
  if (listen(fd_, backlog_) == -1) {
    Logger::logError(LOG_EMERG, "listen() to %s failed", addr_text_.c_str());
    if (closeSocket(fd_) == -1) {
      Logger::logError(LOG_EMERG, "close() socket %s failed", addr_text_.c_str());
      throw CloseSocketException();
    }
    throw ListenException();
  }
}

void Listening::setListeningConnection(Connection *c) {
  c->setListen(true);
  c->setType(type_);
  c->setListening(this);
  c->setSockaddrToConnectPort(sockaddr_.sin_port);
  c->setSockaddrToConnectIP(sockaddr_.sin_addr.s_addr);
  connection_ = c;
}

Connection *Listening::getListeningConnection() const {
  return connection_;
}

socket_t Listening::getFd() const {
  return fd_;
}

const std::string &Listening::getAddrText() const {
  return addr_text_;
}
}  // namespace ft
