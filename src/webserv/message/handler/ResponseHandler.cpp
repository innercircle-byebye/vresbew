#include "webserv/message/handler/ResponseHandler.hpp"

namespace ft {

ResponseHandler::ResponseHandler(Request &request, Response &response,
                                 HttpConfig *&http_config)
    : request_(request),
      response_(response),
      http_config_(http_config) {
  this->error400 = "<html>\n<head><title>400 Bad Request</title></head>\n<body>\n<center><h1>400 Bad Request</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error404 = "<html>\n<head><title>404 Not Found</title></head>\n<body>\n<center><h1>404 Not Found</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error405 = "<html>\n<head><title>404 Method Not Allowed</title></head>\n<body>\n<center><h1>405 Method Not Allowed</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error409 = "<html>\n<head><title>409 Conflict</title></head>\n<body>\n<center><h1>409 Confilct</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error409 = "<html>\n<head><title>409 Conflict</title></head>\n<body>\n<center><h1>409 Confilct</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error500 = "<html>\n<head><title>500 Internal Server Error</title></head>\n<body>\n<center><h1>500 Internal Server Error</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
};

//TODO: setLocationConfig로 바꿔도 될지 확인해보기
void ResponseHandler::setServerConfig(struct sockaddr_in *addr) {
  this->server_config_ = this->http_config_->getServerConfig(addr->sin_port,
                                                             addr->sin_addr.s_addr, this->request_.headers["Host"]);
}

void ResponseHandler::setResponse() {
  this->response_.header_["Date"] = this->getCurrentDate();
  LocationConfig *location = this->server_config_->getLocationConfig(this->request_.uri);

  if (!this->isValidRequestMethod()) {
    setResponse400();
    return;
  }
  if (!this->isValidRequestVersion()) {
    setResponse400();
    return;
  }
  if (!this->isRequestMethodAllowed()) {
    setResponse405();
    return;
  }
  // TODO: 수정 필요
  // switch case 쓰려고 굳이 이렇게 까지 할 필요가 있을까...
  switch (getMethodByEnum(this->request_.method)) {
    //need last modified header
    case METHOD_GET:
    case METHOD_HEAD: {
      if (isUriOnlySlash()) {
        this->request_.uri += location->getIndex().at(0);
      }
      if (!isFileExist(getAccessPath(this->request_.uri))) {
      // 403 Forbidden 케이스도 있음
        setResponse404();
        break;
      } else {
        setResponse200();
        if (getMethodByEnum(this->request_.method) == METHOD_HEAD)
          break;
        else {
          setResponseBodyFromFile(getAccessPath(this->request_.uri));
          break;
        }
      }
    }
    case METHOD_PUT: {
      if (isUriOnlySlash()) {
        setResponse409();
        break;
      }
      if (!isPathAccessable(location->getRoot())) {
        std::cout << "here" << std::endl;
        setResponse500();
        break;
      }
      /// file writing not working yet!!!!!
      if (!isFileExist(getAccessPath(this->request_.uri))) {
        setResponse201();
      } else {
        setResponse204();
      }
      break;
    }
    case METHOD_POST:
    case METHOD_DELETE:
    {
      // if isUriOnlyOrSlash -> delete everything in there and 403 forbidden
      // if path is directory -> 409 Conflict and do nothing
      // if file is missing -> 404 not found
      // if file is available -> 204 No Content and delete the file

    }
    default:
      break;
  }

  if (this->response_.response_body_.size() > 0) {
    this->response_.header_["Content-Length"] = std::to_string(this->response_.response_body_.size());
  }
}

bool ResponseHandler::isValidRequestMethod() {
  if (!(this->request_.method == "GET" || this->request_.method == "POST" ||
        this->request_.method == "DELETE" || this->request_.method == "PUT" || this->request_.method == "HEAD"))
    return (false);
  return (true);
}

bool ResponseHandler::isValidRequestVersion() {
  if (this->request_.version == "HTTP/1.1") {
    if (this->request_.headers.count("Host"))
      return (true);
    return (false);
  } else if (this->request_.version == "HTTP/1.0") {
    if (!this->request_.headers.count("Host"))
      this->request_.headers["Host"] = "";
    return (true);
  }
  return (false);
}

bool ResponseHandler::isRequestMethodAllowed() {
  LocationConfig *location = this->server_config_->getLocationConfig(this->request_.uri);

  if (!location->checkAcceptedMethod(this->request_.method))
    return (false);
  return (true);
}

std::string ResponseHandler::getAccessPath(std::string uri) {
  LocationConfig *location = this->server_config_->getLocationConfig(this->request_.uri);

  std::string path;
  path = "." + location->getRoot() + uri;
  return (path);
}

bool ResponseHandler::isUriOnlySlash() {
  if (this->request_.uri == "/")
    return (true);
  return (false);
}

bool ResponseHandler::isFileExist(std::string host_path) {
  LocationConfig *location = this->server_config_->getLocationConfig(this->request_.uri);

  if (stat(host_path.c_str(), &this->stat_buffer_) < 0) {
    std::cout << "this ain't work" << std::endl;
    return (false);
  }
  return (true);
}

bool ResponseHandler::isPathAccessable(std::string path) {
  LocationConfig *location = this->server_config_->getLocationConfig(this->request_.uri);
  path.insert(0, ".");
  std::cout << path << std::endl;
  if (stat(path.c_str(), &this->stat_buffer_) < 0) {
    return (false);
  }
  std::cout << this->stat_buffer_.st_mode << std::endl;
  if (this->stat_buffer_.st_mode & S_IRWXU)
    return (true);
  return (false);
}

void ResponseHandler::setResponseBodyFromFile(std::string path) {
  LocationConfig *location = this->server_config_->getLocationConfig(this->request_.uri);  // 없으면 not found

  std::ifstream file(path.c_str());
  file.seekg(0, std::ios::end);
  this->response_.response_body_.reserve(file.tellg());
  file.seekg(0, std::ios::beg);

  this->response_.response_body_.assign((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
}

void ResponseHandler::setResponse400() {
  this->response_.status_code_ = "400";
  this->response_.status_message_ = "Bad Request";
  this->response_.response_body_ = this->error400;
  this->response_.header_["Connection"] = "close";
}
void ResponseHandler::setResponse404() {
  this->response_.status_code_ = "404";
  this->response_.status_message_ = "Not Found";
  this->response_.response_body_ = this->error404;
  this->response_.header_["Connection"] = "keep-alive";
}

void ResponseHandler::setResponse405() {
  this->response_.status_code_ = "405";
  this->response_.status_message_ = "Method Not Allowed";
  this->response_.response_body_ = this->error405;
  //확인안됨
  this->response_.header_["Connection"] = "keep-alive";
}

void ResponseHandler::setResponse409() {
  this->response_.status_code_ = "409";
  this->response_.status_message_ = "Conflict";
  this->response_.response_body_ = this->error409;
  //확인안됨
  this->response_.header_["Connection"] = "keep-alive";
}

void ResponseHandler::setResponse500() {
  this->response_.status_code_ = "500";
  this->response_.status_message_ = "Internal Server Error";
  this->response_.response_body_ = this->error500;
  this->response_.header_["Connection"] = "close";
}

void ResponseHandler::setResponse200() {
  this->response_.status_code_ = "200";
  this->response_.status_message_ = "OK";
  this->response_.header_["Connection"] = "keep-alive";
}

void ResponseHandler::setResponse201() {
  this->response_.status_code_ = "201";
  this->response_.status_message_ = "Created";
  this->response_.header_["Connection"] = "keep-alive";
}

void ResponseHandler::setResponse204() {
  this->response_.status_code_ = "204";
  this->response_.status_message_ = "No Content";
  this->response_.header_["Connection"] = "keep-alive";
}


std::string ResponseHandler::getCurrentDate() {
  //TODO: 개선이 필요함
  std::string current_time;
  time_t t;       // t passed as argument in function time()
  struct tm *tt;  // decalring variable for localtime()
  time(&t);       //passing argument to time()
  tt = gmtime(&t);
  current_time.append(asctime(tt), strlen(asctime(tt)) - 1);
  current_time.append(" GMT");

  return (current_time);
}

int ResponseHandler::getMethodByEnum(std::string request_method) {
  if (request_method == "GET")
    return (METHOD_GET);
  else if (request_method == "HEAD")
    return (METHOD_HEAD);
  else if (request_method == "PUT")
    return (METHOD_PUT);
  else if (request_method == "POST")
    return (METHOD_POST);
  else if (request_method == "DELETE")
    return (METHOD_DELETE);
  return (-1);
}

}  // namespace ft
