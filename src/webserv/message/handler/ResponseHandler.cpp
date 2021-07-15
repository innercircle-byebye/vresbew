#include "webserv/message/handler/ResponseHandler.hpp"

namespace ft {

// ResponseHandler::ResponseHandler(Request &request, Response &response,
//                                  HttpConfig *&http_config)
ResponseHandler::ResponseHandler(Request &request, Response &response)
    : request_(request),
      response_(response) {
  //   http_config_(http_config) {
  this->error400 = "<html>\n<head><title>400 Bad Request</title></head>\n<body>\n<center><h1>400 Bad Request</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error403 = "<html>\n<head><title>403 Forbidden</title></head>\n<body>\n<center><h1>403 Forbidden</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error404 = "<html>\n<head><title>404 Not Found</title></head>\n<body>\n<center><h1>404 Not Found</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error405 = "<html>\n<head><title>405 Method Not Allowed</title></head>\n<body>\n<center><h1>405 Method Not Allowed</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error409 = "<html>\n<head><title>409 Conflict</title></head>\n<body>\n<center><h1>409 Confilct</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error409 = "<html>\n<head><title>409 Conflict</title></head>\n<body>\n<center><h1>409 Confilct</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
  this->error500 = "<html>\n<head><title>500 Internal Server Error</title></head>\n<body>\n<center><h1>500 Internal Server Error</h1></center>\n<hr><center>vresbew</center>\n</body>\n</html>\n";
}

//TODO: setLocationConfig로 바꿔도 될지 확인해보기
void ResponseHandler::setServerConfig(HttpConfig *http_config, struct sockaddr_in *addr) {
  this->server_config_ = http_config->getServerConfig(addr->sin_port,
                                                      addr->sin_addr.s_addr, this->request_.headers["Host"]);
}

void ResponseHandler::setResponse() {
  this->response_.header_["Date"] = this->getCurrentDate();
  LocationConfig *location = this->server_config_->getLocationConfig(this->request_.uri);

  std::cout << "version: " << this->request_.version << std::endl;
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
      if (this->request_.headers["Content-Length"] == "") {
        //content-type에 대한 처리도 필요하지 않을까 합니다.
        setResponse500();
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
    case METHOD_DELETE: {
      std::cout << "in delete" << std::endl;
      if (isUriOnlySlash()) {  // URI 에 "/" 만 있는 경우
        std::string url = getAccessPath(this->request_.uri);
        // stat 으로 하위에 존재하는 모든것을 탐색해야합니다.
        std::cout << "uri : " << this->request_.uri << " url : " << url << std::endl;
        if ((dir = opendir(url.c_str())) == NULL) {
          std::cout << "Error opendir" << std::endl;
        } else {  // error handling 이 잘되면 else 안하고 쌉가능하다.
          std::cout << "directory open success" << std::endl;
          while ((entry = readdir(dir)) != NULL) {
            std::cout << "entry d_name : " << entry->d_name << std::endl;
            // 전부 지우자...
            /*
            if (remove(entry->d_name) != 0) {
              std::cout << "Error remove " << entry->d_name << std::endl;
              setResponse405();  // 실패하면 무슨 error 를 주어야하는거지?!
              break;  // while 꺠면 closedir() 만날듯?!
            }
            */
          }
          setResponse204();  // 여튼 성공!
        }
        closedir(dir);

        /*
        if (remove(url.c_str()) != 0)
          std::cout << "Error remove " << url << std::endl;
        else {
          std::cout << "remove success" << std::endl;
          setResponse403();
        }
        */
      }
      else {  // "/" 가 아닌 경우
        std::string url = getAccessPath(this->request_.uri);
        std::cout << "url : " << url << std::endl;
        if (stat(url.c_str(), &this->stat_buffer_) < 0) {
          std::cout <<  "stat ain't work" << std::endl;
          // if file is missing -> 404 not found
          setResponse404();
          break;
        }
        // file or directory
        if (S_ISDIR(this->stat_buffer_.st_mode)) {  // is directory
          // if path is directory -> 409 Conflict and do nothing
          std::cout << "is directory" << std::endl;
          setResponse409();
        } else {  // is not directory == file ?!
          std::cout << "is not directory" << std::endl;
          // file 이 존재합니다. 존재하지 않으면 stat() 이 -1 을 반환합니다.
          // file 을 지웁니다. remove()
          if (remove(url.c_str()) != 0)
            std::cout << "Error remove " << url << std::endl;
            // 궁금한 점 수정에 대한 접근 권한이 없으면 remove 는 실패할까?
          else
            std::cout << "remove success" << std::endl;
          setResponse204();
        }
      }
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
  if (stat(host_path.c_str(), &this->stat_buffer_) < 0) {
    std::cout << "this ain't work" << std::endl;
    return (false);
  }
  return (true);
}

bool ResponseHandler::isPathAccessable(std::string path) {
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

// kycho 님 이 부분도 추가해서 구현 부탁드립니다!
void ResponseHandler::setResponse403() {
  this->response_.status_code_ = "403";
  this->response_.status_message_ = "Forbidden";
  this->response_.response_body_ = this->error403;
  this->response_.header_["Connection"] = "keep-alive";
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
