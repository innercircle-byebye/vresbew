#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "webserv/logger/Logger.hpp"
#include "webserv/message/Request.hpp"
#include "webserv/message/Response.hpp"
#include "webserv/socket/Listening.hpp"
#include "webserv/socket/SocketManager.hpp"
#include "webserv/webserv.hpp"

namespace ft {

enum ChunkedMessageStatus {
  CHUNKED_KEEP_COMING = 0,
  CHUNKED_ZERO_RN_RN,
  CHUNKED_END //  필요없긴함
};

class Connection {
 private:
  bool listen_;

  socket_t fd_;
  int type_;
  struct sockaddr_in sockaddr_to_connect_;
  socklen_t socklen_to_connect_;

  Listening *listening_;

  HttpConfig *httpconfig_;

  Request request_;
  Response response_;

  Connection *next_;

 public:
  char buffer_[BUF_SIZE];
  int writepipe[2], readpipe[2];
  std::string temp_chunked;
  std::string cgi_output_temp;
  int chunked_checker;

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

  bool getListen() const;
  Connection *getNext() const;
  socket_t getFd() const;
  struct sockaddr_in getServerSockaddr() const;
  // const HttpConfig	*getHttpConfig() const;
  // const RequestMessage		&getRequestMessage() const;
  Request &getRequest();
  Response &getResponse();
  HttpConfig *getHttpConfig();
  struct sockaddr_in &getSockaddrToConnect();
};
}  // namespace ft
#endif
