#include "webserv/message/handler/MessageHandler.hpp"

namespace ft {

RequestHandler MessageHandler::request_handler_ = RequestHandler();
ResponseHandler MessageHandler::response_handler_ = ResponseHandler();

MessageHandler::MessageHandler() {}
MessageHandler::~MessageHandler() {}

void MessageHandler::handle_request_header(Connection *c) {
  //RequestHandler  request_handler_;
  std::cout << "==========check_buffer=========" << std::endl;
  std::cout << c->buffer_ << std::endl;
  std::cout << "==========check_buffer=========" << std::endl;

  // recv(c->getFd(), c->buffer_, BUF_SIZE, 0);
  // 2. request_handler의 request가 c의 request가 되도록 세팅
  request_handler_.setRequest(&c->getRequest());
  // 2. append (이전에 request가 setting되어야함)
  request_handler_.appendMsg(c->buffer_);
  // 4. process by recv_phase
  request_handler_.processByRecvPhase(c);
  // 5. clear c->buffer_
  // memset(c->buffer_, 0, BUF_SIZE);
}

void MessageHandler::check_cgi_request(Connection *c) {
  ServerConfig *serverconfig_test = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
  LocationConfig *locationconfig_test = serverconfig_test->getLocationConfig(c->getRequest().getUri());
  //TODO: c->getRequest().getUri().find_last_of() 부분을 메세지 헤더의 mime_types로 확인하도록 교체/ 확인 필요
  if (!locationconfig_test->getCgiPath().empty() &&
      locationconfig_test->checkCgiExtension(c->getRequest().getUri())) {
    c->setRecvPhase(MESSAGE_CGI_PROCESS);
  }
}

void MessageHandler::check_body_status(Connection *c) {
  if (c->getStringBufferContentLength() == 0)
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  else
    c->setRecvPhase(MESSAGE_BODY_INCOMING);
}

void MessageHandler::handle_request_body(Connection *c) {
  if ((size_t)c->getStringBufferContentLength() <= strlen(c->buffer_)) {
    c->appendBodyBuf(c->buffer_, c->getStringBufferContentLength());
    c->setStringBufferContentLength(0);
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  } else {
    c->setStringBufferContentLength(c->getStringBufferContentLength() - strlen(c->buffer_));
    c->setBodyBuf(c->buffer_);
  }
}

void MessageHandler::init_cgi_child(Connection *c) {
  ServerConfig *server_config = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
  LocationConfig *location = server_config->getLocationConfig(c->getRequest().getUri());
  char **environ;
  char **command;
  pid_t pid;
  std::map<std::string, std::string> env_set;
  {
    if (!c->getRequest().getHeaderValue("Content-Length").empty()) {
      env_set["CONTENT_LENGTH"] = c->getRequest().getHeaderValue("Content-Length");
    }
    if (c->getRequest().getMethod() == "GET") {
      env_set["QUERY_STRING"] = c->getRequest().getEntityBody();
    }
    env_set["REQUEST_METHOD"] = c->getRequest().getMethod();
    env_set["REDIRECT_STATUS"] = "CGI";
    env_set["SCRIPT_FILENAME"] = response_handler_.getAccessPath(c->getRequest().getUri(), location);
    env_set["SERVER_PROTOCOL"] = "HTTP/1.1";
    env_set["PATH_INFO"] = response_handler_.getAccessPath(c->getRequest().getUri(), location);
    env_set["CONTENT_TYPE"] = "application/x-www-form-urlencoded";
    env_set["GATEWAY_INTERFACE"] = "CGI/1.1";
    env_set["PATH_TRANSLATED"] = response_handler_.getAccessPath(c->getRequest().getUri(), location);
    env_set["REMOTE_ADDR"] = "127.0.0.1";  // TODO: ip주소 받아오는 부분 찾기
    if (c->getRequest().getMethod() == "GET")
      env_set["REQUEST_URI"] = c->getRequest().getUri() + "?" + c->getRequest().getEntityBody();
    else
      env_set["REQUEST_URI"] = response_handler_.getAccessPath(c->getRequest().getUri(), location);
    env_set["HTTP_HOST"] = c->getRequest().getHeaderValue("Host");
    env_set["SERVER_PORT"] = std::to_string(ntohs(c->getSockaddrToConnect().sin_port));  // 포트도
    env_set["SERVER_SOFTWARE"] = "versbew";
    env_set["SCRIPT_NAME"] = location->getCgiPath();
  }
  environ = response_handler_.setEnviron(env_set);
  command = response_handler_.setCommand(location->getCgiPath(), response_handler_.getAccessPath(c->getRequest().getUri(), location));
  std::string cgi_output_temp;
  // TODO: 실패 예외처리
  pipe(c->writepipe);
  // TODO: 실패 예외처리
  pipe(c->readpipe);
  pid = fork();
  if (!pid) {
    // TODO: 실패 예외처리
    close(c->writepipe[1]);
    // TODO: 실패 예외처리
    close(c->readpipe[0]);
    // TODO: 실패 예외처리
    dup2(c->writepipe[0], 0);
    // TODO: 실패 예외처리
    close(c->writepipe[0]);
    // TODO: 실패 예외처리
    dup2(c->readpipe[1], 1);
    // TODO: 실패 예외처리
    close(c->readpipe[1]);
    // TODO: 실패 예외처리
    execve(location->getCgiPath().c_str(), command, environ);
  }
  // TODO: 실패 예외처리
  close(c->writepipe[0]);
  // TODO: 실패 예외처리
  close(c->readpipe[1]);

  if (c->getRequest().getMethod() == "GET")
    c->setRecvPhase(MESSAGE_CGI_COMPLETE);
  else
    c->setRecvPhase(MESSAGE_CGI_INCOMING);

  // if (!c->getRequest().getMsg().empty() &&
  //     !c->getRequest().getHeaderValue("Content-Length").empty()) {
  //   write(c->writepipe[1], c->getRequest().getMsg().c_str(), static_cast<size_t>(c->getRequest().getMsg().size()));
  //   c->getRequest().setBufferContentLength(c->getRequest().getBufferContentLength() - c->getRequest().getMsg().size());
  //   c->getRequest().getMsg().clear();
  //   if ((size_t)c->getRequest().getBufferContentLength() == 0) {
  //     c->getRequest().setRecvPhase(MESSAGE_CGI_COMPLETE);
  //     return;
  //   }
  // }
}

// void MessageHandler::process_cgi_header_chunked(Connection *c) {
//   MessageHandler::response_handler_.setResponse(&c->getResponse());

//   size_t pos = c->cgi_output_temp.find("\r\n\r\n");
//   std::string cgi_header_lines = c->cgi_output_temp.substr(0, pos);
//   {
//     size_t pos = c->cgi_output_temp.find("\r\n\r\n");
//     cgi_header_lines = c->cgi_output_temp.substr(0, pos);
//     c->cgi_output_temp.erase(0, pos + 4);

//     while ((pos = cgi_header_lines.find("\r\n")) != std::string::npos) {
//       std::string one_header_line = cgi_header_lines.substr(0, pos);
//       std::vector<std::string> key_and_value = request_handler_.splitByDelimiter(one_header_line, SPACE);
//       // @sungyongcho: 저는 CGI 실행파일을 믿습니다...
//       // if (key_and_value.size() != 2)  // 400 Bad Request
//       //   ;
//       std::string key, value;
//       // parse key and validation
//       key = key_and_value[0].erase(key_and_value[0].size() - 1);
//       value = key_and_value[1];
//       // TODO: cgi의 위치를 한번 이동 할 예정...
//       // MessageHandler::response_handler_.setResponse(&c->getResponse());
//       if (key.compare("Status") == 0) {
//         // response_handler_.setStatusLineWithCode(value);
//         std::cout << "valu222222222: " << value << std::endl;
//         c->getResponse().setStatusCode(value);
//       }
//       std::cout << "keyeeee: " << key << std::endl;
//       std::cout << "valueeeee: " << value << std::endl;
//       if (key.compare("Status") != 0)
//         c->getResponse().setHeader(key, value);
//       cgi_header_lines.erase(0, pos + 2);
//     }
//   }
//   cgi_header_lines.clear();
// }

void MessageHandler::process_cgi_header(Connection *c) {
  MessageHandler::response_handler_.setResponse(&c->getResponse());

  std::string cgi_output_response_header;
  {
    size_t pos = c->getBodyBuf().find("\r\n\r\n");
    cgi_output_response_header = c->getBodyBuf().substr(0, pos);
    c->getBodyBuf().erase(0, pos + 4);
    while ((pos = cgi_output_response_header.find("\r\n")) != std::string::npos) {
      std::string one_header_line = cgi_output_response_header.substr(0, pos);
      std::vector<std::string> key_and_value = request_handler_.splitByDelimiter(one_header_line, SPACE);
      // @sungyongcho: 저는 CGI 실행파일을 믿습니다...
      // if (key_and_value.size() != 2)  // 400 Bad Request
      //   ;
      std::string key, value;
      // parse key and validation
      key = key_and_value[0].erase(key_and_value[0].size() - 1);
      value = key_and_value[1];
      // TODO: cgi의 위치를 한번 이동 할 예정...
      // MessageHandler::response_handler_.setResponse(&c->getResponse());
      if (key.compare("Status") == 0) {
        response_handler_.setStatusLineWithCode(value);
        c->status_code_ = value;
      }
      std::cout << "key: " << key << std::endl;
      std::cout << "value: " << value << std::endl;
      if (key.compare("Status") != 0)
        c->getResponse().setHeader(key, value);
      cgi_output_response_header.erase(0, pos + 2);
    }
  }
  // c->getResponse().setResponseBody(c->getBodyBuf());
  // TODO: 전체 리팩토링 하면서 시점 조절이 필요
  // if (!c->cgi_output_temp.empty()) {
  //   c->getResponse().setResponseBody(c->cgi_output_temp);
  // }
  // c->cgi_output_temp.clear();
}

std::string MessageHandler::parseCgiHeader(const std::string &cgi_output) {
  return (cgi_output);
}

// void MessageHandler::handle_response(Connection *c) {
//   //ResponseHandler response_handler_;
//   response_handler_.setResponse(&c->getResponse());
//   response_handler_.setServerConfig(c->getHttpConfig(), c->getSockaddrToConnect(), c->getRequest().getHeaderValue("Host"));
//   // TODO: HTTP/1.0 일 때 로직 복구 필요
//   // request에서 처리할지, response에서 처리할지 결정 필요

//   // if (c->getRequest().getHttpVersion() == "HTTP/1.0" && !c->getRequest().getHeaders().count("Host"))
//   //   c->getRequest().setHeader("Host", "");
//   // if (!isValidRequestMethod(c->getRequest().getMethod()) ||
//   //     !isValidRequestVersion(c->getRequest().getHttpVersion(), c->getRequest().getHeaders()))
//   //   response_handler_.setStatusLineWithCode("400");
//   // // else if (c->getBodyBuf().size() > 0)
//   // // {
//   // //   std::cout << "nigga" << std::endl;
//   // //   response_handler_.setStatusLineWithCode(c->status_code_);
//   // // }
//   // else
//     response_handler_.setResponseFields(c->getRequest());
//   response_handler_.makeResponseMsg();

//   // TODO: 이동가능
//   /// executePutMEthod가 있던 자리...
//   if (c->getRequest().getMethod() == "PUT" &&
//       (c->getResponse().getStatusCode() == "201" || (c->getResponse().getStatusCode() == "204"))) {
//     // create response body
//     executePutMethod(response_handler_.getAccessPath(c->getRequest().getUri()), c->getRequest().getEntityBody());
//   }
//   send(c->getFd(), c->getResponse().getMsg().c_str(), c->getResponse().getMsg().size(), 0);
// }

void MessageHandler::handle_response(Connection *c) {
  //ResponseHandler response_handler_;
  response_handler_.setResponse(&c->getResponse());
  response_handler_.setServerConfig(c->getHttpConfig(), c->getSockaddrToConnect(), c->getRequest().getHeaderValue("Host"));
  // TODO: HTTP/1.0 일 때 로직 복구 필요
  // request에서 처리할지, response에서 처리할지 결정 필요

  if (c->getRequest().getHttpVersion() == "HTTP/1.0" && !c->getRequest().getHeaders().count("Host"))
    c->getRequest().setHeader("Host", "");
  if (!isValidRequestMethod(c->getRequest().getMethod()) ||
      !isValidRequestVersion(c->getRequest().getHttpVersion(), c->getRequest().getHeaders()))
    response_handler_.setStatusLineWithCode("400");
  // else if (c->getBodyBuf().size() > 0)
  // {
  //   std::cout << "nigga" << std::endl;
  //   response_handler_.setStatusLineWithCode(c->status_code_);
  // }
  else
    response_handler_.setResponseFields(c->getRequest());
  response_handler_.makeResponseMsg();

  // TODO: 이동가능
  /// executePutMEthod가 있던 자리...
  if (c->getRequest().getMethod() == "PUT" &&
      (c->getResponse().getStatusCode() == "201" || (c->getResponse().getStatusCode() == "204"))) {
    // create response body
    executePutMethod(response_handler_.getAccessPath(c->getRequest().getUri()), c->getRequest().getEntityBody());
  }

  // if (c->getResponse().getHeaderValue("X-Powered-By") == "PHP/8.0.7" &&
  //     c->getResponse().getHeaderValue("Status").empty()) {
  //   return;
  // } else
  send(c->getFd(), c->getResponse().getMsg().c_str(), c->getResponse().getMsg().size(), 0);
}

void MessageHandler::executePutMethod(std::string path, std::string content) {
  std::ofstream output(path.c_str());
  output << content;
  output.close();
}

bool MessageHandler::isValidRequestMethod(const std::string &method) {
  if (method.compare("GET") && method.compare("POST") &&
      method.compare("DELETE") && method.compare("PUT") && method.compare("HEAD"))
    return (false);
  return (true);
}

bool MessageHandler::isValidRequestVersion(const std::string &http_version, const std::map<std::string, std::string> &headers) {
  if (!http_version.compare("HTTP/1.1")) {
    if (headers.count("Host"))
      return (true);
    return (false);
  } else if (!http_version.compare("HTTP/1.0")) {
    // if (!headers.count("Host"))
    //   request_->setHeader("Host", "");   // ?? 자동으로 ""되어있는거 아닌지?
    // A: 아닙니다. Response의 경우 응답 헤더들을 미리 설정 해 놓았지만 Request는 어떤 헤더가 들어 올지 모르기 때문에 설정 해주지 않았습니다.
    // HTTP/1.0 처리 하지 못하는 상황 발생하여 수정하겠습니다.
    return (true);
  }
  return (false);
}

}  // namespace ft
