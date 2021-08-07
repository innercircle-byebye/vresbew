#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "webserv/logger/Logger.hpp"
#include "webserv/message/Request.hpp"
#include "webserv/message/Response.hpp"
#include "webserv/socket/Listening.hpp"
#include "webserv/socket/SocketManager.hpp"
#include "webserv/webserv.hpp"

namespace ft {

enum MessageFromBufferStatus {
  MESSAGE_START_LINE_INCOMPLETE = 0,
  MESSAGE_START_LINE_COMPLETE,
  MESSAGE_HEADER_INCOMPLETE,
  MESSAGE_HEADER_COMPLETE,
  MESSAGE_HEADER_PARSED,  // ?? 어디서 사용되는지
  MESSAGE_CHUNKED,        // to test chunked
  MESSAGE_CGI_INCOMING,
  MESSAGE_CGI_COMPLETE,
  MESSAGE_BODY_INCOMING,
  MESSAGE_BODY_COMPLETE,
  MESSAGE_INTERRUPTED
};

enum ChunkedMessageStatus {
  STR_SIZE = 0,
  STR,
  END
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

  int recv_phase_;  // msg가 얼마나 setting되었는지 알 수 있는 변수
  int string_buffer_content_length_;

 public:
  char buffer_[BUF_SIZE];
  bool interrupted;
  std::string body_buf_;
  pid_t cgi_pid;
  int writepipe[2], readpipe[2];

  int status_code_;

  int chunked_checker_;
  size_t chunked_str_size_;

  // std::string temp_chunked;     //TODO: remove
  // std::string cgi_output_temp;  //TODO: remove

  Connection();
  ~Connection();

  Connection *eventAccept(SocketManager *sv);

  void clear();

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

  // 이전
  int getRecvPhase() const;
  int getStringBufferContentLength() const;
  void setRecvPhase(int recv_phase);
  void setStringBufferContentLength(int buffer_content_length);

  std::string &getBodyBuf();
  void setBodyBuf(std::string body_buf_);
  void appendBodyBuf(char *buffer);
  void appendBodyBuf(char *buffer, size_t size);
};
}  // namespace ft
#endif
