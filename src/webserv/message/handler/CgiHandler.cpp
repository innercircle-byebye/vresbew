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

  // std::cout << "check body buf" << std::endl;
  // std::cout << c->getBodyBuf() << std::endl;
  // std::cout << "check body buf" << std::endl;
  if (c->getBodyBuf().size() == 0) {  //자식 프로세스로 보낼 c->body_buf_ 가 비어있는 경우 파이프 닫음
    close(c->writepipe[1]);
  } else {
    // // TODO: 수정 필요
    size_t size = c->getBodyBuf().size();
    std::cout << "================body_buf_size=============" << std::endl;
    std::cout << c->getBodyBuf().size() << std::endl;
    std::cout << "================body_buf_size=============" << std::endl;

    for (size_t i = 0; i <= size; i += BUF_SIZE) {
      // std::cout << "i: [" << i << "]" << std::endl;
      write(c->writepipe[1], c->getBodyBuf().substr(i, BUF_SIZE + i).c_str(), BUF_SIZE);
      // c->getBodyBuf().erase(i, BUF_SIZE + i);
      if (i == 0) {
        read(c->readpipe[0], c->buffer_, BUF_SIZE);
        c->temp.append(c->buffer_);
      }
      read(c->readpipe[0], c->buffer_, BUF_SIZE);
      c->temp.append(c->buffer_);
      memset(c->buffer_, 0, BUF_SIZE);
      // std::cout << c->buffer_ << std::endl;
    }
    //숫자 확인
    c->setStringBufferContentLength(-1);
    c->getBodyBuf().clear();  // 뒤에서 또 쓰일걸 대비해 혹시몰라 초기화.. #2
  }

  c->setRecvPhase(MESSAGE_CGI_COMPLETE);
}

void CgiHandler::handle_cgi_header(Connection *c) {
  MessageHandler::response_handler_.setResponse(&c->getResponse(), &c->getBodyBuf());

  std::cout << "am i even working" << std::endl;
  std::cout << "temp_size 1: [" << c->temp.size() << "]" << std::endl;

  // nbytes = read(c->readpipe[0], c->buffer_, BUF_SIZE);
  // std::cout << "c->buffer" << c->buffer_ << std::endl;
  // c->appendBodyBuf(c->buffer_);

  std::string cgi_output_response_header;
  {
    size_t pos = c->temp.find("\r\n\r\n");
    cgi_output_response_header = c->temp.substr(0, pos);
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
      std::cout << "key: " << key << std::endl;
      std::cout << "value: " << value << std::endl;
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

void CgiHandler::send_chunked_cgi_response_to_client_and_close(Connection *c) {
  c->getResponse().setHeader("Content-Length", SSTR(c->temp.size()));
  MessageHandler::response_handler_.makeResponseHeader();
  std::cout << "hihi" << std::endl;
  std::cout << "================header=============" << std::endl;
  std::cout << c->getResponse().getHeaderMsg().c_str() << std::endl;
  std::cout << "================header=============" << std::endl;

  send(c->getFd(), c->getResponse().getHeaderMsg().c_str(), c->getResponse().getHeaderMsg().size(), 0);
  // send(c->getFd(), c->getBodyBuf().c_str(), (size_t)c->getBodyBuf().size(), 0);

  size_t size = c->temp.size();
  std::cout << "temp_size 2: [" << c->temp.size() << "]" << std::endl;

  for (size_t i = 0; i <= size; i += 100000) {
    std::cout << "i: [" << i << "]" << std::endl;

    send(c->getFd(), c->temp.substr(i, 100000 + i).c_str(), 100000, 0);
  }
  // while ((nbytes = read(c->readpipe[0], c->buffer_, BUF_SIZE))) {
  //   send(c->getFd(), c->buffer_, nbytes, 0);
  //   memset(c->buffer_, 0, nbytes);
  // }
  std::cout << "byebye" << std::endl;
  std::cout << *(c->temp.rbegin()) << std::endl;

  close(c->readpipe[0]);
  close(c->readpipe[1]);
  close(c->writepipe[0]);
  close(c->writepipe[1]);
  // send(c->getFd(), &"\0", 1, 0);
  waitpid(c->cgi_pid, NULL, 0);
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
