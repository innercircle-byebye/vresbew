#include "webserv/message/handler/ResponseHandler.hpp"

namespace ft {

ResponseHandler::ResponseHandler() {
  this->error400 = "<html>\n<head><title>400 Bad Request</title></head>\n<body>\n<center><h1>400 Bad Request</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error404 = "<html>\n<head><title>404 Not Found</title></head>\n<body>\n<center><h1>404 Not Found</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error405 = "<html>\n<head><title>404 Method Not Allowed</title></head>\n<body>\n<center><h1>405 Method Not Allowed</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error409 = "<html>\n<head><title>409 Conflict</title></head>\n<body>\n<center><h1>409 Confilct</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error409 = "<html>\n<head><title>409 Conflict</title></head>\n<body>\n<center><h1>409 Confilct</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error500 = "<html>\n<head><title>500 Internal Server Error</title></head>\n<body>\n<center><h1>500 Internal Server Error</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
}

ResponseHandler::~ResponseHandler() {}

void ResponseHandler::setResponse(Response *response) {
  response_ = response;
}

//TODO: setLocationConfig로 바꿔도 될지 확인해보기
void ResponseHandler::setServerConfig(HttpConfig *http_config, struct sockaddr_in &addr, const std::string &host) {
  this->server_config_ = http_config->getServerConfig(addr.sin_port, addr.sin_addr.s_addr, host);
}

void ResponseHandler::setResponseFields(const std::string &method, std::string &uri) {
  this->response_->setHeader("Date", Time::getCurrentDate());
  LocationConfig *location = this->server_config_->getLocationConfig(uri);

  // TODO: 수정 필요
  // switch case 쓰려고 굳이 이렇게 까지 할 필요가 있을까...
  switch (getMethodByEnum(method)) {
    //need last modified header
    case METHOD_GET:
    case METHOD_HEAD: {
      if (!uri.compare("/")) {
        uri += location->getIndex().at(0);
      }
      if (!isFileExist(uri)) {
      // 403 Forbidden 케이스도 있음
        setResponse404();
        break;
      } else {
        setResponse200();
        if (getMethodByEnum(method) == METHOD_HEAD)
          break;
        else {
          setResponseBodyFromFile(uri);
          break;
        }
      }
    }
    case METHOD_PUT: {
      if (!uri.compare("/")) {
        setResponse409();
        break;
      }
      if (!isPathAccessable(location->getRoot(), uri)) {
        std::cout << "here" << std::endl;
        setResponse500();
        break;
      }
      /// file writing not working yet!!!!!
      if (!isFileExist(uri)) {
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

  if (this->response_->getResponseBody().size() > 0) {
    this->response_->setHeader("Content-Length", std::to_string(this->response_->getResponseBody().size()));
  }
}

void ResponseHandler::makeResponseMsg() {
  setResponseStatusLine();
  setResponseHeader();
  setResponseBody();
}

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
  std::map<std::string, std::string>::iterator it;
  // 순서에 맞아야하는지 확인해야함
  for (it = headers.begin(); it != headers.end(); it++) {
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

bool ResponseHandler::isRequestMethodAllowed(const std::string &uri, const std::string &method) {
  LocationConfig *location = this->server_config_->getLocationConfig(uri);

  if (!location->checkAcceptedMethod(method))
    return (false);
  return (true);
}

std::string ResponseHandler::getAccessPath(std::string uri) {
  LocationConfig *location = this->server_config_->getLocationConfig(uri);

  std::string path;
  path = "." + location->getRoot() + uri;
  return (path);
}

// bool ResponseHandler::isUriOnlySlash(std::string &uri) {
//   if (!uri.compare("/"))
//     return (true);
//   return (false);
// }

bool ResponseHandler::isFileExist(std::string &uri) {
  // LocationConfig *location = this->server_config_->getLocationConfig(uri);

  if (stat(getAccessPath(uri).c_str(), &this->stat_buffer_) < 0) {
    std::cout << "this ain't work" << std::endl;
    return (false);
  }
  return (true);
}

bool ResponseHandler::isPathAccessable(std::string path, std::string &uri) {
  LocationConfig *location = this->server_config_->getLocationConfig(uri);
  (void)location;

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

void ResponseHandler::setResponseBodyFromFile(std::string &uri) {
  LocationConfig *location = this->server_config_->getLocationConfig(uri);  // 없으면 not found
  (void)location;

  std::ifstream file(getAccessPath(uri).c_str());
  file.seekg(0, std::ios::end);
  this->response_->getResponseBody().reserve(file.tellg());
  file.seekg(0, std::ios::beg);

  this->response_->getResponseBody().assign((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
}

void ResponseHandler::setResponse400() {
  this->response_->setStatusCode("400");
  this->response_->setStatusMessage("Bad Request");
  this->response_->setResponseBody(this->error400);
  this->response_->setHeader("Connection", "close");
}
void ResponseHandler::setResponse404() {
  this->response_->setStatusCode("404");
  this->response_->setStatusMessage("Not Found");
  this->response_->setResponseBody(this->error404);
  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse405() {
  this->response_->setStatusCode("405");
  this->response_->setStatusMessage("Method Not Allowed");
  this->response_->setResponseBody(this->error405);
  //확인안됨
  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse409() {
  this->response_->setStatusCode("409");
  this->response_->setStatusMessage("Conflict");
  this->response_->setResponseBody(this->error409);
  //확인안됨
  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse500() {
  this->response_->setStatusCode("500");
  this->response_->setStatusMessage("Internal Server Error");
  this->response_->setResponseBody(this->error500);
  this->response_->setHeader("Connection", "close");
}

void ResponseHandler::setResponse200() {
  this->response_->setStatusCode("200");
  this->response_->setStatusMessage("OK");
  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse201() {
  this->response_->setStatusCode("201");
  this->response_->setStatusMessage("Created");
  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse204() {
  this->response_->setStatusCode("204");
  this->response_->setStatusMessage("No Content");
  this->response_->setHeader("Connection", "keep-alive");
}


// std::string ResponseHandler::getCurrentDate() {
//   //TODO: 개선이 필요함
//   std::string current_time;
//   time_t t;       // t passed as argument in function time()
//   struct tm *tt;  // decalring variable for localtime()
//   time(&t);       //passing argument to time()
//   tt = gmtime(&t);
//   current_time.append(asctime(tt), strlen(asctime(tt)) - 1);
//   current_time.append(" GMT");

//   return (current_time);
// }

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
