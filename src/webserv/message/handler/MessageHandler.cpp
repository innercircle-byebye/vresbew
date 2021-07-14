#include "webserv/message/handler/MessageHandler.hpp"

namespace ft {
MessageHandler::MessageHandler(Request &request, Response &response)
    : request_(request),
      response_(response),
      recv_phase(0),
    //   http_config_(NULL),
      sockaddr_to_connect_(NULL),
      request_handler_(request_),
	  response_handler_(request_, response_)
    //   response_handler_(request_, response_, http_config_)
{
  	this->response_header_buf_ = NULL;
}

MessageHandler::~MessageHandler() {
  this->clearResponseHeaderBuf();
}

void MessageHandler::checkBufferForHeader(char *buffer) {
  // append string first
  // check \r\n\r\n
  int substr_size;  // 안쓰고있음
  this->msg_header_buf_ += buffer;
  if (this->msg_header_buf_.find("\r\n\r\n") != std::string::npos) {
    substr_size = this->msg_header_buf_.find("\r\n\r\n");
    this->msg_body_buf_ = this->msg_header_buf_.substr(substr_size + 4);
    this->msg_header_buf_ = this->msg_header_buf_.substr(0, substr_size);
    std::cout << "==========msg_body_buf==========" << std::endl;
    std::cout << this->msg_body_buf_ << std::endl;
    std::cout << "==========msg_body_buf==========" << std::endl;
    this->createRequest();
    this->recv_phase = MESSAGE_HEADER_COMPLETE;
    //handle_body를 나누어야함
    try {
      this->content_length_ = stoi(this->request_.headers["Content-Length"]);
    } catch (const std::exception &e) {
      std::cout << "content-length not available" << std::endl;
      this->request_.headers.erase("Content-Length");
      this->recv_phase = MESSAGE_BODY_NO_NEED;
    }
  }
}

// 로직 수정 필요
// handlebody 로직을 나누어야 함
// nc와 telnet의 동작이 다르고 content_length를 활용 못하고있음
void MessageHandler::handleBody(char *buffer) {
;
  size_t buffer_size_;
  if (this->recv_phase == MESSAGE_HEADER_COMPLETE) {
    // std::cout << "msg_body_buf_size: " << this->msg_body_buf_.size() << std::endl;
    // std::cout << "content_lenght 2: " << this->content_length_ << std::endl;
    // if (this->msg_body_buf_.size() >= this->content_length_) {
    //   this->msg_body_buf_.substr(0, this->content_length_);
    //   this->recv_phase = MESSAGE_BODY_COMPLETE;
    // } else
    // {
      this->recv_phase = MESSAGE_BODY_INCOMING;
    // }
    // return ;
  } else {
    buffer_size_ = strlen(buffer);
    std::cout << "buffer_size_: "<< buffer_size_ << std::endl;
    if (content_length_ < buffer_size_)
    {
      this->msg_body_buf_.append(buffer, content_length_);
      this->recv_phase = MESSAGE_BODY_COMPLETE;
    }
    else if (content_length_ >= buffer_size_) {
      this->msg_body_buf_.append(buffer);
      this->content_length_ -= buffer_size_;
    } else {
      this->msg_body_buf_.append(buffer, (buffer_size_ - content_length_));
      this->recv_phase = MESSAGE_BODY_COMPLETE;
    }
  }
  std::cout << "-----------body-----------" << std::endl;
  std::cout << msg_body_buf_ << std::endl;
  std::cout << "-----------body end-------" << std::endl;
}

void MessageHandler::checkRemainderBufferForHeader(char *buffer) {
  // this->msg_header_buf_.append("GET / HTTP/1.1");
  this->recv_phase = MESSAGE_HEADER_INCOMPLETE;
}

// void MessageHandler::setHttpConfig(HttpConfig *http_config) {
//   this->http_config_ = http_config;
// }

void MessageHandler::setSockAddr(struct sockaddr_in *addr) {
  this->sockaddr_to_connect_ = addr;
}

void MessageHandler::setContentLengthToZero(void) {
  this->content_length_ = 0;
}

void MessageHandler::clearMsgBodyBuf(void) {
  this->msg_body_buf_.clear();
}

void MessageHandler::clearMsgHeaderBuf(void) {
  this->msg_header_buf_.clear();
}

void MessageHandler::clearResponseHeaderBuf(void) {
  if (this->response_header_buf_ != NULL)
    delete response_header_buf_;
}

void MessageHandler::createRequest(void) {
  this->request_handler_.setRequest(this->msg_header_buf_);
}

void MessageHandler::createResponse(HttpConfig *http_config) {
  this->response_handler_.setServerConfig(http_config, this->sockaddr_to_connect_);
  this->response_handler_.setResponse();
  if (this->request_.method == "PUT" &&
      (this->response_.status_code_ == "201" || this->response_.status_code_ == "204")) {
    std::cout << "in message handler it catched " << std::endl;
    executePutMethod(this->response_handler_.getAccessPath(this->request_.uri));
  }
  this->response_header_buf_ = createResponseHeaderBuf();
}

void MessageHandler::executePutMethod(std::string path) {
  std::ofstream output(path.c_str());

  output << this->msg_body_buf_;
  output.close();
}

std::string *MessageHandler::createResponseHeaderBuf() {
  std::string *result = new std::string();
  *result += this->response_.http_ver_;
  *result += " ";
  *result += this->response_.status_code_;
  *result += " ";
  *result += response_.status_message_;
  *result += "\r\n";
  std::map<std::string, std::string>::iterator it;
  // 순서에 맞아야하는지 확인해야함
  for (it = this->response_.header_.begin(); it != this->response_.header_.end(); it++) {
    if (it->second != "") {
      *result += it->first;
      *result += ": ";
      *result += it->second;
      *result += "\r\n";
    }
  }
  *result += "\r\n";
  if (response_.response_body_.size()) {
    *result += response_.response_body_;
  }
  return (result);
}

std::string MessageHandler::getResponseStatus() {
  return (this->response_.status_code_);
}

std::string MessageHandler::getRequestUri() {
  return (this->request_.uri);
}

}  // namespace ft
