#include "webserv/message/handler/ResponseHandler.hpp"

namespace ft {

ResponseHandler::ResponseHandler() {}

ResponseHandler::~ResponseHandler() {}

void ResponseHandler::setResponse(Response *response) { response_ = response; }

//TODO: setLocationConfig로 바꿔도 될지 확인해보기
void ResponseHandler::setServerConfig(HttpConfig *http_config, struct sockaddr_in &addr, const std::string &host) {
  this->server_config_ = http_config->getServerConfig(addr.sin_port, addr.sin_addr.s_addr, host);
}

void ResponseHandler::setResponseFields(const std::string &method, std::string &uri) {
  this->response_->setHeader("Date", Time::getCurrentDate());
  LocationConfig *location = this->server_config_->getLocationConfig(uri);

  if (!location->checkAcceptedMethod(method)) {
    setResponse405();
    return;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  // TODO: 수정 필요
  if (method == "GET" || method == "HEAD") {
    //need last modified header
    if (!uri.compare("/")) {
      uri += location->getIndex().at(0);
    }
    if (!isFileExist(uri)) {
      // 403 Forbidden 케이스도 있음
      setResponse404();
    } else {
      setResponse200();
      if (method == "GET")
        setResponseBodyFromFile(uri);
    }
  } else if (method == "PUT") {
    if (!uri.compare("/")) {
      setResponse409();
    } else if (!isPathAccessable(location->getRoot(), uri)) {
      std::cout << "here" << std::endl;
      setResponse500();
    } else if (!isFileExist(uri)) {  /// file writing not working yet!!!!!
      setResponse201();
    } else {
      setResponse204();
    }
  } else if (method == "POST") {
    // write code!!!!
  } else if (method == "DELETE") {
    // if isUriOnlyOrSlash -> delete everything in there and 403 forbidden
    // if path is directory -> 409 Conflict and do nothing
    // if file is missing -> 404 not found
    // if file is available -> 204 No Content and delete the file
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////
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
  struct stat stat_buffer;

  if (stat(getAccessPath(uri).c_str(), &stat_buffer) < 0) {
    std::cout << "this ain't work" << std::endl;
    return (false);
  }
  return (true);
}

bool ResponseHandler::isPathAccessable(std::string path, std::string &uri) {
  struct stat stat_buffer;
  LocationConfig *location = this->server_config_->getLocationConfig(uri);
  (void)location;

  path.insert(0, ".");
  std::cout << path << std::endl;
  if (stat(path.c_str(), &stat_buffer) < 0) {
    return (false);
  }
  std::cout << stat_buffer.st_mode << std::endl;
  if (stat_buffer.st_mode & S_IRWXU)
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

  std::string error_body = getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
  this->response_->setResponseBody(error_body);

  this->response_->setHeader("Connection", "close");
}
void ResponseHandler::setResponse404() {
  this->response_->setStatusCode("404");
  this->response_->setStatusMessage("Not Found");

  std::string error_body = getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
  this->response_->setResponseBody(error_body);

  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse405() {
  this->response_->setStatusCode("405");
  this->response_->setStatusMessage("Method Not Allowed");

  std::string error_body = getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
  this->response_->setResponseBody(error_body);

  //확인안됨
  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse409() {
  this->response_->setStatusCode("409");
  this->response_->setStatusMessage("Conflict");

  std::string error_body = getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
  this->response_->setResponseBody(error_body);

  //확인안됨
  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse500() {
  this->response_->setStatusCode("500");
  this->response_->setStatusMessage("Internal Server Error");

  std::string error_body = getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
  this->response_->setResponseBody(error_body);

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

std::string ResponseHandler::getDefaultErrorBody(std::string status_code, std::string status_message) {
  std::string body;

  body += "<html>\n";
  body += "<head><title>" + status_code + " " + status_message + "</title></head>\n";
  body += "<body>\n";
  body += "<center><h1>" + status_code + " " + status_message + "</h1></center>\n";
  body += "<hr><center>" + response_->getHeaderValue("Server") + "</center>\n";
  body += "</body>\n";
  body += "</html>\n";

  return body;
}

}  // namespace ft
