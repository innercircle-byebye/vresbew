#include "webserv/message/handler/ResponseHandler.hpp"

namespace ft {

ResponseHandler::ResponseHandler() {}

ResponseHandler::~ResponseHandler() {}

void ResponseHandler::setResponse(Response *response, std::string *body_buf) {
  response_ = response;
  body_buf_ = body_buf;
}

//TODO: setLocationConfig로 바꿔도 될지 확인해보기
//      일단 안하는게 맞는걸로 확인되는데 다시 확인 필요...
void ResponseHandler::setServerConfig(HttpConfig *http_config, struct sockaddr_in &addr, const std::string &host) {
  this->server_config_ = http_config->getServerConfig(addr.sin_port, addr.sin_addr.s_addr, host);
}

void ResponseHandler::executeMethod(Request &request) {
  LocationConfig *location = this->server_config_->getLocationConfig(request.getPath());

  if (request.getMethod() == "GET" || request.getMethod() == "HEAD")
    processGetAndHeaderMethod(request, location);
  else if (request.getMethod() == "PUT")
    processPutMethod(request, location);
  else if (request.getMethod() == "POST")
    processPostMethod(request, location);
  else if (request.getMethod() == "DELETE")
    processDeleteMethod(request.getPath(), location);
}

void ResponseHandler::setDefaultHeader(Connection *c, Request &request) {
  response_->setHeader("Content-Length",
                       std::to_string(this->body_buf_->size()));

  response_->setHeader("Date", Time::getCurrentDate());

  if (response_->getStatusCode() > 299) {  // TODO: 조건 다시 확인
    response_->setHeader("Content-Type", "text/html");
  } else {
    size_t extension_len;
    if ((extension_len = request.getPath().find('.')) != std::string::npos) {
      // TODO: string 안 만들고...
      std::string temp = request.getPath().substr(extension_len, request.getPath().size());
      response_->setHeader("Content-Type", MimeType::of(temp));
    }
  }
  if (response_->getStatusCode() == 201)
    createLocationHeaderFor201(c, request);
}
/*-----------------------MAKING RESPONSE MESSAGE-----------------------------*/

void ResponseHandler::makeResponseHeader() {
  setResponseStatusLine();
  setResponseHeader();
}

void ResponseHandler::setResponseStatusLine() {
  response_->getHeaderMsg() += this->response_->getHttpVersion();
  response_->getHeaderMsg() += " ";
  response_->getHeaderMsg() += SSTR(this->response_->getStatusCode());
  response_->getHeaderMsg() += " ";
  response_->getHeaderMsg() += this->response_->getStatusMessage();
  response_->getHeaderMsg() += "\r\n";
}

void ResponseHandler::setResponseHeader() {
  std::map<std::string, std::string> headers = response_->getHeaders();
  std::map<std::string, std::string>::reverse_iterator it;
  for (it = headers.rbegin(); it != headers.rend(); it++) {
    if (it->second != "") {
      response_->getHeaderMsg() += it->first;
      response_->getHeaderMsg() += ": ";
      response_->getHeaderMsg() += it->second;
      response_->getHeaderMsg() += "\r\n";
      // std::cout << "[" << it->first << "] [" << it->second << "]" << std::endl;
    }
  }
  response_->getHeaderMsg() += "\r\n";
}

void ResponseHandler::setStatusLineWithCode(int status_code) {
  response_->setStatusCode(status_code);
  if (response_->getStatusMessage().empty())
    response_->setStatusMessage(StatusMessage::of(status_code));
  response_->setConnectionHeaderByStatusCode(status_code);
}

void ResponseHandler::setDefaultErrorBody() {
  body_buf_->append("<html>\r\n");
  body_buf_->append("<head><title>" + SSTR(response_->getStatusCode()) + " " + StatusMessage::of(response_->getStatusCode()) + "</title></head>\r\n");
  body_buf_->append("<body>\r\n");
  body_buf_->append("<center><h1>" + SSTR(response_->getStatusCode()) + " " + StatusMessage::of(response_->getStatusCode()) + "</h1></center>\r\n");
  body_buf_->append("<hr><center>" + response_->getHeaderValue("Server") + "</center>\r\n");
  body_buf_->append("</body>\r\n");
  body_buf_->append("</html>\r\n");
}

void ResponseHandler::setAutoindexBody(const std::string &uri) {
  std::stringstream ss;
  std::string url = getAccessPath(uri);
  DIR *dir_ptr;
  struct dirent *item;

  ss << "<html>\r\n";
  ss << "<head><title>Index of " + uri + "</title></head>\r\n";
  ss << "<body>\r\n";
  ss << "<h1>Index of " + uri + "</h1><hr><pre>";
  ss << "<a href=\"../\">../</a>\r\n";
  if (!(dir_ptr = opendir(url.c_str()))) {
    // Logger::logError();
    return ;
  }
  while ((item = readdir(dir_ptr))) {
    if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
      continue ;
    std::string pathname = std::string(item->d_name);
    if (stat((url + pathname).c_str(), &this->stat_buffer_) < 0) {
      // Logger::logError();
      return ;
    }
    if (S_ISDIR(this->stat_buffer_.st_mode))
      pathname += "/";
    ss << "<a href=\"" + pathname + "\">" + pathname + "</a>";
    ss << std::setw(70 - pathname.size()) << Time::getFileModifiedTime(this->stat_buffer_.st_mtime);
    std::string filesize = (S_ISDIR(this->stat_buffer_.st_mode) ? "-" : SSTR(this->stat_buffer_.st_size));
    ss << std::right << std::setw(20) << filesize << "\r\n";
  }
  ss << "</pre><hr></body>\r\n";
  ss << "</html>\r\n";
  body_buf_->append(ss.str());
}
// making response message end

/*-----------------------MAKING RESPONSE MESSAGE END-----------------------------*/

/*--------------------------EXECUTING METHODS BEGIN--------------------------------*/

// ***********blocks for setResponseFields begin*************** //

void ResponseHandler::processGetAndHeaderMethod(Request &request, LocationConfig *&location) {
  //need last modified header
  if (stat(getAccessPath(request.getPath()).c_str(), &this->stat_buffer_) < 0) {
      // Logger::logError();
      return ;
  }
  if (location->getAutoindex() && S_ISDIR(this->stat_buffer_.st_mode)) {
    setStatusLineWithCode(200);
    setAutoindexBody(request.getPath());
    return ;
  }

  // TODO: connection의 status_code를 보고 결정하도록...
  if (this->response_->getHeaderValue("X-Powered-By") == "PHP/8.0.7" &&
      this->response_->getHeaderValue("Status").empty()) {
    setStatusLineWithCode(200);
    return;
  }

  // TODO: REQUEST에서 처리 해야될 수도 있을것같음
  if (*(request.getPath().rbegin()) == '/') {
    findIndexForGetWhenOnlySlash(request, location);
    if (*(request.getPath().rbegin()) == '/') {
      setStatusLineWithCode(403);
      return;
    }
  }
  if (!isFileExist(request.getPath(), location)) {
    setStatusLineWithCode(404);
    return;
  } else {
    if (S_ISDIR(this->stat_buffer_.st_mode)) {
      setStatusLineWithCode(301);
      // TODO: string 을 생성 하지 않도록 수정하는 작업 필요
      // std::string temp_url = "http://" + request.getHeaderValue("Host") + request.getUri();
      std::string temp_url = "http://" + request.getHeaderValue("Host") + ":" + request.getPort() + request.getPath();
      this->response_->setHeader("Location", temp_url);
      return;
    }
    setStatusLineWithCode(200);
    // body가 만들져 있지 않는 경우의 조건 추가
    if (body_buf_->empty())
      setResponseBodyFromFile(request.getPath(), location);
  }
}

void ResponseHandler::processPutMethod(Request &request, LocationConfig *&location) {
  if (*(request.getPath().rbegin()) == '/') {
    setStatusLineWithCode(409);
    return;
  }
  if (!isFileExist(request.getPath(), location)) {
    // 경로가 디렉토리 이거나, 경로에 파일을 쓸 수 없을때
    if (S_ISDIR(this->stat_buffer_.st_mode) || (this->stat_buffer_.st_mode & S_IRWXU)) {
      setStatusLineWithCode(500);
      return;
    }
    setStatusLineWithCode(201);
  } else {
    setStatusLineWithCode(204);
  }
}

void ResponseHandler::processPostMethod(Request &request, LocationConfig *&location) {
  if (this->response_->getStatusCode() == 302) {
    setStatusLineWithCode(this->response_->getStatusCode());
    return;
  }
  if (!location->checkCgiExtension(request.getPath()) ||
      location->getCgiPath().empty()) {
    setStatusLineWithCode(405);
    return;
  }
  setStatusLineWithCode(200);
}

void ResponseHandler::processDeleteMethod(const std::string &uri, LocationConfig *&location) {
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
      setStatusLineWithCode(405);
      return;
    } else {
      if (S_ISDIR(this->stat_buffer_.st_mode)) {
        DIR *dir_ptr;
        struct dirent *item;

        if (!(dir_ptr = opendir(url.c_str()))) {
          setStatusLineWithCode(403);  // Not Allowed
          return;
        }
        while ((item = readdir(dir_ptr))) {
          if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
            continue;
          std::string new_path(url);
          new_path += item->d_name;
          if (deletePathRecursive(new_path) == -1) {
            setStatusLineWithCode(403);
            return;
          }
        }
        setStatusLineWithCode(403);
      } else {
        if (remove(url.c_str()) != 0) {
          setStatusLineWithCode(403);
          return;
        }
        setStatusLineWithCode(204);
      }
    }
  } else {  // "/" 가 아닌 경우
    std::string url = getAccessPath(uri);
    if (stat(url.c_str(), &this->stat_buffer_) < 0) {
      setStatusLineWithCode(404);
    } else {
      // file or directory
      if (S_ISDIR(this->stat_buffer_.st_mode)) {
        setStatusLineWithCode(409);
      } else {  // is not directory == file ?!
        if (remove(url.c_str()) != 0) {
          setStatusLineWithCode(403);
          return;
        }
        setStatusLineWithCode(204);
      }
    }
  }
}

