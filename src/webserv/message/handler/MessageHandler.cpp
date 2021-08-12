#include "webserv/message/handler/MessageHandler.hpp"

namespace ft {

RequestHandler MessageHandler::request_handler_ = RequestHandler();
ResponseHandler MessageHandler::response_handler_ = ResponseHandler();

MessageHandler::MessageHandler() {}
MessageHandler::~MessageHandler() {}

void MessageHandler::handleRequestHeader(Connection *c) {
  if (c->getRecvPhase() == MESSAGE_START_LINE_INCOMPLETE && !std::string(c->buffer_).compare(CRLF))
    return;
  // 1. request_handler의 request가 c의 request가 되도록 세팅
  request_handler_.setRequest(&c->getRequest());
  // 2. append (이전에 request가 setting되어야함)
  request_handler_.appendMsg(c->buffer_);
  // 3. process by recv_phase
  request_handler_.processByRecvPhase(c);
}

void MessageHandler::checkRequestHeader(Connection *c) {
  ServerConfig *serverconfig_test = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
  LocationConfig *locationconfig_test = serverconfig_test->getLocationConfig(c->getRequest().getPath());

  locationconfig_test->print_status_for_debug("============");
  std::cout <<"ntohs:["<< ntohs(c->getSockaddrToConnect().sin_port) << "]" <<std::endl;
  std::cout << c->getRequest().getHeaderValue("Host") << "]" << std::endl;
  // 있어야되는지??
  request_handler_.setRequest(&c->getRequest());

  //t_uri uri_struct 전체 셋업하는 부분으로...
  request_handler_.setupUriStruct(serverconfig_test, locationconfig_test);

  //client_max_body_size 셋업
  c->client_max_body_size = locationconfig_test->getClientMaxBodySize();

  std::cout << "uri: " << c->getRequest().getUri() << std::endl;
  std::cout << "schema: " << c->getRequest().getSchema() << std::endl;
  std::cout << "host: " << c->getRequest().getHost() << std::endl;
  std::cout << "port: " << c->getRequest().getPort() << std::endl;
  std::cout << "path: " << c->getRequest().getPath() << std::endl;
  std::cout << "filepath: " << c->getRequest().getFilePath() << std::endl;
  std::cout << "query_string: |" << c->getRequest().getQueryString() << "|" << std::endl;

  if (!c->getRequest().getHeaderValue("Content-Length").empty()) {
    c->setStringBufferContentLength(stoi(c->getRequest().getHeaderValue("Content-Length")));
    if (!c->getRequest().getMsg().empty())
      c->setBodyBuf(c->getRequest().getMsg());
  } else if (c->getRequest().getMethod().compare("HEAD") && c->getRequest().getMethod().compare("GET"))
    c->is_chunked_ = true;

  if (request_handler_.isHostHeaderExist() == false) {
    c->req_status_code_ = 400;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (locationconfig_test->checkReturn()) {
    request_handler_.applyReturnDirectiveStatusCode(c, locationconfig_test);
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }
  if (*(c->getRequest().getFilePath().rbegin()) == '/') {
    request_handler_.findIndexForGetWhenOnlySlash(locationconfig_test);
    if (*(c->getRequest().getFilePath().rbegin()) == '/' && locationconfig_test->getAutoindex() == false) {
      c->req_status_code_ = 404;
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
      return;
    }
  }

  if (request_handler_.isUriFileExist(locationconfig_test) == false &&
      c->getRequest().getMethod() != "PUT" && c->getRequest().getMethod() != "POST") {
      std::cout << "aaa" << std::endl;
    c->req_status_code_ = 404;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (request_handler_.isUriDirectory(locationconfig_test) == true &&
      c->getRequest().getMethod().compare("DELETE") && locationconfig_test->getAutoindex() == false ) {
      std::cout << "bbb" << std::endl;
    c->req_status_code_ = 301;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (request_handler_.isAllowedMethod(locationconfig_test) == false) {
    c->req_status_code_ = 405;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (c->getRequest().getMethod().compare("GET") && c->getRequest().getMethod().compare("HEAD") &&
      c->getRequest().getHeaderValue("Content-Length").empty() && !c->getRequest().getHeaderValue("Transfer-Encoding").compare("chunked")) {
    c->setRecvPhase(MESSAGE_CHUNKED);
    c->is_chunked_ = true;
  } else if (c->getRequest().getMethod() == "GET") {
    c->is_chunked_ = false;
    c->getBodyBuf().clear();
    c->setStringBufferContentLength(-1);
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  } else if (c->is_chunked_ == true)
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  else if (c->is_chunked_ == false && c->getStringBufferContentLength() != -1 &&
           (c->getStringBufferContentLength() <= (int)c->getBodyBuf().size()))
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  else
    c->setRecvPhase(MESSAGE_BODY_INCOMING);
  c->client_max_body_size = locationconfig_test->getClientMaxBodySize();
}

void MessageHandler::checkCgiProcess(Connection *c) {
  ServerConfig *serverconfig_test = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
  LocationConfig *locationconfig_test = serverconfig_test->getLocationConfig(c->getRequest().getPath());

  if (!locationconfig_test->getCgiPath().empty() &&
      locationconfig_test->checkCgiExtension(c->getRequest().getPath())) {
    CgiHandler::initCgiChild(c);
  }
}

void MessageHandler::handleChunkedBody(Connection *c) {
  request_handler_.setRequest(&c->getRequest());

  request_handler_.handleChunked(c);
}

void MessageHandler::handleRequestBody(Connection *c) {
  if (c->is_chunked_ == false) {
    if ((size_t)c->getStringBufferContentLength() <= strlen(c->buffer_)) {
      c->appendBodyBuf(c->buffer_, c->getStringBufferContentLength());
      c->setStringBufferContentLength(-1);
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    } else {
      c->setStringBufferContentLength(c->getStringBufferContentLength() - strlen(c->buffer_));
      c->setBodyBuf(c->buffer_);
    }
  } else
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
}

void MessageHandler::executeServerSide(Connection *c) {
  request_handler_.setRequest(&c->getRequest());

  response_handler_.setResponse(&c->getResponse(), &c->getBodyBuf());
  response_handler_.setServerConfig(c->getHttpConfig(), c->getSockaddrToConnect(), c->getRequest().getHeaderValue("Host"));

  if (c->req_status_code_ != NOT_SET) {
    response_handler_.setStatusLineWithCode(c->req_status_code_);
    return;
  }

  response_handler_.executeMethod(c->getRequest());
}

void MessageHandler::setResponseMessage(Connection *c) {
  if (!(c->getResponse().getStatusCode() == 200 ||
        c->getResponse().getStatusCode() == 201 ||
        c->getResponse().getStatusCode() == 204) &&
      c->getBodyBuf().empty()) {
    ServerConfig *server_config = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
    LocationConfig *location = server_config->getLocationConfig(c->getRequest().getPath());

    response_handler_.setErrorBody(location->getErrorPage());
  }

  response_handler_.setDefaultHeader(c, c->getRequest());
  response_handler_.makeResponseHeader();
}

void MessageHandler::sendResponseToClient(Connection *c) {
  send(c->getFd(), c->getResponse().getHeaderMsg().c_str(), c->getResponse().getHeaderMsg().size(), 0);
  if (c->getRequest().getMethod() != "HEAD")
    send(c->getFd(), c->getBodyBuf().c_str(), c->getBodyBuf().size(), 0);
}

}  // namespace ft
