#include "webserv/message/handler/ResponseHandler.hpp"

namespace ft {

ResponseHandler::ResponseHandler() {
  memset(&this->stat_buffer_, 0, sizeof(this->stat_buffer_));
}

ResponseHandler::~ResponseHandler() {
  memset(&this->stat_buffer_, 0, sizeof(this->stat_buffer_));
}

void ResponseHandler::setResponse(Response *response, std::string *body_buf) {
  response_ = response;
  body_buf_ = body_buf;
}

void ResponseHandler::setLocationConfig(LocationConfig *location_config) {
  this->location_config_ = location_config;
}

void ResponseHandler::setServerNameHeader(void) {
  response_->setHeader("Server", location_config_->getProgramName());
}

void ResponseHandler::executeMethod(Request &request) {
  stat(request.getFilePath().c_str(), &this->stat_buffer_);

  if (request.getMethod() == "GET" || request.getMethod() == "HEAD")
    processGetAndHeaderMethod(request, location_config_);
  else if (request.getMethod() == "PUT")
    processPutMethod(request);
  else if (request.getMethod() == "POST")
    processPostMethod(request, location_config_);
  else if (request.getMethod() == "DELETE")
    processDeleteMethod(request);
}

void ResponseHandler::setDefaultHeader(Connection *c, Request &request) {
  if (response_->getHeaderValue("Content-Length").empty())
    response_->setHeader("Content-Length",
                         SSTR(this->body_buf_->size()));

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
  if (response_->getStatusCode() == 301)
    createLocationHeaderFor301(request);
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
      response_->getHeaderMsg() += CRLF;
      // std::cout << "[" << it->first << "] [" << it->second << "]" << std::endl;
    }
  }
  response_->getHeaderMsg() += CRLF;
}

void ResponseHandler::setStatusLineWithCode(int status_code) {
  response_->setStatusCode(status_code);
  if (response_->getStatusMessage().empty())
    response_->setStatusMessage(StatusMessage::of(status_code));
  response_->setConnectionHeaderByStatusCode(status_code);
}

