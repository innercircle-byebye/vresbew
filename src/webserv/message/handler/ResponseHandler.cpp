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
      if (!(this->dir_ = opendir(url.c_str()))) {
        std::cout << "Error opendir" << std::endl;
        setResponse404();  // 404 Not Found
        return ;
      }
      // error handling 이 잘되면 else 안하고 쌉가능하다.
      std::cout << "directory open success" << std::endl;
      while ((this->entry_ = readdir(this->dir_)) != NULL) {
        std::cout << "entry d_name : " << entry_->d_name << std::endl;
        if (strcmp(entry_->d_name, ".") == 0) {
          std::cout << "find ." << std::endl;
          continue ;
        }
        else if (strcmp(entry_->d_name, "..") == 0) {
          std::cout << "find .." << std::endl;
          continue ;
        }
        else {
          std::string remove_target_path;

          remove_target_path += url;
          remove_target_path += entry_->d_name;
          std::cout << "path of remove target : " << remove_target_path << std::endl;
          // directory 인지 file 인지 확인하는 기능
          if (stat(remove_target_path.c_str(), &this->stat_buffer_) < 0) {
            std::cout << "stat ain't work" << std::endl;
            setResponse404();  // 404 Not Found
            return ;
          }
          if (S_ISDIR(this->stat_buffer_.st_mode)) {
            // 또 open dir 로 열어 제낀다음에 하위 목록들 싹다 지워야 directory 가 지워집니다.
            // TODO: directory 를 열고 하위 자료를 싹 지우는 제귀함수 구현하기
            if (rmdir(remove_target_path.c_str()) != 0) {
              // rmdir fail
              std::cout << "Error rmdir " << entry_->d_name << std::endl;
              setResponse403();
              return ;
            }
          } else if (S_ISREG(this->stat_buffer_.st_mode)) {
            if (remove(remove_target_path.c_str()) != 0) {
              // remove fail
              std::cout << "Error remove " << entry_->d_name << std::endl;
              setResponse403();
              return ;
            }
          }
        }
      }
      setResponse403();  // 여튼 성공!
      closedir(this->dir_);
    } else {  // "/" 가 아닌 경우
      std::string url = getAccessPath(uri);
      std::cout << "url : " << url << std::endl;
      if (stat(url.c_str(), &this->stat_buffer_) < 0) {
        std::cout << "stat ain't work" << std::endl;
        // if file is missing -> 404 not found
        setResponse404();
      } else {
        // file or directory
        if (S_ISDIR(this->stat_buffer_.st_mode)) {
          // is directory
          // if path is directory -> 409 Conflict and do nothing
          std::cout << "is directory" << std::endl;
          setResponse409();
        } else {  // is not directory == file ?!
          std::cout << "is not directory" << std::endl;
          // file 이 존재합니다. 존재하지 않으면 stat() 이 -1 을 반환합니다.
          // file 을 지웁니다. remove()
          if (remove(url.c_str()) != 0) {
            std::cout << "Error remove " << url << std::endl;
            setResponse403();
            return ;
          }
          std::cout << "remove success" << std::endl;
          setResponse204();
        }
      }
    }
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

void ResponseHandler::setResponse400() {
  unsigned int status_code = 400;
  std::string status_code_str = std::to_string(status_code);  // TODO : remove (c++11)

  this->response_->setStatusCode(status_code_str);
  this->response_->setStatusMessage(StatusMessage::of(status_code));
  std::string error_body = getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
  this->response_->setResponseBody(error_body);

  this->response_->setHeader("Connection", "close");
}

// kycho 님 이 부분도 추가해서 구현 부탁드립니다!
void ResponseHandler::setResponse403() {
  unsigned int status_code = 403;
  std::string status_code_str = std::to_string(status_code);  // TODO : remove (c++11)

  this->response_->setStatusCode(status_code_str);
  this->response_->setStatusMessage(StatusMessage::of(status_code));
  std::string error_body = getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
  this->response_->setResponseBody(error_body);

  this->response_->setHeader("Connection", "keep-alive");  // TODO: 커넥션 값 확인
}

void ResponseHandler::setResponse404() {
  unsigned int status_code = 404;
  std::string status_code_str = std::to_string(status_code);  // TODO : remove (c++11)

  this->response_->setStatusCode(status_code_str);
  this->response_->setStatusMessage(StatusMessage::of(status_code));
  std::string error_body = getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
  this->response_->setResponseBody(error_body);

  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse405() {
  unsigned int status_code = 405;
  std::string status_code_str = std::to_string(status_code);  // TODO : remove (c++11)

  this->response_->setStatusCode(status_code_str);
  this->response_->setStatusMessage(StatusMessage::of(status_code));
  std::string error_body = getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
  this->response_->setResponseBody(error_body);

  //확인안됨
  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse409() {
  unsigned int status_code = 409;
  std::string status_code_str = std::to_string(status_code);  // TODO : remove (c++11)

  this->response_->setStatusCode(status_code_str);
  this->response_->setStatusMessage(StatusMessage::of(status_code));
  std::string error_body = getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
  this->response_->setResponseBody(error_body);

  //확인안됨
  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse500() {
  unsigned int status_code = 500;
  std::string status_code_str = std::to_string(status_code);  // TODO : remove (c++11)

  this->response_->setStatusCode(status_code_str);
  this->response_->setStatusMessage(StatusMessage::of(status_code));
  std::string error_body = getDefaultErrorBody(this->response_->getStatusCode(), this->response_->getStatusMessage());
  this->response_->setResponseBody(error_body);

  this->response_->setHeader("Connection", "close");
}

void ResponseHandler::setResponse200() {
  unsigned int status_code = 200;
  std::string status_code_str = std::to_string(status_code);  // TODO : remove (c++11)

  this->response_->setStatusCode(status_code_str);
  this->response_->setStatusMessage(StatusMessage::of(status_code));
  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse201() {
  unsigned int status_code = 201;
  std::string status_code_str = std::to_string(status_code);  // TODO : remove (c++11)

  this->response_->setStatusCode(status_code_str);
  this->response_->setStatusMessage(StatusMessage::of(status_code));
  this->response_->setHeader("Connection", "keep-alive");
}

void ResponseHandler::setResponse204() {
  unsigned int status_code = 204;
  std::string status_code_str = std::to_string(status_code);  // TODO : remove (c++11)

  this->response_->setStatusCode(status_code_str);
  this->response_->setStatusMessage(StatusMessage::of(status_code));
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
