#include "webserv/message/handler/ResponseHandler.hpp"

namespace ft {

ResponseHandler::ResponseHandler() {}

ResponseHandler::~ResponseHandler() {}

void ResponseHandler::setResponse(Response *response) { response_ = response; }

//TODO: setLocationConfig로 바꿔도 될지 확인해보기
//      일단 안하는게 맞는걸로 확인함.
void ResponseHandler::setServerConfig(HttpConfig *http_config, struct sockaddr_in &addr, const std::string &host) {
  this->server_config_ = http_config->getServerConfig(addr.sin_port, addr.sin_addr.s_addr, host);
}

void ResponseHandler::setResponseFields(const std::string &method, std::string &uri) {
  this->response_->setHeader("Date", Time::getCurrentDate());
  LocationConfig *location = this->server_config_->getLocationConfig(uri);

  if (!location->checkAcceptedMethod(method)) {
    setStatusLineWithCode("405");
    return;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  // TODO: 수정 필요
  if (method == "GET" || method == "HEAD") {
    processGetAndHeaderMethod(method, uri, location);
  } else if (method == "PUT") {
    processPutMethod(uri, location);
  } else if (method == "POST") {
  } else if (method == "DELETE") {
    processDeleteMethod(uri, location);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  if (this->response_->getResponseBody().size() > 0) {
    this->response_->setHeader("Content-Length",
                               std::to_string(this->response_->getResponseBody().size()));
  }
}

/*-----------------------MAKING RESPONSE MESSAGE-----------------------------*/

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
  std::map<std::string, std::string>::iterator it;
  // nginx 는 알파벳 역순으로 저장함
  for (it = headers.end(); it != headers.begin(); it--) {
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
// Response::response_ setter begin

// making response message begin
void ResponseHandler::setStatusLineWithCode(const std::string &status_code) {
  this->response_->setStatusCode(status_code);
  this->response_->setStatusMessage(StatusMessage::of(stoi(status_code)));
  this->response_->setConnectionHeaderByStatusCode(status_code);
  // MUST BE EXECUTED ONLY WHEN BODY IS NOT PROVIDED
  // TODO: fix this garbage conditional statement...
  if (!(!status_code.compare("200") ||
        !status_code.compare("201") ||
        !status_code.compare("204")))
    this->response_->getResponseBody() =
        getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
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

// making response message end

/*-----------------------MAKING RESPONSE MESSAGE-----------------------------*/

/*--------------------------EXECUTING METHODS--------------------------------*/

// blocks for setResponseFields begin

void ResponseHandler::processGetAndHeaderMethod(const std::string &method,
                                                std::string &uri, LocationConfig *&location) {
  //need last modified header
  if (!uri.compare("/")) {
    uri += location->getIndex().at(0);
  }
  if (!isFileExist(uri)) {
    // 403 Forbidden 케이스도 있음
    setStatusLineWithCode("404");
  } else {
    setStatusLineWithCode("200");
    if (method == "GET")
      setResponseBodyFromFile(uri);
  }
}

void ResponseHandler::processPutMethod(std::string &uri, LocationConfig *&location) {
  if (!uri.compare("/")) {
    setStatusLineWithCode("409");
  } else if (!isPathAccessable(location->getRoot(), uri)) {
    std::cout << "here" << std::endl;
    setStatusLineWithCode("500");
  } else if (!isFileExist(uri)) {
    setStatusLineWithCode("201");
  } else {
    setStatusLineWithCode("204");
  }
}

void ResponseHandler::processDeleteMethod(std::string &uri, LocationConfig *&location) {
  // TODO: 경로가 "/"로 시작하지 않는 경우에는 "./"를 붙이도록 수정
  // if isUriOnlyOrSlash -> delete everything in there and 403 forbidden
  // if path is directory -> 409 Conflict and do nothing
  // if file is missing -> 404 not found
  // if file is available -> 204 No Content and delete the file
  std::cout << "in delete" << std::endl;
  if (!uri.compare("/")) {  // URI 에 "/" 만 있는 경우
    std::string url = getAccessPath(uri);
    // stat 으로 하위에 존재하는 모든것을 탐색해야합니다.
    std::cout << "uri : " << uri << " url : " << url << std::endl;
    if (deletePathRecursive(url) == -1) {
      setStatusLineWithCode("403");  // TODO:실패인데 어떤 status code 를 주어야하는지 모르겠습니다...
                                     // A: 201 혹은 204로 추정됩니다.
    } else {
      setStatusLineWithCode("403");
    }
  } else {  // "/" 가 아닌 경우
    std::string url = getAccessPath(uri);
    std::cout << "url : " << url << std::endl;
    if (stat(url.c_str(), &this->stat_buffer_) < 0) {
      std::cout << "stat ain't work" << std::endl;
      setStatusLineWithCode("404");
    } else {
      // file or directory
      if (S_ISDIR(this->stat_buffer_.st_mode)) {
        // is directory
        // if path is directory -> 409 Conflict and do nothing
        std::cout << "is directory" << std::endl;
        setStatusLineWithCode("409");
      } else {  // is not directory == file ?!
        std::cout << "is not directory" << std::endl;
        // file 이 존재합니다. 존재하지 않으면 stat() 이 -1 을 반환합니다.
        // file 을 지웁니다. remove()
        if (remove(url.c_str()) != 0) {
          std::cout << "Error remove " << url << std::endl;
          setStatusLineWithCode("403");
          return;
        }
        std::cout << "remove success" << std::endl;
        setStatusLineWithCode("204");
      }
    }
  }
}

std::string ResponseHandler::getAccessPath(std::string uri) {
  LocationConfig *location = this->server_config_->getLocationConfig(uri);

  std::string path;
  path = "." + location->getRoot() + uri;
  return (path);
}

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
  std::cout << stat_buffer_.st_mode << std::endl;
  if (stat_buffer_.st_mode & S_IRWXU)
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

int ResponseHandler::deletePathRecursive(std::string &path) {
  // stat 동작시키고
  if (stat(path.c_str(), &stat_buffer_) != 0) {
    // std::cerr << "fail stat(<File>)" << std::endl;
    return (-1);  // error
  }

  if (S_ISDIR(stat_buffer_.st_mode)) {
    DIR *dir_ptr;
    struct dirent *item;

    // std::cout << path << " is directory" << std::endl;
    if (!(dir_ptr = opendir(path.c_str()))) {
      // std::cerr << "fail opendir(<FILE>) " << path << std::endl;
      return (-1);
    }
    while ((item = readdir(dir_ptr))) {
      if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
        continue;
      std::string new_path(path);
      new_path += "/";
      new_path += item->d_name;
      // std::cout << "Good?! : " << new_path << std::endl;
      if (deletePathRecursive(new_path) == -1) {
        // std::cout << "Error!!! " << new_path << std::endl;
        return (-1);
      }
    }
    // std::cout << "finish inner search " << path << std::endl;
    if (rmdir(path.c_str()) == -1) {
      // std::cerr << "fail rmdir(<DIR>) " << path << std::endl;
      return (-1);
    }
    // std::cout << "success rmdir " << path << std::endl;
  } else if (S_ISREG(stat_buffer_.st_mode)) {
    // std::cout << path << " is file" << std::endl;
    // remove(path.c_str());
    if (remove(path.c_str()) != 0) {
      // std::cerr << "fail remove(<FILE>) " << path << std::endl;
      return (-1);
    }
    // std::cout << "success remove " << path << std::endl;
  }
  return (0);
}

/*--------------------------EXECUTING METHODS END--------------------------------*/
}  // namespace ft
