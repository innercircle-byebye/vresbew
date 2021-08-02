#include "webserv/message/handler/CgiHandler.hpp"

namespace ft {

void CgiHandler::init_cgi_child(Connection *c) {
  ServerConfig *server_config = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
  LocationConfig *location = server_config->getLocationConfig(c->getRequest().getPath());
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
           setCommand(location->getCgiPath(), location->getRoot() + c->getRequest().getPath()),
           setEnviron(c));
  }
  // TODO: 실패 예외처리
  close(c->writepipe[0]);
  // TODO: 실패 예외처리
  close(c->readpipe[1]);

  if (!c->getBodyBuf().empty()) {
    write(c->writepipe[1], c->getBodyBuf().c_str(), (size_t)c->getBodyBuf().size());
    c->setStringBufferContentLength(c->getStringBufferContentLength() - c->getBodyBuf().size());
    c->getBodyBuf().clear();  // 뒤에서 또 쓰일걸 대비해 혹시몰라 초기화.. #2
  }
  // TODO: 조건 확인후 변경
  if ((c->getRequest().getMethod() == "POST" && c->getRequest().getHeaderValue("Content-Length").empty()) ||
      c->getStringBufferContentLength() > 0)  // POST 요청에 request 헤더에 Content-Length가 안 주어 졌다
    c->setRecvPhase(MESSAGE_CGI_INCOMING);
  else {
    c->setStringBufferContentLength(0);  // 뒤에서 또 쓰일걸 대비해 혹시몰라 초기화.. #2
    c->setRecvPhase(MESSAGE_CGI_COMPLETE);
  }
}

void CgiHandler::handle_cgi_header(Connection *c) {
  MessageHandler::response_handler_.setResponse(&c->getResponse(), &c->getBodyBuf());

  std::cout << "am i even working" << std::endl;
  size_t nbytes;
  nbytes = read(c->readpipe[0], c->buffer_, BUF_SIZE);
  c->appendBodyBuf(c->buffer_);
  memset(c->buffer_, 0, nbytes);

  std::string cgi_output_response_header;
  {
    size_t pos = c->getBodyBuf().find("\r\n\r\n");
    cgi_output_response_header = c->getBodyBuf().substr(0, pos);
    c->getBodyBuf().erase(0, pos + 4);
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
        MessageHandler::response_handler_.setStatusLineWithCode(value);
        c->status_code_ = value;
      }
      std::cout << "key: " << key << std::endl;
      std::cout << "value: " << value << std::endl;
      if (key.compare("Status") != 0)
        c->getResponse().setHeader(key, value);
      cgi_output_response_header.erase(0, pos + 2);
    }
  }
}

void CgiHandler::receive_cgi_process_body(Connection *c, ssize_t recv_len) {
  std::cout << "i'm here" << std::endl;
  // 한번의 버퍼 안에 전체 메세지가 다 들어 올 경우
  if (recv_len == -1)
    return;
  // Transfer-Encoding : chunked 아닐 때 (= Content-Length가 있을 때)
  if (c->getStringBufferContentLength() > 0) {
    if (c->getStringBufferContentLength() > recv_len) {
      c->setStringBufferContentLength(c->getStringBufferContentLength() - recv_len);
      write(c->writepipe[1], c->buffer_, recv_len);
    } else {
      write(c->writepipe[1], c->buffer_, recv_len);
      c->setStringBufferContentLength(0);
      c->setRecvPhase(MESSAGE_CGI_COMPLETE);
      close(c->writepipe[1]);
    }
  }
  // Request에서 Content-Length 헤더가 주어지지 않을 때
  // (= Transfer-Encoding: chunked 헤더가 주어 젔을 때)
  else {
    if (!c->getRequest().getMsg().empty()) {
      std::cout << "hihi" << std::endl;
      size_t pos;

      while (c->getRequest().getMsg().find("\r\n") != std::string::npos) {
        pos = c->getRequest().getMsg().find("\r\n");
        std::string temp_msg = c->getRequest().getMsg().substr(0, pos + 2);
        std::cout << "tempsize: " << pos << std::endl;

        std::cout << "before: " << temp_msg << std::endl;
        write(c->writepipe[1], temp_msg.c_str(), (size_t)temp_msg.size());
        if (temp_msg == "0\r\n") {
          c->chunked_checker = CHUNKED_ZERO_RN_RN;
          // } else if (c->getRequest().getMsg().substr(0, pos) == "0\r\n\r\n") {
          //   c->getRequest().setRecvPhase(MESSAGE_CGI_COMPLETE);
          //   close(c->writepipe[1]);
        } else if (c->chunked_checker == CHUNKED_ZERO_RN_RN && temp_msg == "\r\n") {
          c->setRecvPhase(MESSAGE_CGI_COMPLETE);
          close(c->writepipe[1]);
        } else {
          c->chunked_checker = CHUNKED_KEEP_COMING;
        }
        c->getRequest().getMsg().erase(0, pos + 2);
        temp_msg.clear();
        // std::cout << "after: " << c->getRequest().getMsg() << std::endl;
      }
      c->getRequest().getMsg().clear();
    }
    if (c->chunked_checker == CHUNKED_ZERO_RN_RN) {
      std::cout << "hi" << std::endl;
    }
    std::cout << "aftermath: " << c->buffer_ << std::endl;
    write(c->writepipe[1], c->buffer_, recv_len);
    if (!strcmp(c->buffer_, "0\r\n")) {
      c->chunked_checker = CHUNKED_ZERO_RN_RN;
      // } else if (!strcmp(c->buffer_, "0\r\n\r\n")) {
      //   c->getRequest().setRecvPhase(MESSAGE_CGI_COMPLETE);
      //   close(c->writepipe[1]);
    } else if (c->chunked_checker == CHUNKED_ZERO_RN_RN && !strcmp(c->buffer_, "\r\n")) {
      c->setRecvPhase(MESSAGE_CGI_COMPLETE);
      close(c->writepipe[1]);
    } else
      c->chunked_checker = CHUNKED_KEEP_COMING;
  }
}