void ResponseHandler::setErrorBody(const std::string &error_page_directory_path) {
  bool check_error_body = false;

  if (error_page_directory_path != "") {
    std::string error_page_path = error_page_directory_path + "/" + SSTR(response_->getStatusCode()) + ".html";
    struct stat stat_buff;

    if (stat(error_page_path.c_str(), &stat_buff) == 0 && S_ISREG(stat_buff.st_mode)) {
      check_error_body = true;
      std::ifstream file(error_page_path.c_str());
      file.seekg(0, std::ios::end);
      body_buf_->reserve(file.tellg());
      file.seekg(0, std::ios::beg);
      body_buf_->assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
  }

  if (check_error_body == false)
    setDefaultErrorBody();
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
    return;
  }
  while ((item = readdir(dir_ptr))) {
    if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
      continue;
    std::string pathname = std::string(item->d_name);
    if (stat((url + pathname).c_str(), &this->stat_buffer_) < 0) {
      // Logger::logError();
      return;
    }
    if (S_ISDIR(this->stat_buffer_.st_mode))
      pathname += "/";
    ss << "<a href=\"" + pathname + "\">" + pathname + "</a>";
    ss << std::setw(70 - pathname.size()) << Time::getFileModifiedTime(this->stat_buffer_.st_mtime);
    std::string filesize = (S_ISDIR(this->stat_buffer_.st_mode) ? "-" : SSTR(this->stat_buffer_.st_size));
    ss << std::right << std::setw(20) << filesize << CRLF;
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
  if (location->getAutoindex() && S_ISDIR(this->stat_buffer_.st_mode)) {
    std::cout << "ccc" << std::endl;

    setStatusLineWithCode(200);
    setAutoindexBody(request.getPath());
    return;
  }
  setStatusLineWithCode(200);
  if (body_buf_->empty())
    setResponseBodyFromFile(request.getFilePath());
}

void ResponseHandler::processPutMethod(Request &request) {
  if (*(request.getPath().rbegin()) == '/') {
    setStatusLineWithCode(409);
    return;
  }
  if (!isFileExist(request.getFilePath())) {
    setStatusLineWithCode(201);
  } else {
    setStatusLineWithCode(204);
  }
  std::ofstream output(request.getFilePath().c_str());
  output << *(this->body_buf_);
  output.close();
  this->body_buf_->clear();
}

void ResponseHandler::processPostMethod(Request &request, LocationConfig *&location) {
  if (this->response_->getStatusCode() == 302) {
    setStatusLineWithCode(this->response_->getStatusCode());
    return;
  }
  if (!location->checkCgiExtension(request.getFilePath())) {
    setStatusLineWithCode(405);
    return;
  }
  setStatusLineWithCode(200);
}

void ResponseHandler::processDeleteMethod(Request &request) {
  if (!request.getPath().compare("/")) {  // URI 에 "/" 만 있는 경우
    std::string path = request.getFilePath().substr(0, request.getFilePath().find_last_of("/") + 1);
    if (stat(path.c_str(), &this->stat_buffer_) < 0) {
      setStatusLineWithCode(405);
      return;
    } else {
      if (S_ISDIR(this->stat_buffer_.st_mode)) {
        DIR *dir_ptr;
        struct dirent *item;

        if (!(dir_ptr = opendir(path.c_str()))) {
          setStatusLineWithCode(403);  // Not Allowed
          return;
        }
        while ((item = readdir(dir_ptr))) {
          if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
            continue;
          std::string new_path(path);
          new_path += item->d_name;
          if (deletePathRecursive(new_path) == -1) {
            setStatusLineWithCode(403);
            return;
          }
        }
        setStatusLineWithCode(403);
      } else {
        if (remove(path.c_str()) != 0) {
          setStatusLineWithCode(403);
          return;
        }
        setStatusLineWithCode(204);
      }
    }
  } else {  // "/" 가 아닌 경우
    std::string path = request.getFilePath();
    if (stat(path.c_str(), &this->stat_buffer_) < 0) {
      setStatusLineWithCode(404);
    } else {
      // file or directory
      if (S_ISDIR(this->stat_buffer_.st_mode)) {
        if (deletePathRecursive(path) == -1) {
          setStatusLineWithCode(403);
          return;
        }
      } else {  // is not directory == file ?!
        if (remove(path.c_str()) != 0) {
          setStatusLineWithCode(403);
          return;
        }
      }
      setStatusLineWithCode(204);
    }
  }
}

// ***********blocks for setResponseFields end*************** //

bool ResponseHandler::isFileExist(const std::string &path) {
  if (stat(path.c_str(), &this->stat_buffer_) < 0) {
    return (false);
  }
  return (true);
}

// 함수가 불리는 시점에서는 이미 파일은 존재함
void ResponseHandler::setResponseBodyFromFile(const std::string &filepath) {
  std::ifstream file(filepath.c_str());

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

    return (removeDirectory(path));
  } else if (S_ISREG(this->stat_buffer_.st_mode)) {
    return (removeFile(path));
  }
  return (0);
}

int ResponseHandler::removeFile(std::string file_name) {
  if (remove(file_name.c_str()) != 0) {
    return (-1);
  }
  return (0);
}

int ResponseHandler::removeDirectory(std::string directory_name) {
  if (rmdir(directory_name.c_str()) != 0) {
    return (-1);
  }
  return (0);
}

void ResponseHandler::createLocationHeaderFor201(Connection *c, Request &request) {
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

void ResponseHandler::createLocationHeaderFor301(Request &request) {
  std::string temp_url = "http://" + request.getHeaderValue("Host") + request.getPath();
  if (*(temp_url.rbegin()) != '/')
    temp_url.append("/");
  this->response_->setHeader("Location", temp_url);
}

/*--------------------------EXECUTING METHODS END--------------------------------*/
}  // namespace ft
