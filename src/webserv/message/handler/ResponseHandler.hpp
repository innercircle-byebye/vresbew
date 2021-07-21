#ifndef RESPONSE_HANDLER_HPP
#define RESPONSE_HANDLER_HPP

#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>
#include <stdio.h>  // remove()
#include <unistd.h>  // rmdir(2)
#include <dirent.h>  // opendir()

#include "webserv/config/HttpConfig.hpp"
#include "webserv/logger/Time.hpp"
#include "webserv/message/Request.hpp"
#include "webserv/message/Response.hpp"
#include "webserv/message/StatusMessage.hpp"

namespace ft {

class ResponseHandler {
  Response *response_;
  ServerConfig *server_config_;

  struct stat stat_buffer_;
  // 함수 내 지역변수로 옮겼습니다. 향후 지워주세요!!
  // struct dirent *entry_; // TODO: 지역변수로 전환 검토
  // DIR *dir_; // TODO: 지역변수로 전환 검토

 public:
  ResponseHandler();
  ~ResponseHandler();

  void setResponse(Response *response);
  void setServerConfig(HttpConfig *http_config, struct sockaddr_in &addr, const std::string &host);
  void setResponseFields(const std::string &method, std::string &uri);
  void makeResponseMsg();

  void setResponse400();
  std::string getAccessPath(std::string uri);

 private:
  void setResponseStatusLine();
  void setResponseHeader();
  void setResponseBody();

  // static bool isUriOnlySlash(std::string &uri);
  bool isFileExist(std::string &uri);
  bool isPathAccessable(std::string path, std::string &uri);
  void setResponseBodyFromFile(std::string &uri);
  void setResponse200();
  void setResponse201();
  void setResponse204();
  void setResponse403();
  void setResponse404();
  void setResponse405();
  void setResponse409();
  void setResponse500();

  std::string getDefaultErrorBody(std::string status_code, std::string status_message);

  int deletePathRecurcive(std::string &path);

};
}  // namespace ft
#endif