// ***********blocks for setResponseFields end*************** //

std::string ResponseHandler::getAccessPath(const std::string &uri) {
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

bool ResponseHandler::isFileExist(const std::string &path) {
  // std::cout << "REMINDER: THIS METHOD SHOULD ONLY BE \
  //   CALLED WHEN FILE PATH IS COMBINED WITH ROOT DIRECTIVE"
  //           << path << std::endl;
  // std::cout << "path: " << path << std::endl;
  if (stat(path.c_str(), &this->stat_buffer_) < 0) {
    // std::cout << "this aint work" << std::endl;
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
void ResponseHandler::setResponseBodyFromFile(const std::string &uri, LocationConfig *&location) {
  std::ifstream file(getAccessPath(uri, location).c_str());

  file.seekg(0, std::ios::end);
  body_buf_->reserve(file.tellg());
  file.seekg(0, std::ios::beg);

  body_buf_->assign((std::istreambuf_iterator<char>(file)),
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
    temp = location->getRoot() + request.getPath() + *it_index;
    if (isFileExist(temp)) {
      request.setPath(request.getPath() + *it_index);
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

void ResponseHandler::createLocationHeaderFor201(Connection *c, Request &request) {
  // TODO: 리팩토링..
  char str[INET_ADDRSTRLEN];

  inet_ntop(AF_INET, &(c->getSockaddrToConnect().sin_addr.s_addr), str, INET_ADDRSTRLEN);

  std::string full_uri;

  if (request.getSchema().empty())
    request.setSchema("http://");
  if (request.getHost().empty())
    request.setHost(((strcmp(str, "127.0.0.1") == 0) ? "localhost" : str));
  if (request.getPort().empty())
    request.setPort(SSTR(htons(c->getSockaddrToConnect().sin_port)));

  full_uri = request.getSchema() + request.getHost() + ":" + request.getPort() + request.getPath();
  response_->setHeader("Location", full_uri);
}

/*--------------------------EXECUTING METHODS END--------------------------------*/
}  // namespace ft
