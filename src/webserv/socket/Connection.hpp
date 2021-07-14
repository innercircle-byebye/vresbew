#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "webserv/logger/Logger.hpp"
#include "webserv/message/handler/MessageHandler.hpp"
#include "webserv/socket/Listening.hpp"
#include "webserv/webserv.hpp"
#include "webserv/socket/SocketManager.hpp"

namespace ft {

class Connection {
 private:
  bool listen_;

  socket_t fd_;
  int type_;
  struct sockaddr_in sockaddr_to_connect_;
  socklen_t socklen_to_connect_;

  Listening *listening_;

  HttpConfig *httpconfig_;

  Connection *next_;

 public:
  char buffer_[BUF_SIZE];

  MessageHandler message_handler_;
  Response	response_;
  Request	request_;

  Connection();
  ~Connection();

  Connection *eventAccept(SocketManager *sv);

  void setListen(bool listen);
  void setNext(Connection *next);
  void setFd(socket_t fd);
  void setType(int type);
  void setListening(Listening *listening);
  void setSockaddrToConnectPort(in_port_t port);
  void setSockaddrToConnectIP(in_addr_t ipaddr);
  void setHttpConfig(HttpConfig *httpconfig);
//   void passHttpConfigToMessageHandler(HttpConfig *http_config);

  bool getListen() const;
  Connection *getNext() const;
  socket_t getFd() const;
  struct sockaddr_in getServerSockaddr() const;
  // const HttpConfig	*getHttpConfig() const;
  // const RequestMessage		&getRequestMessage() const;
  HttpConfig *getHttpConfig();
  struct sockaddr_in getSockaddrToConnect();
};
}  // namespace ft
#endif
