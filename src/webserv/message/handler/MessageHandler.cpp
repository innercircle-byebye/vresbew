#include "webserv/message/handler/MessageHandler.hpp"

namespace ft {

RequestHandler MessageHandler::request_handler_ = RequestHandler();
ResponseHandler MessageHandler::response_handler_ = ResponseHandler();

MessageHandler::MessageHandler() {}
MessageHandler::~MessageHandler() {}

void MessageHandler::handle_request_header(Connection *c) {
  if (c->getRecvPhase() == MESSAGE_START_LINE_INCOMPLETE && !std::string(c->buffer_).compare("\r\n"))
    return;
  // 1. request_handler의 request가 c의 request가 되도록 세팅
  request_handler_.setRequest(&c->getRequest());
  // 2. buffer 안에서 ctrl_c 가 전송 되었는지 확인
  check_interrupt_received(c);
  // 3. append (이전에 request가 setting되어야함)
  request_handler_.appendMsg(c->buffer_);
  // 4. process by recv_phase
  request_handler_.processByRecvPhase(c);
}

void MessageHandler::check_request_header(Connection *c) {
  ServerConfig *serverconfig_test = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
  LocationConfig *locationconfig_test = serverconfig_test->getLocationConfig(c->getRequest().getPath());

  // 있어야되는지??
  request_handler_.setRequest(&c->getRequest());

  if (request_handler_.isHostHeaderExist() == false) {
    c->status_code_ = 400;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (request_handler_.isUriFileExist(locationconfig_test) == false) {
    c->status_code_ = 404;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (request_handler_.isAllowedMethod(locationconfig_test) == false) {
    c->status_code_ = 405;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (!c->getRequest().getHeaderValue("Content-Length").empty())
    c->setStringBufferContentLength(stoi(c->getRequest().getHeaderValue("Content-Length")));

  if (c->interrupted == true) {
    c->setRecvPhase(MESSAGE_INTERRUPTED);
    return;
  }

  // TODO: 조건문 정리 CHUNKED_CHUNKED
  if (c->getRequest().getMethod() == "GET") {
    c->getBodyBuf().clear();
    c->setStringBufferContentLength(-1);
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  } else if ((c->getStringBufferContentLength() == (int)c->getBodyBuf().size()))
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  else
    c->setRecvPhase(MESSAGE_BODY_INCOMING);
}

void MessageHandler::check_cgi_process(Connection *c) {
  ServerConfig *serverconfig_test = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
  LocationConfig *locationconfig_test = serverconfig_test->getLocationConfig(c->getRequest().getPath());

  if (!locationconfig_test->getCgiPath().empty() &&
      locationconfig_test->checkCgiExtension(c->getRequest().getPath())) {
    CgiHandler::init_cgi_child(c);
  }
}

void MessageHandler::handle_request_body(Connection *c) {
  check_interrupt_received(c);

  // TODO: 조건문 수정 CHUNKED_CHUNKED
  // Transfer-Encoding : chunked 아닐 때
  if (c->getStringBufferContentLength() != -1) {
    if ((size_t)c->getStringBufferContentLength() <= strlen(c->buffer_)) {
      c->appendBodyBuf(c->buffer_, c->getStringBufferContentLength());
      c->setStringBufferContentLength(0);
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    } else {
      c->setStringBufferContentLength(c->getStringBufferContentLength() - strlen(c->buffer_));
      c->setBodyBuf(c->buffer_);
    }
  } else {
    ////// 여기
    std::cout << "====chunked_body_place==========" << std::endl;
    std::cout << c->body_buf_ << std::endl;
    std::cout << "====chunked_body_place==========" << std::endl;
    std::cout << "====chunked_body_place==========" << std::endl;
    std::cout << c->buffer_ << std::endl;
    std::cout << "====chunked_body_place==========" << std::endl;
    ////// 여기

    // TODO: c->body_buf_ 에 append 를 할것이 아니라 temp_buf_ 에 해야합니다. 수정을 요합니다.
    // buffer
    char *ptr;

    if (*c->buffer_)
      // c->buffer_ 처리
      ptr = c->buffer_;
    else
      // body_buf_ 처리
      ptr = (char *)c->getBodyBuf().c_str();
    // 아래 기능을 함수로 만들어 보는 것도 좋을 듯합니다.
    chunked_decode(ptr, c);
    /*
    if (c->need_more_append_length) {  // 이전에 동작에서 메세지가 끊어져서 들어온경우 남은 메세지를 append 한다.
      std::cout << "!!!!!!append first!!!!!!" << std::endl;
      // write(c->writepipe[1], ptr, std::min(c->need_more_append_length, strlen(ptr)));
      c->body_buf_.append(ptr, std::min(c->need_more_append_length, strlen(ptr)));
      c->need_more_append_length += std::min(c->need_more_append_length, strlen(ptr));
    }
    // calc chunked type length
    size_t length = 0;
    // while (length = strtoul(c->getBodyBuf().c_str() + length, &ptr, 16)) {
    while (length = strtoul(ptr + length, &ptr, 16)) {
      // check ptr 뒤에 CRLF
      std::string new_str = ptr;
      // new_str.compare(0, 2, "\r\n");
      if (new_str.compare(0, 2, "\r\n") != 0) {
        c->setRecvPhase(MESSAGE_BODY_COMPLETE);
        return;
      }
      // c->need_more_append_length = length - write(c->writepipe[1], ptr + 2, std::min(length, strlen(ptr)));
      c->body_buf_.append(ptr + 2, std::min(length, strlen(ptr)));
      c->need_more_append_length -= std::min(length, strlen(ptr));
      length += 2;
    }
    // 0 을 만난 경우
    if (length == 0) {
      // 0 CRLF CRLF
      if (strcmp(ptr, "\r\n\r\n") == 0) {
        std::cout << "finish chunked decord" << std::endl;
        c->setRecvPhase(MESSAGE_BODY_COMPLETE);
        c->body_buf_.clear();
      } else {
        std::cout << "chunked decord FAIL...." << std::endl;
        c->setRecvPhase(MESSAGE_BODY_COMPLETE);
        // 0 뒤에 CRLF CRLF 가 아닌 다른게 온 경우 (잘못된 경우입니다.!)
      }
    }
    */
  }
}

void MessageHandler::set_response_header(Connection *c) {
  response_handler_.setResponse(&c->getResponse(), &c->getBodyBuf());
  response_handler_.setServerConfig(c->getHttpConfig(), c->getSockaddrToConnect(), c->getRequest().getHeaderValue("Host"));

  // status_code 기본값: -1
  if (c->status_code_ > 0) {
    response_handler_.setStatusLineWithCode(c->status_code_);
    return;
  }

  response_handler_.setResponseFields(c->getRequest());

  if (c->getRequest().getMethod() == "PUT" &&
      (c->getResponse().getStatusCode() == 201 || (c->getResponse().getStatusCode() == 204))) {
    // create response body
    executePutMethod(response_handler_.getAccessPath(c->getRequest().getPath()), c->getBodyBuf());

    //TODO: remove;
    c->getBodyBuf().clear();
  }
}

void MessageHandler::set_response_message(Connection *c) {
  // MUST BE EXECUTED ONLY WHEN BODY IS NOT PROVIDED
  // TODO: fix this garbage conditional statement...
  std::cout << c->getResponse().getHeaderMsg() << std::endl;
  if (!(c->getResponse().getStatusCode() == 200 ||
        c->getResponse().getStatusCode() == 201 ||
        c->getResponse().getStatusCode() == 204))
    response_handler_.setDefaultErrorBody();

  c->getResponse().setHeader("Content-Length",
                             std::to_string(c->getBodyBuf().size()));

  response_handler_.makeResponseHeader();
}

void MessageHandler::send_response_to_client(Connection *c) {
  send(c->getFd(), c->getResponse().getHeaderMsg().c_str(), c->getResponse().getHeaderMsg().size(), 0);
  if (c->getRequest().getMethod() != "HEAD")
    send(c->getFd(), c->getBodyBuf().c_str(), c->getBodyBuf().size(), 0);
}

void MessageHandler::executePutMethod(std::string path, std::string content) {
  std::ofstream output(path.c_str());
  output << content;
  output.close();
}

void MessageHandler::check_interrupt_received(Connection *c) {
  int i = 0;
  while (i < CTRL_C_LIST) {
    if (strchr(c->buffer_, ctrl_c[i]))
      c->interrupted = true;
    i++;
  }
}

void MessageHandler::chunked_decode(char *ptr, Connection *c) {
  if (c->need_more_append_length) {  // 이전에 동작에서 메세지가 끊어져서 들어온경우 남은 메세지를 append 한다.
    std::cout << "!!!!!!append first!!!!!!" << std::endl;
    // write(c->writepipe[1], ptr, std::min(c->need_more_append_length, strlen(ptr)));
    // c->body_buf_.append(ptr, std::min(c->need_more_append_length, strlen(ptr)));
    c->temp_buf_.append(ptr, std::min(c->need_more_append_length, strlen(ptr)));
    c->need_more_append_length += std::min(c->need_more_append_length, strlen(ptr));
  }
  // calc chunked type length
  size_t length = 0;
  // while (length = strtoul(c->getBodyBuf().c_str() + length, &ptr, 16)) {
  while ((length = strtoul(ptr + length, &ptr, 16))) {
    // check ptr 뒤에 CRLF
    std::string new_str = ptr;
    // new_str.compare(0, 2, "\r\n");
    if (new_str.compare(0, 2, "\r\n") != 0) {
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
      return;
    }
    // c->need_more_append_length = length - write(c->writepipe[1], ptr + 2, std::min(length, strlen(ptr)));
    c->temp_buf_.append(ptr + 2, std::min(length, strlen(ptr)));
    c->need_more_append_length -= std::min(length, strlen(ptr));
    length += 2;
  }
  // 0 을 만난 경우
  if (length == 0) {
    // 0 CRLF CRLF
    if (strcmp(ptr, "\r\n\r\n") == 0) {
      std::cout << "finish chunked decord" << std::endl;
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
      c->body_buf_ = c->temp_buf_;
      return ;
    } else {
      std::cout << "chunked decord FAIL...." << std::endl;
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
      // 0 뒤에 CRLF CRLF 가 아닌 다른게 온 경우 (잘못된 경우입니다.!)
      c->body_buf_ = c->temp_buf_;
      return ;
    }
  }
  c->body_buf_.clear();
}

}  // namespace ft
