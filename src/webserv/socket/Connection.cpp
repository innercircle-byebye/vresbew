#include "webserv/socket/Connection.hpp"

namespace ft {

/*
** Connection은 클라이언트와 서버가 bind 된 이후에 계속 유지 되어 있는 상태
** 바인딩이 된 이후에 생긴 connection에 클라이언트의 접속정보, 서버의 포트, 클라이언트가 보내오는 메세지를 필수적으로 보관하고
** HTTP 규약에 맞는 메세지를 보내 올 시 연결을 계속 유지하며 서버에서 요청에 맞는 동작을 실행하거나, 응답을 보내게 된다.
*/

Connection::Connection()
    : listen_(false), fd_(-1), type_(SOCK_STREAM), listening_(NULL), request_(), response_() {
  sockaddr_to_connect_.sin_family = AF_INET;
  memset(buffer_, 0, BUF_SIZE);

  // 메세지를 받아 오는 과정에서 단계를 나누기 위한 변수
  recv_phase_ = MESSAGE_START_LINE_INCOMPLETE;
  // 보내온 메세지를 서버가 해석한 결과가 담기는 변수
  req_status_code_ = NOT_SET;
  // 클라이언트로부터 받아오는 메세지가 담길 버퍼
  body_buf_ = "";
  // 클라이언트가 'content-length' 헤더 를 보냈을 때 value에 해당하는 값을 담기 위한 변수
  // 클라이언트가 요청을 보낼 때 메세지의 크기를 정해서 보내주는 경우에 활용
  string_buffer_content_length_ = -1;
  // 클라이언트가 'content-length' 헤더를 보내지 않고, 'transfer-encoding' 헤더를 보냈을 경우에
  // 보내오는 메세지의 상태를 확인 하기 위해 설정한 변수
  chunked_checker_ = STR_SIZE;
  // 'transfer-encoding' 헤더를 통해 메세지를 보내 왔을 때 길이정보를 저장할 변수
  // (확인 바랍니다)
  chunked_str_size_ = 0;
  // 클라이언트의 메세지가  'transfer-encoding' 헤더를 보냈는지 아닌지 확인하기 위한 변수
  is_chunked_ = false;
  // (cgi프로세스 실행 시) 보낸 메세지의 길이를 확인하는 변수
  send_len = 0;
  // 서버 프로그램의 config 를 통해 'client 가 최대로 보낼 수 있는 body 의 사이즈'를
  // 확인하는 변수
  client_max_body_size = -1;
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

// 클라이언트와의 연결이 종료 되지 않는 이상, 현재 Connection 객체 내에서
// 해당 멤버변수들을 초기화 하여 재활용 하기위해 사용하기 위해서도 쓰인다.

void Connection::clear() {
  request_.clear();
  response_.clear();
  body_buf_.clear();
  recv_phase_ = MESSAGE_START_LINE_INCOMPLETE;
  string_buffer_content_length_ = -1;
  req_status_code_ = NOT_SET;
  chunked_checker_ = STR_SIZE;
  chunked_str_size_ = 0;
  is_chunked_ = false;
  temp.clear();
  send_len = 0;
  client_max_body_size = -1;
}

// 'Transfer-Encoding : chunked' 헤더를 포함하여 요청 메세지를 보낸 경우
// 요청 메세지를 다시금 한번 더 보낼 수 있다. 이를 위해 connection 객체의 일부분만 초기화 하기 위해
// 만든 변수
void Connection::clearAtChunked() {
  request_.clear();
  response_.clear();
  body_buf_.clear();
  string_buffer_content_length_ = -1;
  chunked_str_size_ = 0;
  temp.clear();
  send_len = 0;
  client_max_body_size = -1;
}

void Connection::appendBodyBuf(char *buffer) {
  body_buf_.append(buffer);
}

void Connection::appendBodyBuf(char *buffer, size_t size) {
  body_buf_.append(buffer, size);
}

// ref: Kqueue.cpp:68
// connection 내에서 클라이언트가 보내오는 메세지를 처리하기 위해 사용되는 함수
void Connection::process_read_event(Kqueue *kq, SocketManager *sm) {
  if (this->listen_) {
    Connection *conn = this->eventAccept(sm);  // throw
    kq->kqueueSetEvent(conn, EVFILT_READ, EV_ADD);
  } else {
    // this->fd_의 클라이언트 소켓으로 부터 buf_size - 1 만큼의 메세지를 recv 시도 합니다.
    ssize_t recv_len = recv(this->fd_, this->buffer_, BUF_SIZE - 1, 0);
    if (strchr(this->buffer_, ctrl_c[0])) {
      sm->closeConnection(this);
      return;
    }
    // if / else if 로 분기가 나뉘저 지지 않은 이유는
    // 클라이언트로부터 버퍼 메세지를 받아 올때 얼마만큼의 메세지가 한번에 들어 올지 알 수 없기 때문에
    if (this->recv_phase_ == MESSAGE_START_LINE_INCOMPLETE ||
        this->recv_phase_ == MESSAGE_START_LINE_COMPLETE ||
        this->recv_phase_ == MESSAGE_HEADER_INCOMPLETE ||
        this->recv_phase_ == MESSAGE_CHUNKED) {
      // 메세지 헤더 파싱
      MessageHandler::handleRequestHeader(this);
    }
    if (this->recv_phase_ == MESSAGE_HEADER_COMPLETE)
      // 메세지의 헤더까지 파싱이 완료 된 이후 헤더의 상태를 확인
      MessageHandler::checkRequestHeader(this);
    // if / elsf if 로 분기가 나뉘어 지는점 유의해주세요
    if (this->recv_phase_ == MESSAGE_CHUNKED)
      // 해당 메세지가 chunked 메세지인 경우
      MessageHandler::handleChunkedBody(this);
    else if (this->recv_phase_ == MESSAGE_BODY_INCOMING)
      // 해당 메세지에서 content-length 헤더가 주어진 경우
      MessageHandler::handleRequestBody(this);
    // 위 두 단계의 메세지 읽어오는 과정이 완료 된 이후
    if (this->recv_phase_ == MESSAGE_BODY_COMPLETE) {
      // 해당 요청이 에러가 발생하는 경우 아래 분기를 실행 할 필요 없이 바로 response를 생성 하면 됨
      if (this->req_status_code_ == NOT_SET)
        // 해당 메세지가 cgi 요청인지 확인
        // 맞으면 cgi 프로세스 실행 -> 필요한 경우 자식 프로세스로 body메세지를 전송
        // -> 메세지를 받아옴
        MessageHandler::checkCgiProcess(this);
      if (this->recv_phase_ == MESSAGE_CGI_COMPLETE) {
        // cgi 프로세스가 완료된 경우, cgi 응답에서도 header를 따로 보내오기 때문에
        // 이를 파싱하여, cgi 프로세스의 상태를 확인하고(Status로 코드를 만들어서 보내주기도 함)
        // 응답 메세지의 헤더 메세지를 만들어야 함.
        CgiHandler::handleCgiHeader(this);
        CgiHandler::setupCgiMessage(this);
      } else {
        // 만약 cgi 요청이 아닌 경우, 서버 내부에서 필요한 동작을 실행하기 위한 함수
        MessageHandler::executeServerSide(this);  // 서버가 실제 동작을 진행하는 부분
        MessageHandler::setResponseMessage(this);
      }
      // 이 과정이 모두 끝나면 (MESSAGE_BODY_COMPLETE)
      // 클라이언트로부터 메세지를 보낼 준비가 되었기 때문에
      // connection 객체에서
      // 현재 kevent를 지우고, EVFILT_WRITE로 현재 이벤트를 새로 등록
      kq->kqueueSetEvent(this, EVFILT_READ, EV_DELETE);
      kq->kqueueSetEvent(this, EVFILT_WRITE, EV_ADD);
    }
    memset(this->buffer_, 0, recv_len);
  }
}

// ref: Kqueue.cpp:78
// connection 내에서 클라이언트가 보내오는 메세지를 처리하기 위해 사용되는 함수
void Connection::process_write_event(Kqueue *kq, SocketManager *sm) {
  if (this->recv_phase_ == MESSAGE_CGI_COMPLETE) {
    // CGI 응답의 경우 getHeaderMessage에 응답 메세지 (헤더 + body) 모두 함께 들어 오게되는데
    // 용량이 큰 응답의 경우 클라이언트로 응답을 '쪼개어' 보내주기 위해 필요한 반복문
    // (처음에 0으로 설정된) send_len이 보내야하는 메세지의 길이까지 도달 할 때 까지
    // BUF_SIZE 만큼 메세지를 보내주게 된다.
    if (this->send_len < this->response_.getHeaderMsg().size()) {
      size_t j = std::min(this->response_.getHeaderMsg().size(), this->send_len + BUF_SIZE - 1);
      ssize_t real_send_len = send(this->fd_, &(this->response_.getHeaderMsg()[this->send_len]), j - this->send_len, 0);
      this->send_len += real_send_len;
    }
    // 해당 블럭 공통
    // 현재 생성된 메세지 (MESSAGE_BODY_COMPLETE 혹은 MESSAGE_CGI_COMPLETE) 다 거쳐온 메세지의
    // 속성 중 '요청 메세지' 요청의 HTTP버전 요청 값이 HTTP/1.0 이거나
    // '응답 메세지'의 헤더 중 'connection' 헤더의 값이 'close'로 설정된 경우
    // (보통 status_code를 따라 설정된다)
    // 소켓메니저로부터 현재 connection 객체의 연결을 끊도록 한다.
    if (!this->response_.getHeaderValue("Connection").compare("close") ||
        !this->request_.getHttpVersion().compare("HTTP/1.0")) {
      sm->closeConnection(this);
      return;
    }
    if (this->send_len >= this->response_.getHeaderMsg().size()) {
      // 모든 메세지를 보냈을 때
      // 현재 EVFILT_WRITE로 등록된 kevent를 삭제 하고
      // EVFILT_READ로 이벤트를 새로 등록
      kq->kqueueSetEvent(this, EVFILT_WRITE, EV_DELETE);
      kq->kqueueSetEvent(this, EVFILT_READ, EV_ADD);
      // connection 정보를 모두 초기화
      this->clear();
    }
  } else if (this->recv_phase_ == MESSAGE_BODY_COMPLETE) {
    MessageHandler::sendResponseToClient(this);
    if (!this->response_.getHeaderValue("Connection").compare("close") ||
        !this->request_.getHttpVersion().compare("HTTP/1.0")) {
      sm->closeConnection(this);
      return;
    }
    if (is_chunked_)
      this->recv_phase_ = MESSAGE_CHUNKED;
    else {
      this->recv_phase_ = MESSAGE_START_LINE_INCOMPLETE;
      this->req_status_code_ = NOT_SET;
    }
    kq->kqueueSetEvent(this, EVFILT_WRITE, EV_DELETE);
    kq->kqueueSetEvent(this, EVFILT_READ, EV_ADD);
    this->clearAtChunked();
  }
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
struct sockaddr_in &Connection::getSockaddrToConnect() {
  return sockaddr_to_connect_;
}
int Connection::getRecvPhase() const { return recv_phase_; }
int Connection::getStringBufferContentLength() const { return string_buffer_content_length_; }
std::string &Connection::getBodyBuf() { return (body_buf_); }

}  // namespace ft
