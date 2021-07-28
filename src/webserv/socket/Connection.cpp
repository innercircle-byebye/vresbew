#include "webserv/socket/Connection.hpp"

namespace ft {

Connection::Connection()
    : listen_(false), fd_(-1), type_(SOCK_STREAM), listening_(NULL), request_(), response_() {

  sockaddr_to_connect_.sin_family = AF_INET;
  memset(buffer_, 0, BUF_SIZE);
  chunked_checker = CHUNKED_KEEP_COMING;
  recv_phase_ = MESSAGE_START_LINE_INCOMPLETE;
}

Connection::~Connection() {}

// listen == true 일때 호출됨
Connection *Connection::eventAccept(SocketManager *sm) {
  struct sockaddr_in sockaddr;
  socklen_t socklen = sizeof(struct sockaddr_in);

  socket_t s = accept(fd_, (struct sockaddr *)&sockaddr, &socklen);

  if (s == -1) {
    Logger::logError(LOG_ALERT, "accept() failed");
    throw AcceptExcception();
  }

  Connection *c = sm->getConnection(s);

  if (c == NULL) {  // free_connection 없음
    if (closeSocket(s) == -1) {
      Logger::logError(LOG_ALERT, "close() socket failed");
      throw CloseSocketException();
    }
    throw ConnNotEnoughException();
  }
  if (nonblocking(s) == -1) {
    Logger::logError(LOG_ALERT, "fcntl(O_NONBLOCK) failed");
    sm->closeConnection(c);  // s가 close됨
    throw NonblockingException();
  }

  c->setListening(listening_);
  c->setSockaddrToConnectPort(sockaddr_to_connect_.sin_port);
  c->setSockaddrToConnectIP(sockaddr.sin_addr.s_addr);
  return c;
}

/* SETTER */
void Connection::setListen(bool listen) { listen_ = listen; }

void Connection::setNext(Connection *next) { next_ = next; }

void Connection::setFd(socket_t fd) { fd_ = fd; }

void Connection::setType(int type) { type_ = type; }

void Connection::setListening(Listening *listening) { listening_ = listening; }

void Connection::setSockaddrToConnectPort(in_port_t port) { sockaddr_to_connect_.sin_port = port; }

void Connection::setSockaddrToConnectIP(in_addr_t ipaddr) { sockaddr_to_connect_.sin_addr.s_addr = ipaddr; }

void Connection::setHttpConfig(HttpConfig *httpconfig) { httpconfig_ = httpconfig; }

// void Connection::passHttpConfigToMessageHandler(HttpConfig *http_config) {
//   this->message_handler_.setHttpConfig(http_config);
// }

/* GETTER */
bool Connection::getListen() const { return listen_; }
Connection *Connection::getNext() const { return next_; }
socket_t Connection::getFd() const { return fd_; }
/*
const HttpConfig	*Connection::getHttpConfig() const
{ return httpconfig; }
const RequestMessage		&Connection::getRequestMessage() const
{ return req_msg; }
*/

Request &Connection::getRequest() { return request_; }

Response &Connection::getResponse() { return response_; }

HttpConfig *Connection::getHttpConfig() { return httpconfig_; }

struct sockaddr_in &Connection::getSockaddrToConnect() {
  return sockaddr_to_connect_;
}

// 이전

int Connection::getRecvPhase() const {return recv_phase_; }
void Connection::setRecvPhase(int recv_phase) { recv_phase_ = recv_phase;}

int Connection::getStringBufferContentLength() const { return string_buffer_content_length_; }
void Connection::setStringBufferContentLength(int string_buffer_content_length) {
  string_buffer_content_length_ = string_buffer_content_length;
}


}  // namespace ft
