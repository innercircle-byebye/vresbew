#include "webserv/message/handler/MessageHandler.hpp"

namespace ft {

RequestHandler MessageHandler::request_handler_ = RequestHandler();
ResponseHandler MessageHandler::response_handler_ = ResponseHandler();

MessageHandler::MessageHandler() {}
MessageHandler::~MessageHandler() {}

void MessageHandler::handle_request_header(Connection *c) {
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
  // request_handler_.setRequest(&c->getRequest());

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
}

void MessageHandler::check_cgi_request(Connection *c) {
  ServerConfig *serverconfig_test = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
  LocationConfig *locationconfig_test = serverconfig_test->getLocationConfig(c->getRequest().getPath());
  //TODO: c->getRequest().getUri().find_last_of() 부분을 메세지 헤더의 mime_types로 확인하도록 교체/ 확인 필요
  if (!locationconfig_test->getCgiPath().empty() &&
      locationconfig_test->checkCgiExtension(c->getRequest().getPath())) {
    c->setRecvPhase(MESSAGE_CGI_PROCESS);
  }
}

void MessageHandler::check_body_status(Connection *c) {
  //TODO: 아래 주석에 나와있는 조건문으로 변경
  // if (c->getStringBufferContentLength() == 0
  //  && !c->getRequest().getHeaderValue("Content-Length").empty())
  if (c->getStringBufferContentLength() == 0)
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  else
    c->setRecvPhase(MESSAGE_BODY_INCOMING);
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
  send(c->getFd(), c->getBodyBuf().c_str(), c->getBodyBuf().size(), 0);
}

void MessageHandler::executePutMethod(std::string path, std::string content) {
  std::ofstream output(path.c_str());
  output << content;
  output.close();
}

// bool MessageHandler::isValidRequestMethod(const std::string &method) {
//   if (method.compare("GET") && method.compare("POST") &&
//       method.compare("DELETE") && method.compare("PUT") && method.compare("HEAD"))
//     return (false);
//   return (true);
// }

// bool MessageHandler::isValidRequestVersion(const std::string &http_version, const std::map<std::string, std::string> &headers) {
//   if (!http_version.compare("HTTP/1.1")) {
//     if (headers.count("Host"))
//       return (true);
//     return (false);
//   } else if (!http_version.compare("HTTP/1.0")) {
//     // if (!headers.count("Host"))
//     //   request_->setHeader("Host", "");   // ?? 자동으로 ""되어있는거 아닌지?
//     // A: 아닙니다. Response의 경우 응답 헤더들을 미리 설정 해 놓았지만 Request는 어떤 헤더가 들어 올지 모르기 때문에 설정 해주지 않았습니다.
//     // HTTP/1.0 처리 하지 못하는 상황 발생하여 수정하겠습니다.
//     return (true);
//   }
//   return (false);
// }

void MessageHandler::check_interrupt_received(Connection *c) {
  int i = 0;
  while (i < CTRL_C_LIST) {
    if (strchr(c->buffer_, ctrl_c[i]))
      c->interrupted = true;
    i++;
  }
}

}  // namespace ft
