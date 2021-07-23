#include "webserv/message/handler/ResponseHandler.hpp"

namespace ft {

ResponseHandler::ResponseHandler() {}

ResponseHandler::~ResponseHandler() {}

void ResponseHandler::setResponse(Response *response) { response_ = response; }

//TODO: setLocationConfig로 바꿔도 될지 확인해보기
//      일단 안하는게 맞는걸로 확인되는데 다시 확인 필요...
void ResponseHandler::setServerConfig(HttpConfig *http_config, struct sockaddr_in &addr, const std::string &host) {
  this->server_config_ = http_config->getServerConfig(addr.sin_port, addr.sin_addr.s_addr, host);
}

void ResponseHandler::setResponseFields(Request &request) {
  this->response_->setHeader("Date", Time::getCurrentDate());
  LocationConfig *location = this->server_config_->getLocationConfig(request.getUri());

  if (!location->checkAcceptedMethod(request.getMethod())) {
    setStatusLineWithCode("405");
    return;
  }

  if (request.getMethod() == "GET" || request.getMethod() == "HEAD")
    processGetAndHeaderMethod(request, location);
  else if (request.getMethod() == "PUT")
    processPutMethod(request.getUri(), location);
  else if (request.getMethod() == "POST")
    // 아무것도 없음
    processPostMethod(request, location);
  else if (request.getMethod() == "DELETE")
    processDeleteMethod(request.getUri(), location);

  if (this->response_->getResponseBody().size() > 0) {
    this->response_->setHeader("Content-Length",
                               std::to_string(this->response_->getResponseBody().size()));
  }
}

/*-----------------------MAKING RESPONSE MESSAGE-----------------------------*/

// 흐름상 가장 아래에 위치함
void ResponseHandler::makeResponseMsg() {
  setResponseStatusLine();
  setResponseHeader();
  setResponseBody();
}

// Response::response_ setter begin
void ResponseHandler::setResponseStatusLine() {
  response_->getMsg() += this->response_->getHttpVersion();
  response_->getMsg() += " ";
  response_->getMsg() += this->response_->getStatusCode();
  response_->getMsg() += " ";
  response_->getMsg() += this->response_->getStatusMessage();
  response_->getMsg() += "\r\n";
}

void ResponseHandler::setResponseHeader() {
  std::map<std::string, std::string> headers = response_->getHeaders();
  std::map<std::string, std::string>::reverse_iterator it;
  // nginx 는 알파벳 역순으로 메세지를 보냄
  for (it = headers.rbegin(); it != headers.rend(); it++) {
    if (it->second != "") {
      response_->getMsg() += it->first;
      response_->getMsg() += ": ";
      response_->getMsg() += it->second;
      response_->getMsg() += "\r\n";
    }
  }
  response_->getMsg() += "\r\n";
}
void ResponseHandler::setResponseBody() {
  if (response_->getResponseBody().size()) {
    response_->getMsg() += response_->getResponseBody();
  }
}

// Response::response_ setter end

// making response message begin
void ResponseHandler::setStatusLineWithCode(const std::string &status_code) {
  this->response_->setStatusCode(status_code);
  this->response_->setStatusMessage(StatusMessage::of(stoi(status_code)));
  this->response_->setConnectionHeaderByStatusCode(status_code);
  // MUST BE EXECUTED ONLY WHEN BODY IS NOT PROVIDED
  // TODO: fix this garbage conditional statement...
  if (!(!status_code.compare("200") ||
        !status_code.compare("201") ||
        !status_code.compare("204") ||
        !status_code.compare("999")))
    this->response_->getResponseBody() =
        getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
}

std::string ResponseHandler::getDefaultErrorBody(std::string status_code, std::string status_message) {
  //TODO: 리팩토링 필요..
  std::string body;

  body += "<html>\n";
  body += "<head><title>" + status_code + " " + status_message + "</title></head>\n";
  body += "<body>\n";
  body += "<center><h1>" + status_code + " " + status_message + "</h1></center>\n";
  body += "<hr><center>" + response_->getHeaderValue("Server") + "</center>\n";
  body += "</body>\n";
  body += "</html>\n";

  return (body);
}

// making response message end

/*-----------------------MAKING RESPONSE MESSAGE END-----------------------------*/

/*--------------------------EXECUTING METHODS BEGIN--------------------------------*/

// ***********blocks for setResponseFields begin*************** //

void ResponseHandler::processGetAndHeaderMethod(Request &request, LocationConfig *&location) {
  //need last modified header
  // TODO: apply for all url when directory is given
  if (*(request.getUri().rbegin()) == '/') {
    findIndexForGetWhenOnlySlash(request, location);
    if (!request.getUri().compare("/")) {
      setStatusLineWithCode("403");
      return;
    }
  }
  if (!isFileExist(request.getUri(), location)) {
    setStatusLineWithCode("404");
    return;
  } else {
    if (S_ISDIR(this->stat_buffer_.st_mode)) {
      setStatusLineWithCode("301");
      // TODO: string 을 생성 하지 않도록 수정하는 작업 필요
      std::string temp_url = "http://" + request.getHeaderValue("Host") + request.getUri() + "/";
      this->response_->setHeader("Location", temp_url);
      return;
    }
    setStatusLineWithCode("200");
    if (request.getMethod() == "GET")
      setResponseBodyFromFile(request.getUri(), location);
  }
}

void ResponseHandler::processPutMethod(std::string &uri, LocationConfig *&location) {
  if (!uri.compare("/")) {
    setStatusLineWithCode("409");
    return;
  } else if (!isPathAccessable(uri, location)) {
    setStatusLineWithCode("500");
    return;
  }
  if (!isFileExist(uri)) {
    setStatusLineWithCode("201");
  } else {
    setStatusLineWithCode("204");
  }
}

void ResponseHandler::processPostMethod(Request &request, LocationConfig *&location) {
  if (!location->checkCgiExtension(request.getUri()) ||
      location->getCgiPath().empty()) {
    setStatusLineWithCode("405");
    return;
  }

  // // assume query string is placed in request.entity_body_;

  // // init env_set map which will be transformned into char** later
  // char **environ;
  // char **command;
  // pid_t pid;
  // int pipe_fd[2];
  // char foo[4096];

  // std::string cgi_output_temp;
  // std::map<std::string, std::string> env_set;
  // {
  //   env_set["CONTENT_LENGTH"] = request.getHeaderValue("Content-Length");
  //   env_set["QUERY_STRING"] = request.getEntityBody();
  //   env_set["REQUEST_METHOD"] = request.getMethod();
  //   env_set["REDIRECT_STATUS"] = "CGI";
  //   env_set["SCRIPT_FILENAME"] = getAccessPath(request.getUri());
  //   env_set["SERVER_PROTOCOL"] = "HTTP/1.1";
  //   env_set["PATH_INFO"] = getAccessPath(request.getUri());
  //   env_set["CONTENT_TYPE"] = "test/file";
  //   env_set["GATEWAY_INTERFACE"] = "CGI/1.1";
  //   env_set["PATH_TRANSLATED"] = getAccessPath(request.getUri());
  //   env_set["REMOTE_ADDR"] = "127.0.0.1";
  //   env_set["REQUEST_URI"] = getAccessPath(request.getUri());
  //   env_set["SERVER_PORT"] = "80";
  //   env_set["SERVER_PROTOCOL"] = "HTTP/1.1";
  //   env_set["SERVER_SOFTWARE"] = "versbew";
  //   env_set["SCRIPT_NAMME"] = location->getCgiPath();
  // }
  // environ = setEnviron(env_set);
  // command = setCommand(location->getCgiPath(), getAccessPath(request.getUri()));

  // pipe(pipe_fd);
  // pid = fork();
  // if (!pid) {
  //   dup2(pipe_fd[1], STDOUT_FILENO);
  //   close(pipe_fd[0]);
  //   close(pipe_fd[1]);
  //   execve(location->getCgiPath().c_str(), command, environ);
  //   exit(1);
  // } else {
  //   close(pipe_fd[1]);
  //   int nbytes;
  //   int i = 0;
  //   while ((nbytes = read(pipe_fd[0], foo, sizeof(foo)))) {
  //     cgi_output_temp.append(foo);
  //     i++;
  //   }
  //   std::cout << "i: " << nbytes << std::endl;
  //   // write(STDOUT_FILENO, foo, strlen(foo));
  //   wait(NULL);
  // }

  // this->response_->setResponseBody(cgi_output_temp);
  setStatusLineWithCode("200");
}

void ResponseHandler::processDeleteMethod(std::string &uri, LocationConfig *&location) {
  (void)location;
  // TODO: 경로가 "/"로 시작하지 않는 경우에는 "./"를 붙이도록 수정
  // if isUriOnlyOrSlash -> delete everything in there and 403 forbidden
  // if path is directory -> 409 Conflict and do nothing
  // if file is missing -> 404 not found
  // if file is available -> 204 No Content and delete the file
  std::cout << "start process delete method : " << uri << std::endl;
  if (!uri.compare("/")) {  // URI 에 "/" 만 있는 경우
    std::string url = getAccessPath(uri);
    if (stat(url.c_str(), &this->stat_buffer_) < 0) {
      setStatusLineWithCode("405");
      return;
    } else {
      if (S_ISDIR(this->stat_buffer_.st_mode)) {
        DIR *dir_ptr;
        struct dirent *item;

        if (!(dir_ptr = opendir(url.c_str()))) {
          setStatusLineWithCode("403");  // Not Allowed
          return;
        }
        while ((item = readdir(dir_ptr))) {
          if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
            continue;
          std::string new_path(url);
          new_path += item->d_name;
          if (deletePathRecursive(new_path) == -1) {
            setStatusLineWithCode("403");
            return;
          }
        }
        setStatusLineWithCode("403");
      } else {
        if (remove(url.c_str()) != 0) {
          setStatusLineWithCode("403");
          return;
        }
        setStatusLineWithCode("204");
      }
    }
  } else {  // "/" 가 아닌 경우
    std::string url = getAccessPath(uri);
    if (stat(url.c_str(), &this->stat_buffer_) < 0) {
      setStatusLineWithCode("404");
    } else {
      // file or directory
      if (S_ISDIR(this->stat_buffer_.st_mode)) {
        setStatusLineWithCode("409");
      } else {  // is not directory == file ?!
        if (remove(url.c_str()) != 0) {
          setStatusLineWithCode("403");
          return;
        }
        setStatusLineWithCode("204");
      }
    }
  }
}

// ***********blocks for setResponseFields end*************** //

std::string ResponseHandler::getAccessPath(std::string &uri) {
  LocationConfig *location = this->server_config_->getLocationConfig(uri);
  std::string path;
  path = location->getRoot() + uri;
  return (path);
}

std::string ResponseHandler::getAccessPath(const std::string &uri, LocationConfig *&location) {
  std::string path;
  path = location->getRoot() + uri;
  return (path);
}

// bool ResponseHandler::isFileExist(std::string &uri) {
//   if (stat(getAccessPath(uri).c_str(), &this->stat_buffer_) < 0) {
//     std::cout << "this ain't work" << std::endl;
//     return (false);
//   }
//   return (true);
// }

bool ResponseHandler::isFileExist(const std::string &path) {
  std::cout << "path: " << path << std::endl;
  if (stat(path.c_str(), &this->stat_buffer_) < 0) {
    std::cout << "this doesn't work" << std::endl;
    return (false);
  }
  return (true);
}
bool ResponseHandler::isFileExist(const std::string &path, LocationConfig *&location) {
  if (stat(getAccessPath(path, location).c_str(), &this->stat_buffer_) < 0) {
    std::cout << "this doesn't work" << std::endl;
    return (false);
  }
  return (true);
}

bool ResponseHandler::isPathAccessable(std::string &uri, LocationConfig *&location) {
  if (stat(getAccessPath(uri, location).c_str(), &this->stat_buffer_) < 0) {
    return (false);
  }
  if (stat_buffer_.st_mode & S_IRWXU)
    return (true);
  return (false);
}

// 함수가 불리는 시점에서는 이미 파일은 존재함
void ResponseHandler::setResponseBodyFromFile(std::string &uri, LocationConfig *&location) {
  std::ifstream file(getAccessPath(uri, location).c_str());

  file.seekg(0, std::ios::end);
  this->response_->getResponseBody().reserve(file.tellg());
  file.seekg(0, std::ios::beg);

  this->response_->getResponseBody().assign((std::istreambuf_iterator<char>(file)),
                                            std::istreambuf_iterator<char>());
}

int ResponseHandler::deletePathRecursive(std::string &path) {
  // struct stat stat_buffer;

  if (stat(path.c_str(), &this->stat_buffer_) != 0) {
    return (-1);  // error
  }

  if (S_ISDIR(this->stat_buffer_.st_mode)) {
    DIR *dir_ptr;
    struct dirent *item;

    if (!(dir_ptr = opendir(path.c_str()))) {
      return (-1);
    }
    while ((item = readdir(dir_ptr))) {
      if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
        continue;
      std::string new_path(path);
      new_path += "/";
      new_path += item->d_name;
      if (deletePathRecursive(new_path) == -1) {
        return (-1);
      }
    }
    /*
    if (rmdir(path.c_str()) == -1) {
      return (-1);
    }
    */
    return (remove_directory(path));
  } else if (S_ISREG(this->stat_buffer_.st_mode)) {
    /*
    if (remove(path.c_str()) != 0) {
      return (-1);
    }
    */
    return (remove_file(path));
  }
  return (0);
}

void ResponseHandler::findIndexForGetWhenOnlySlash(Request &request, LocationConfig *&location) {
  std::vector<std::string>::const_iterator it_index;
  std::string temp;
  for (it_index = location->getIndex().begin(); it_index != location->getIndex().end(); it_index++) {
    temp = location->getRoot() + request.getUri() + *it_index;
    if (isFileExist(temp)) {
      request.getUri() += *it_index;
      break;
    }
    temp.clear();
  }
}

int ResponseHandler::remove_file(std::string file_name) {
  if (remove(file_name.c_str()) != 0) {
    std::cout << "fail remove " << file_name << std::endl;
    return (-1);
  }
  std::cout << "sucess remove file " << file_name << std::endl;
  return (0);
}

int ResponseHandler::remove_directory(std::string directory_name) {
  if (rmdir(directory_name.c_str()) != 0) {
    std::cout << "fail remove " << directory_name << std::endl;
    return (-1);
  }
  std::cout << "sucess remove file " << directory_name << std::endl;
  return (0);
}

char **ResponseHandler::setEnviron(std::map<std::string, std::string> env) {
  char **return_value;
  std::string temp;

  return_value = (char **)malloc(sizeof(char *) * (env.size() + 1));
  int i = 0;
  std::map<std::string, std::string>::iterator it;
  for (it = env.begin(); it != env.end(); it++) {
    temp = (*it).first + "=" + (*it).second;
    char *p = (char *)malloc(temp.size() + 1);
    strcpy(p, temp.c_str());
    return_value[i] = p;
    i++;
  }
  return_value[i] = NULL;
  return (return_value);
}

char **ResponseHandler::setCommand(std::string command, std::string path) {
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

/*--------------------------EXECUTING METHODS END--------------------------------*/
}  // namespace ft
