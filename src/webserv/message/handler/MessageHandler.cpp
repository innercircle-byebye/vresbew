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
  // 2. request_handler의 request가 c의 request가 되도록 세팅
  request_handler_.setRequest(&c->getRequest());
  // 2. append (이전에 request가 setting되어야함)
  request_handler_.appendMsg(c->buffer_);
  // 4. process by recv_phase
  request_handler_.processByRecvPhase();

  // 5. clear c->buffer_
  memset(c->buffer_, 0, recv_len);
}

void MessageHandler::handle_cgi(Connection *c, LocationConfig *location) {
  char **environ;
  char **command;
  pid_t pid;
  int pipe_fd[2];
  char foo[4096];
  std::map<std::string, std::string> env_set;
  {
    env_set["CONTENT_LENGTH"] = c->getRequest().getHeaderValue("Content-Length");
    env_set["QUERY_STRING"] = c->getRequest().getEntityBody();
    env_set["REQUEST_METHOD"] = c->getRequest().getMethod();
    env_set["REDIRECT_STATUS"] = "CGI";
    env_set["SCRIPT_FILENAME"] = response_handler_.getAccessPath(c->getRequest().getUri(), location);
    env_set["SERVER_PROTOCOL"] = "HTTP/1.1";
    env_set["PATH_INFO"] = response_handler_.getAccessPath(c->getRequest().getUri(), location);
    env_set["CONTENT_TYPE"] = "test/file";
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
  pipe(pipe_fd);
  pid = fork();
  if (!pid) {
    dup2(pipe_fd[1], STDOUT_FILENO);
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    execve(location->getCgiPath().c_str(), command, environ);
    exit(1);
  } else {
    close(pipe_fd[1]);
    int nbytes;
    int i = 0;
    while ((nbytes = read(pipe_fd[0], foo, sizeof(foo)))) {
      cgi_output_temp.append(foo);
      i++;
    }
    std::cout << "i: " << nbytes << std::endl;
    // write(STDOUT_FILENO, foo, strlen(foo));
    wait(NULL);
  }
  c->getResponse().setResponseBody(cgi_output_temp);
}

void MessageHandler::handle_response(Connection *c) {
  //ResponseHandler response_handler_;

  response_handler_.setResponse(&c->getResponse());
  response_handler_.setServerConfig(c->getHttpConfig(), c->getSockaddrToConnect(), c->getRequest().getHeaderValue("Host"));

  // TODO: HTTP/1.0 일 때 로직 복구 필요
  // if (c->getRequest().getHttpVersion() == "HTTP/1.0" && !c->getRequest().getHeaders().count("Host") )
  //   c->getRequest().setHeader("Host", "");

  if (!isValidRequestMethod(c->getRequest().getMethod()) ||
      !isValidRequestVersion(c->getRequest().getHttpVersion(), c->getRequest().getHeaders()))
    response_handler_.setStatusLineWithCode("400");
  else
    response_handler_.setResponseFields(c->getRequest());

  response_handler_.makeResponseMsg();

  /// executePutMEthod가 있던 자리...
  if (c->getRequest().getMethod() == "PUT" &&
      (c->getResponse().getStatusCode() == "201" || (c->getResponse().getStatusCode() == "204"))) {
    std::cout << "in message handler it catched " << std::endl;
    // create response body
    executePutMethod(response_handler_.getAccessPath(c->getRequest().getUri()), c->getRequest().getEntityBody());
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
