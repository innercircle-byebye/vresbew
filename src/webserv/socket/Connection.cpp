#include "webserv/socket/Connection.hpp"

namespace ft {

Connection::Connection()
    : listen_(false), fd_(-1), type_(SOCK_STREAM), listening_(NULL), request_(), response_() {
  sockaddr_to_connect_.sin_family = AF_INET;
  memset(buffer_, 0, BUF_SIZE);

  recv_phase_ = MESSAGE_START_LINE_INCOMPLETE;
  interrupted = false;
  status_code_ = -1;
  body_buf_ = "";
  string_buffer_content_length_ = -1;
  chunked_checker_ = STR_SIZE;
  chunked_str_size_ = 0;
  is_chunked_ = false;
  send_len = 0;
  client_max_body_size = -1;
  // real_send_len = 0;
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

void Connection::clear() {
  request_.clear();
  response_.clear();
  body_buf_.clear();
  recv_phase_ = MESSAGE_START_LINE_INCOMPLETE;
  string_buffer_content_length_ = -1;
  status_code_ = -1;
  interrupted = false;
  chunked_checker_ = STR_SIZE;
  chunked_str_size_ = 0;
  is_chunked_ = false;
  temp.clear();
  send_len = 0;
  // real_send_len = 0;
  client_max_body_size = -1;
}

void Connection::appendBodyBuf(char *buffer) {
  body_buf_.append(buffer);
}
void Connection::appendBodyBuf(char *buffer, size_t size) {
  body_buf_.append(buffer, size);
}

void  Connection::process_read_event(Kqueue *kq, SocketManager *sm) {
  if (this->listen_) {
    Connection *conn = this->eventAccept(sm);  // throw
    kq->kqueueSetEvent(conn, EVFILT_READ, EV_ADD);
  } else {
    ssize_t recv_len = recv(this->fd_, this->buffer_, BUF_SIZE, 0);
    if (recv_phase_ == MESSAGE_INTERRUPTED) {
      if (strchr(this->buffer_, ctrl_c[0])) {
        this->interrupted = true;
        this->recv_phase_ = MESSAGE_BODY_COMPLETE;
      }
    }
    if (this->recv_phase_ == MESSAGE_CHUNKED_REMAINDER) {
      std::cout << "ssup==========================" << std::endl;
      this->body_buf_.append(this->buffer_);
      if (this->body_buf_.find("0\r\n\r\n") != std::string::npos) {
        this->clear();
        return ;
      }
    }
    if (this->recv_phase_ == MESSAGE_START_LINE_INCOMPLETE ||
        this->recv_phase_ == MESSAGE_START_LINE_COMPLETE ||
        this->recv_phase_ == MESSAGE_HEADER_INCOMPLETE) {
      MessageHandler::handle_request_header(this);
    }
    if (this->recv_phase_ == MESSAGE_HEADER_COMPLETE) {
      MessageHandler::check_request_header(this);
    }
    if (this->recv_phase_ == MESSAGE_CHUNKED)
    {
      std::cout << "do I even get here" << std::endl;
      MessageHandler::handle_chunked_body(this);
    }
    else if (this->recv_phase_ == MESSAGE_BODY_INCOMING)
      MessageHandler::handle_request_body(this);
    if (this->recv_phase_ == MESSAGE_BODY_COMPLETE) {
      if (this->status_code_ < 0)
        MessageHandler::check_cgi_process(this);
      if (this->recv_phase_ == MESSAGE_CGI_COMPLETE) {
        CgiHandler::handle_cgi_header(this);
        CgiHandler::setup_cgi_message(this);
      } else {
        MessageHandler::execute_server_side(this);  // 서버가 실제 동작을 진행하는 부분
        MessageHandler::set_response_message(this);
      }
      kq->kqueueSetEvent(this, EVFILT_READ, EV_DELETE);
      kq->kqueueSetEvent(this, EVFILT_WRITE, EV_ADD);
    }
    memset(this->buffer_, 0, recv_len);
  }
}

void Connection::process_write_event(Kqueue *kq, SocketManager *sm) {
  if (this->interrupted == true) {
    sm->closeConnection(this);
    this->clear();
  } else if (this->recv_phase_ == MESSAGE_CGI_COMPLETE) {
    if (this->send_len < this->response_.getHeaderMsg().size()) {
      ssize_t real_send_len;
      size_t j = std::min(this->response_.getHeaderMsg().size(), this->send_len + BUF_SIZE);
      real_send_len = send(this->fd_, &(this->response_.getHeaderMsg()[this->send_len]), j - this->send_len, 0);
      // std::cout << "real_send_len: [" << real_send_len << "]" << std::endl;
      this->send_len += real_send_len;
      if (this->send_len >= this->response_.getHeaderMsg().size()) {
        if (!this->response_.getHeaderValue("Connection").compare("close") ||
            !this->request_.getHttpVersion().compare("HTTP/1.0")) {
          sm->closeConnection(this);
        } else {
          kq->kqueueSetEvent(this, EVFILT_WRITE, EV_DELETE);
          kq->kqueueSetEvent(this, EVFILT_READ, EV_ADD);
        }
        this->clear();
      }
    } else {
      if (!this->response_.getHeaderValue("Connection").compare("close") ||
          !this->request_.getHttpVersion().compare("HTTP/1.0")) {
        sm->closeConnection(this);
      } else {
        kq->kqueueSetEvent(this, EVFILT_WRITE, EV_DELETE);
        kq->kqueueSetEvent(this, EVFILT_READ, EV_ADD);
      }
      this->clear();
    }
  } else if (this->getRecvPhase() == MESSAGE_BODY_COMPLETE) {
    MessageHandler::send_response_to_client(this);
    std::cout << "Complete" << std::endl;
    if (!this->getResponse().getHeaderValue("Connection").compare("close") ||
        !this->getRequest().getHttpVersion().compare("HTTP/1.0")) {
      sm->closeConnection(this);
    } else {
      kq->kqueueSetEvent(this, EVFILT_WRITE, EV_DELETE);
      kq->kqueueSetEvent(this, EVFILT_READ, EV_ADD);
    }
    this->clear();
  }
  // this->clear();
  memset(this->buffer_, 0, BUF_SIZE);
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
void Connection::setStringBufferContentLength(int string_buffer_content_length) {
  string_buffer_content_length_ = string_buffer_content_length;
}
void Connection::setRecvPhase(int recv_phase) { recv_phase_ = recv_phase; }
void Connection::setBodyBuf(std::string body_buf) { body_buf_ = body_buf; }

/* GETTER */
bool Connection::getListen() const { return listen_; }
Connection *Connection::getNext() const { return next_; }
socket_t Connection::getFd() const { return fd_; }
Request &Connection::getRequest() { return request_; }
Response &Connection::getResponse() { return response_; }
HttpConfig *Connection::getHttpConfig() { return httpconfig_; }
struct sockaddr_in &Connection::getSockaddrToConnect() { return sockaddr_to_connect_; }
int Connection::getRecvPhase() const { return recv_phase_; }
int Connection::getStringBufferContentLength() const { return string_buffer_content_length_; }
std::string &Connection::getBodyBuf() { return (body_buf_); }

}  // namespace ft
