#include "webserv/message/handler/MessageHandler.hpp"

namespace ft {

RequestHandler MessageHandler::request_handler_ = RequestHandler();
ResponseHandler MessageHandler::response_handler_ = ResponseHandler();

MessageHandler::MessageHandler() {}

MessageHandler::~MessageHandler() {}

void MessageHandler::handle_request(Connection *c) {
  //RequestHandler  request_handler_;

  // 1. recv
  size_t recv_len = recv(c->getFd(), c->buffer_, BUF_SIZE, 0);
  // recv(c->getFd(), c->buffer_, BUF_SIZE, 0);
  // 2. request_handler의 request가 c의 request가 되도록 세팅
  request_handler_.setRequest(&c->getRequest());
  // 2. append (이전에 request가 setting되어야함)
  request_handler_.appendMsg(c->buffer_);
  // 4. process by recv_phase
  request_handler_.processByRecvPhase(c);
  // 5. clear c->buffer_
  memset(c->buffer_, 0, recv_len);
}

void MessageHandler::handle_cgi(Connection *c, LocationConfig *location) {
  char **environ;
  char **command;
  pid_t pid;

  std::map<std::string, std::string> env_set;
  {
    if (!c->getRequest().getHeaderValue("Content-Length").empty()) {
      std::cout << c->getRequest().getHeaderValue("Content-Length") << std::endl;
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
    env_set["REQUEST_URI"] = response_handler_.getAccessPath(c->getRequest().getUri(), location);
    env_set["SERVER_PORT"] = std::to_string(ntohs(c->getSockaddrToConnect().sin_port));  // 포트도
    env_set["SERVER_SOFTWARE"] = "versbew";
    env_set["SCRIPT_NAMME"] = location->getCgiPath();
  }
  environ = response_handler_.setEnviron(env_set);
  command = response_handler_.setCommand(location->getCgiPath(), response_handler_.getAccessPath(c->getRequest().getUri(), location));

  std::string cgi_output_temp;
  pipe(c->writepipe);
  pipe(c->readpipe);
  pid = fork();

  if (!pid) {
    close(c->writepipe[1]);
    close(c->readpipe[0]);
    dup2(c->writepipe[0], 0);
    close(c->writepipe[0]);
    dup2(c->readpipe[1], 1);
    close(c->readpipe[1]);
    execve(location->getCgiPath().c_str(), command, environ);
  }
  close(c->writepipe[0]);
  close(c->readpipe[1]);

  if (c->getRequest().getMethod() == "GET") {
    c->getRequest().setRecvPhase(MESSAGE_CGI_COMPLETE);
    return;
  }
  if (!c->getRequest().getMsg().empty()) {
    write(c->writepipe[1], c->getRequest().getMsg().c_str(), static_cast<size_t>(c->getRequest().getMsg().size()));
    c->getRequest().setBufferContentLength(c->getRequest().getBufferContentLength() - c->getRequest().getMsg().size());
    c->getRequest().getMsg().clear();
    if ((size_t)c->getRequest().getBufferContentLength() == 0) {
      c->getRequest().setRecvPhase(MESSAGE_CGI_COMPLETE);
      return;
    }
  }
  c->getRequest().setRecvPhase(MESSAGE_CGI_INCOMING);
}

void MessageHandler::process_cgi_response(Connection *c) {
  MessageHandler::response_handler_.setResponse(&c->getResponse());

  std::string cgi_output_response_header;
  {
    size_t pos = c->cgi_output_temp.find("\r\n\r\n");
    cgi_output_response_header = c->cgi_output_temp.substr(0, pos);

    c->cgi_output_temp.erase(0, pos + 4);
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
      if (!key.compare("Status") && value.compare("404")) {
        response_handler_.setStatusLineWithCode(value);
      }
      std::cout << "key: " << key << std::endl;
      std::cout << "value: " << value << std::endl;
      c->getResponse().setHeader(key, value);
      cgi_output_response_header.erase(0, pos + 2);
    }
  }
  // TODO: 전체 리팩토링 하면서 시점 조절이 필요
  if (c->getResponse().getStatusCode() != "404") {
    c->getResponse().setResponseBody(c->cgi_output_temp);
  }
  c->cgi_output_temp.clear();
  cgi_output_response_header.clear();

  c->getRequest().setRecvPhase(MESSAGE_BODY_COMPLETE);
}
std::string MessageHandler::parseCgiHeader(const std::string &cgi_output) {
  return (cgi_output);
}

void MessageHandler::handle_response(Connection *c) {
  //ResponseHandler response_handler_;
  response_handler_.setResponse(&c->getResponse());
  response_handler_.setServerConfig(c->getHttpConfig(), c->getSockaddrToConnect(), c->getRequest().getHeaderValue("Host"));

  // TODO: HTTP/1.0 일 때 로직 복구 필요
  if (c->getRequest().getHttpVersion() == "HTTP/1.0" && !c->getRequest().getHeaders().count("Host"))
    c->getRequest().setHeader("Host", "");

  if (!isValidRequestMethod(c->getRequest().getMethod()) ||
      !isValidRequestVersion(c->getRequest().getHttpVersion(), c->getRequest().getHeaders()))
    response_handler_.setStatusLineWithCode("400");
  else
    response_handler_.setResponseFields(c->getRequest());

  response_handler_.makeResponseMsg();

  /// executePutMEthod가 있던 자리...
  if (c->getRequest().getMethod() == "PUT" &&
      (c->getResponse().getStatusCode() == "201" || (c->getResponse().getStatusCode() == "204"))) {
    // create response body
    executePutMethod(response_handler_.getAccessPath(c->getRequest().getUri()), c->getRequest().getEntityBody());
  }

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