char **CgiHandler::setEnviron(Connection *c) {
  ServerConfig *server_config = c->getHttpConfig()->getServerConfig(c->getSockaddrToConnect().sin_port, c->getSockaddrToConnect().sin_addr.s_addr, c->getRequest().getHeaderValue("Host"));
  LocationConfig *location = server_config->getLocationConfig(c->getRequest().getPath());
  std::map<std::string, std::string> env_set;
  {
    if (!c->getRequest().getHeaderValue("Content-Length").empty()) {
      env_set["CONTENT_LENGTH"] = c->getRequest().getHeaderValue("Content-Length");
    }
    if (c->getRequest().getMethod() == "GET") {
      // TODO: getEntityBody 삭제 필요, 구조체로 변경
      env_set["QUERY_STRING"] = c->getRequest().getQueryString();
    }
    env_set["REQUEST_METHOD"] = c->getRequest().getMethod();
    env_set["REDIRECT_STATUS"] = "CGI";
    env_set["SCRIPT_FILENAME"] = location->getRoot() + c->getRequest().getPath();
    env_set["SERVER_PROTOCOL"] = "HTTP/1.1";
    env_set["PATH_INFO"] = location->getRoot() + c->getRequest().getPath();
    env_set["CONTENT_TYPE"] = c->getRequest().getHeaderValue("Content-Type");
    env_set["GATEWAY_INTERFACE"] = "CGI/1.1";
    env_set["PATH_TRANSLATED"] = location->getRoot() + c->getRequest().getPath();
    env_set["REMOTE_ADDR"] = "127.0.0.1";  // TODO: ip주소 받아오는 부분 찾기
    if (c->getRequest().getMethod() == "GET")
      env_set["REQUEST_URI"] = c->getRequest().getUri();
    else
      env_set["REQUEST_URI"] = location->getRoot() + c->getRequest().getPath();
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

void CgiHandler::send_chunked_cgi_response_to_client_and_close(Connection *c) {
  size_t nbytes;
  MessageHandler::response_handler_.makeResponseHeader();
  send(c->getFd(), c->getResponse().getHeaderMsg().c_str(), c->getResponse().getHeaderMsg().size(), 0);
  send(c->getFd(), c->getBodyBuf().c_str(), (size_t)c->getBodyBuf().size(), 0);
  while ((nbytes = read(c->readpipe[0], c->buffer_, BUF_SIZE))) {
    send(c->getFd(), c->buffer_, nbytes, 0);
    memset(c->buffer_, 0, nbytes);
  }
  close(c->readpipe[0]);
  close(c->readpipe[1]);
  close(c->writepipe[0]);
  close(c->writepipe[1]);
  // send(c->getFd(), &"0", 1, 0);
  wait(NULL);
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
}  // namespace ft
