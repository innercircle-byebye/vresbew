#include "webserv/message/handler/CgiHandler.hpp"

namespace ft {

void CgiHandler::initCgiChild(Connection *c) {
  std::string cgi_output_temp;
  // TODO: 실패 예외처리
  pipe(c->writepipe);
  // TODO: 실패 예외처리
  pipe(c->readpipe);

  char **cgi_command;
  char **env_variable;

  if ((env_variable = setEnviron(c)) == NULL) {
    Logger::logError(LOG_ALERT, "cgi process environment varible creating failed (malloc failed)");
    c->req_status_code_ = 500;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if ((cgi_command = setCommand(c->getLocationConfig()->getCgiPath(), c->getRequest().getFilePath())) == NULL) {
    Logger::logError(LOG_ALERT, "cgi process command creating failed (malloc failed)");
    c->req_status_code_ = 500;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if ((c->cgi_pid = fork()) == -1) {
    close(c->readpipe[0]);
    close(c->readpipe[1]);
    close(c->writepipe[0]);
    close(c->writepipe[1]);
    Logger::logError(LOG_ALERT, "cgi process fork failed");
    c->req_status_code_ = 500;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (c->cgi_pid == 0) {
    close(c->writepipe[1]);
    close(c->readpipe[0]);
    dup2(c->writepipe[0], STDIN_FILENO);
    close(c->writepipe[0]);
    dup2(c->readpipe[1], STDOUT_FILENO);
    close(c->readpipe[1]);

    execve(c->getLocationConfig()->getCgiPath().c_str(), cgi_command, env_variable);
  }
  close(c->writepipe[0]);
  close(c->readpipe[1]);
  memset(c->buffer_, 0, BUF_SIZE);

  if (c->getBodyBuf().size() == 0) {
    close(c->writepipe[1]);
    ssize_t read_len_1_;
    while ((read_len_1_ = read(c->readpipe[0], c->buffer_, BUF_SIZE - 1)) != 0) {
      if (read_len_1_ == -1) {
        close(c->readpipe[0]);
        close(c->readpipe[1]);
        close(c->writepipe[0]);
        close(c->writepipe[1]);
        c->req_status_code_ = 503;
        c->setRecvPhase(MESSAGE_BODY_COMPLETE);
        return;
      }
      c->temp.append(c->buffer_);
    }
  } else {
    ssize_t read_len_2_ = 0;
    ssize_t write_len_ = 0;
    bool write_end = false;
    bool read_end = false;
    size_t size = c->getBodyBuf().size();
    for (size_t i = 0; i < size; i += BUF_SIZE - 1) {
      size_t j = std::min(size, BUF_SIZE + i - 1);
      if (write_end == false) {
        write_len_ = write(c->writepipe[1], c->getBodyBuf().substr(i, j).c_str(), j - i);
        std::cout << "write_len:[" << write_len_ << "]" << std::endl;
        if (write_len_ == -1) {
          std::cout << "aaaa" << std::endl;
          close(c->readpipe[0]);
          close(c->readpipe[1]);
          close(c->writepipe[0]);
          close(c->writepipe[1]);
          c->req_status_code_ = 503;
          c->setRecvPhase(MESSAGE_BODY_COMPLETE);
          return;
        } else if (write_len_ == 0) {
          write_end = true;
        }
      }
      if (read_end == false) {
        if (i == 0) {
          read_len_2_ = read(c->readpipe[0], c->buffer_, BUF_SIZE - 1);
          if (read_len_2_ == -1) {
            close(c->readpipe[0]);
            close(c->readpipe[1]);
            close(c->writepipe[0]);
            close(c->writepipe[1]);
            c->req_status_code_ = 503;
            c->setRecvPhase(MESSAGE_BODY_COMPLETE);
            return;
          }
          c->temp.append(c->buffer_);
          memset(c->buffer_, 0, BUF_SIZE);
        }
        read_len_2_ = read(c->readpipe[0], c->buffer_, BUF_SIZE - 1);
        std::cout << "read_len_2:[" << read_len_2_ << "]" << std::endl;
        if (read_len_2_ == -1) {
          close(c->readpipe[0]);
          close(c->readpipe[1]);
          close(c->writepipe[0]);
          close(c->writepipe[1]);
          c->req_status_code_ = 503;
          c->setRecvPhase(MESSAGE_BODY_COMPLETE);
          return;
        }
        if (read_len_2_ == 0)
          read_end = true;
      }
      if (read_end != write_end || (read_len_2_ != write_len_)) {
        close(c->readpipe[0]);
        close(c->readpipe[1]);
        close(c->writepipe[0]);
        close(c->writepipe[1]);
        c->req_status_code_ = 503;
        c->setRecvPhase(MESSAGE_BODY_COMPLETE);
        return;
      }
      c->temp.append(c->buffer_);
      memset(c->buffer_, 0, BUF_SIZE);
    }
    c->setStringBufferContentLength(-1);
    c->getBodyBuf().clear();
    close(c->writepipe[1]);
  }
  c->setRecvPhase(MESSAGE_CGI_COMPLETE);
}

void CgiHandler::handleCgiHeader(Connection *c) {
  MessageHandler::response_handler_.setResponse(&c->getResponse(), &c->getBodyBuf());

  std::string cgi_output_response_header;
  {
    size_t pos = c->temp.find(CRLFCRLF);
    cgi_output_response_header = c->temp.substr(0, pos + CRLF_LEN);
    c->temp.erase(0, pos + CRLFCRLF_LEN);
    while ((pos = cgi_output_response_header.find(CRLF)) != std::string::npos) {
      std::string one_header_line = cgi_output_response_header.substr(0, pos);
      std::vector<std::string> key_and_value = MessageHandler::request_handler_.splitByDelimiter(one_header_line, SPACE);
      // @sungyongcho: 저는 CGI 실행파일을 믿습니다...
      // if (key_and_value.size() != 2)  // 400 Bad Request
      //   ;
      std::string key, value;
      key = key_and_value[0].erase(key_and_value[0].size() - 1);
      value = key_and_value[1];
      if (key.compare("Status") == 0) {
        MessageHandler::response_handler_.setStatusLineWithCode(stoi(value));
        c->req_status_code_ = stoi(value);
      }
      if (key.compare("Status") != 0)
        c->getResponse().setHeader(key, value);
      cgi_output_response_header.erase(0, pos + CRLF_LEN);
    }
  }
}

char **CgiHandler::setEnviron(Connection *c) {
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
    env_set["SERVER_PORT"] = SSTR(ntohs(c->getSockaddrToConnect().sin_port));
    env_set["SERVER_SOFTWARE"] = c->getLocationConfig()->getProgramName();
    env_set["SCRIPT_NAME"] = c->getLocationConfig()->getCgiPath();
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
  char **return_value;
  return_value = (char **)malloc(sizeof(char *) * (3));

  char *temp;
  temp = (char *)malloc(sizeof(char) * (command.size() + 1));
  strcpy(temp, command.c_str());
  return_value[0] = temp;

  temp = (char *)malloc(sizeof(char) * (path.size() + 1));
  strcpy(temp, path.c_str());
  return_value[1] = temp;
  return_value[2] = NULL;
  return (return_value);
}


void CgiHandler::setupCgiMessage(Connection *c) {
  if (c->req_status_code_ == NOT_SET && !c->getResponse().getHeaderValue("X-Powered-By").compare("PHP/8.0.7")) {
    c->req_status_code_ = 200;
    MessageHandler::response_handler_.setStatusLineWithCode(200);
  }
  if (c->getRequest().getHeaderValue("Content-Length").empty())
    c->getResponse().setHeader("Content-Length", SSTR(c->temp.size()));

  MessageHandler::response_handler_.setDefaultHeader(c, c->getRequest());
  c->getResponse().setHeader("Content-Length", SSTR(c->temp.size()));
  MessageHandler::response_handler_.makeResponseHeader();

  if (!c->temp.empty())
    c->getResponse().getHeaderMsg().append(c->temp);

  c->send_len = 0;
  close(c->readpipe[0]);
  close(c->readpipe[1]);
  close(c->writepipe[0]);
  close(c->writepipe[1]);
  wait(NULL);
}
}  // namespace ft
