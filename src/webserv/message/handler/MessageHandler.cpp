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

  //TODO: c->getRequest().getUri().find_last_of() 부분을 메세지 헤더의 mime_types로 확인하도록 교체/ 확인 필요
  if (!locationconfig_test->getCgiPath().empty() &&
      locationconfig_test->checkCgiExtension(c->getRequest().getPath())) {
    CgiHandler::init_cgi_child(c);
  } else {
    if (c->getStringBufferContentLength() == 0)
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    else
      c->setRecvPhase(MESSAGE_BODY_INCOMING);
  }
}

void MessageHandler::handle_request_body(Connection *c) {
  check_interrupt_received(c);
  if ((size_t)c->getStringBufferContentLength() <= strlen(c->buffer_)) {
    c->appendBodyBuf(c->buffer_, c->getStringBufferContentLength());
    c->setStringBufferContentLength(0);
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  } else {
    c->setStringBufferContentLength(c->getStringBufferContentLength() - strlen(c->buffer_));
    c->setBodyBuf(c->buffer_);
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

  response_handler_.executeMethod(c->getRequest());

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
  if (!(c->getResponse().getStatusCode() == 200 ||
        c->getResponse().getStatusCode() == 201 ||
        c->getResponse().getStatusCode() == 204))
    response_handler_.setDefaultErrorBody();

  response_handler_.setDefaultHeader(c->getRequest());

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

}  // namespace ft
