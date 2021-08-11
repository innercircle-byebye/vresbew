#include "webserv/message/handler/CgiHandler.hpp"

namespace ft {

void CgiHandler::init_cgi_child(Connection *c) {
  ServerConfig *server_config = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
  LocationConfig *location = server_config->getLocationConfig(c->getRequest().getUri());
  // char **environ;
  // char **command;
  // environ = setEnviron(c);
  // command = setCommand(location->getCgiPath(), location->getRoot() + c->getRequest().getUri());
  std::string cgi_output_temp;
  // TODO: 실패 예외처리
  pipe(c->writepipe);
  // TODO: 실패 예외처리
  pipe(c->readpipe);
  c->cgi_pid = fork();
  if (!c->cgi_pid) {
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

    // execve(location->getCgiPath().c_str(), command, environ);
    execve(location->getCgiPath().c_str(),
           setCommand(location->getCgiPath(), c->getRequest().getFilePath()),
           setEnviron(c));
  }
  // TODO: 실패 예외처리
  close(c->writepipe[0]);
  // TODO: 실패 예외처리
  close(c->readpipe[1]);
  memset(c->buffer_, 0, BUF_SIZE);

}

void CgiHandler::handle_cgi_header(Connection *c) {
  MessageHandler::response_handler_.setResponse(&c->getResponse(), &c->getBodyBuf());

  // std::cout << "am i even working" << std::endl;
  // std::cout << "temp_size 1: [" << c->temp.size() << "]" << std::endl;

  // nbytes = read(c->readpipe[0], c->buffer_, BUF_SIZE);
  // std::cout << "c->buffer" << c->buffer_ << std::endl;
  // c->appendBodyBuf(c->buffer_);

  std::string cgi_output_response_header;
  {
    size_t pos = c->temp.find("\r\n\r\n");
    cgi_output_response_header = c->temp.substr(0, pos + 2);
    c->temp.erase(0, pos + 4);
    while ((pos = cgi_output_response_header.find("\r\n")) != std::string::npos) {
      std::string one_header_line = cgi_output_response_header.substr(0, pos);
      std::vector<std::string> key_and_value = MessageHandler::request_handler_.splitByDelimiter(one_header_line, SPACE);
      // @sungyongcho: 저는 CGI 실행파일을 믿습니다...
      // if (key_and_value.size() != 2)  // 400 Bad Request
      //   ;
      std::string key, value;
      // parse key and validation
      key = key_and_value[0].erase(key_and_value[0].size() - 1);
      value = key_and_value[1];
      if (key.compare("Status") == 0) {
        // TODO: c->status_code 만 이용해서 handle_cgi_header 단계 이후에 사용하도록..
        MessageHandler::response_handler_.setStatusLineWithCode(stoi(value));
        c->status_code_ = stoi(value);
      }
      // std::cout << "key: " << key << std::endl;
      // std::cout << "value: " << value << std::endl;
      if (key.compare("Status") != 0)
        c->getResponse().setHeader(key, value);
      cgi_output_response_header.erase(0, pos + 2);
    }
  }
}

char **CgiHandler::setEnviron(Connection *c) {
  ServerConfig *server_config = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
  LocationConfig *location = server_config->getLocationConfig(c->getRequest().getUri());
  std::map<std::string, std::string> env_set;
  {
    if (!c->getRequest().getHeaderValue("Content-Length").empty()) {
      env_set["CONTENT_LENGTH"] = c->getRequest().getHeaderValue("Content-Length");
    } else {
      env_set["CONTENT_LENGTH"] = SSTR(c->getBodyBuf().size());
    }
    if (!c->getRequest().getHeaderValue("X-Secret-Header-For-Test").empty())
      env_set["HTTP_X_SECRET_HEADER_FOR_TEST"] = c->getRequest().getHeaderValue("X-Secret-Header-For-Test");
    if (c->getRequest().getMethod() == "GET") {
      // TODO: getEntityBody 삭제 필요, 구조체로 변경
      env_set["QUERY_STRING"] = c->getRequest().getQueryString();
    }
    env_set["REQUEST_METHOD"] = c->getRequest().getMethod();
    env_set["REDIRECT_STATUS"] = "CGI";
    env_set["SCRIPT_FILENAME"] = c->getRequest().getFilePath();
    env_set["SERVER_PROTOCOL"] = "HTTP/1.1";
    env_set["PATH_INFO"] = c->getRequest().getPath();
    env_set["CONTENT_TYPE"] = c->getRequest().getHeaderValue("Content-Type");
    env_set["GATEWAY_INTERFACE"] = "CGI/1.1";
    env_set["PATH_TRANSLATED"] = c->getRequest().getFilePath();
    env_set["REMOTE_ADDR"] = "127.0.0.1";  // TODO: ip주소 받아오는 부분 찾기
    env_set["REQUEST_URI"] = c->getRequest().getUri();
    env_set["HTTP_HOST"] = c->getRequest().getHeaderValue("Host");
    env_set["SERVER_PORT"] = std::to_string(ntohs(c->getSockaddrToConnect().sin_port));
    env_set["SERVER_SOFTWARE"] = "versbew";
    env_set["SCRIPT_NAME"] = location->getCgiPath();
  }
  char **return_value;
  std::string temp;

  return_value = (char **)malloc(sizeof(char *) * (env_set.size() + 1));
  int i = 0;
  std::map<std::string, std::string>::iterator it;
  for (it = env_set.begin(); it != env_set.end(); it++) {
    temp = (*it).first + "=" + (*it).second;
    char *p = (char *)malloc(temp.size() + 1);
    strcpy(p, temp.c_str());
    return_value[i] = p;
    i++;
  }
  return_value[i] = NULL;
  return (return_value);
}

char **CgiHandler::setCommand(std::string command, std::string path) {
  // TODO: leak check
  char **return_value;
  return_value = (char **)malloc(sizeof(char *) * (3));

  char *temp;
  // TODO: leak check
  temp = (char *)malloc(sizeof(char) * (command.size() + 1));
  strcpy(temp, command.c_str());
  return_value[0] = temp;

  // TODO: leak check
  temp = (char *)malloc(sizeof(char) * (path.size() + 1));
  strcpy(temp, path.c_str());
  return_value[1] = temp;
  return_value[2] = NULL;
  return (return_value);
}

void CgiHandler::receive_cgi_body(Connection *c) {
  size_t nbytes;
  if (c->getRecvPhase() == MESSAGE_CGI_COMPLETE) {
    while ((nbytes = read(c->readpipe[0], c->buffer_, BUF_SIZE))) {
      c->appendBodyBuf(c->buffer_);
      memset(c->buffer_, 0, nbytes);
    }
    close(c->readpipe[0]);
    close(c->readpipe[1]);
    close(c->writepipe[0]);
    close(c->writepipe[1]);
    wait(NULL);
  }
}

void CgiHandler::setup_cgi_message(Connection *c) {
  // std::cout << "status_code: [" << c->status_code_ << "]" << std::endl;
  if (c->status_code_ < 0 && !c->getResponse().getHeaderValue("X-Powered-By").compare("PHP/8.0.7")) {
    c->status_code_ = 200;
    MessageHandler::response_handler_.setStatusLineWithCode(200);
  }
  if (c->getRequest().getHeaderValue("Content-Length").empty())
    c->getResponse().setHeader("Content-Length", SSTR(c->temp.size()));
  // c->getResponse().setHeader("Transfer-Encoding", "chunked");
  MessageHandler::response_handler_.setDefaultHeader(c, c->getRequest());
  c->getResponse().setHeader("Content-Length", SSTR(c->temp.size()));
  c->getResponse().setHeader("Content-Type", "text/html; charset=utf-8");
  // c->getResponse().setHeader("Connection", "close");

  MessageHandler::response_handler_.makeResponseHeader();
  // std::cout << "================temp=============" << std::endl;
  // std::cout << c->temp << std::endl;
  // std::cout << "================temp=============" << std::endl;
  // std::cout << "temp_size :" << c->temp.size() << std::endl;
  // std::cout << "================header=============" << std::endl;
  // std::cout << c->getResponse().getHeaderMsg().c_str() << std::endl;
  // std::cout << "================header=============" << std::endl;
  if (!c->temp.empty())
    c->getResponse().getHeaderMsg().append(c->temp);
  // std::cout << "temp_size :" << c->temp.size() << std::endl;
  // std::cout << "size      :" << c->getResponse().getHeaderMsg().size() << std::endl;

  // c->temp.clear();

  c->send_len = 0;
  close(c->readpipe[0]);
  close(c->readpipe[1]);
  close(c->writepipe[0]);
  close(c->writepipe[1]);
  wait(NULL);
}
}  // namespace ft
