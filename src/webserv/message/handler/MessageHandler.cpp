#include "webserv/message/handler/MessageHandler.hpp"

namespace ft {
MessageHandler::MessageHandler() : request_handler_(), response_handler_() {}

MessageHandler::~MessageHandler() {}

void MessageHandler::handle_request(Connection *c) {
  // 1. recv
  size_t recv_len = recv(c->getFd(), c->buffer_, BUF_SIZE, 0);
  // 2. request_handler의 request가 c의 request가 되도록 세팅
  request_handler_.setRequest(&c->getRequest());
  // 2. append (이전에 request가 setting되어야함)
  request_handler_.appendMsg(c->buffer_);
  // 4. process by recv_phase
  request_handler_.processByRecvPhase();
  // 5. clear c->buffer_
  memset(c->buffer_, 0, recv_len);
}

void MessageHandler::handle_response(Connection *c) {
  // std::cout << c->getRequest().getMethod() << std::endl;
  // std::cout << c->getRequest().getUri() << std::endl;
  // std::cout << c->getRequest().getHttpVersion() << std::endl;

  response_handler_.setResponse(&c->getResponse());
  response_handler_.setServerConfig(c->getHttpConfig(), c->getSockaddrToConnect(), c->getRequest().getHeaderValue("Host"));

  if (!request_handler_.isValidRequestMethod() || !request_handler_.isValidRequestVersion())
    response_handler_.setResponse400();
  else
    response_handler_.setResponseFields(c->getRequest().getMethod(), c->getRequest().getUri());

  response_handler_.makeResponseMsg();
  
  if (c->getRequest().getMethod() == "PUT" &&
      (c->getResponse().getStatusCode() == "201" || (c->getResponse().getStatusCode() == "204"))) {
    std::cout << "in message handler it catched " << std::endl;
    executePutMethod(this->response_handler_.getAccessPath(c->getRequest().getUri()), c->getRequest().getEntityBody());
  }

  send(c->getFd(), c->getResponse().getMsg().c_str(), c->getResponse().getMsg().size(), 0);
  // TODO: 언제 삭제해야하는지 적절한 시기를 확인해야함
  c->getRequest().clear();
  c->getResponse().clear();
}

void MessageHandler::executePutMethod(std::string path, std::string content) {
  std::ofstream output(path.c_str());

  output << content;
  output.close();
}

}  // namespace ft
