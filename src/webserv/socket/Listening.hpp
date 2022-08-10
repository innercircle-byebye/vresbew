#ifndef LISTENING_HPP
#define LISTENING_HPP

#include <arpa/inet.h>

#include "webserv/socket/Connection.hpp"
#include "webserv/webserv.hpp"

namespace ft {
class Listening {
 private:
  socket_t fd_;

  struct sockaddr_in sockaddr_;
  socklen_t socklen_;
  std::string addr_text_;

  int type_;
  int backlog_;

  Connection *connection_;

 public:
  Listening(in_port_t port, in_addr_t ipaddr);
  ~Listening();

  void setSocketFd();
  void bindSocket();
  void listenSocket();

  void setListeningConnection(Connection *c);

  Connection *getListeningConnection() const;
  socket_t getFd() const;
  const std::string &getAddrText() const;
};
}  // namespace ft
#endif
