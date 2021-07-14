#ifndef RESPONSE_HANDLER_HPP
#define RESPONSE_HANDLER_HPP

#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/message/Request.hpp"
#include "webserv/message/Response.hpp"

namespace ft {

// class LocationConfig;

enum requestMethodForEnum {
  METHOD_GET = 0,
  METHOD_HEAD,
  METHOD_PUT,
  METHOD_POST,
  METHOD_DELETE
};

class ResponseHandler {
  HttpConfig *http_config_;
  ServerConfig *server_config_;
  Response &response_;
  Request &request_;
  std::string rootpath_;
  std::string error400;
  std::string error404;
  std::string error405;
  std::string error409;
  std::string error500;
  struct stat stat_buffer_;

 public:
  ResponseHandler(Request &request, Response &response,
                  HttpConfig *&http_config);

  void setServerConfig(struct sockaddr_in *addr);
  void setResponse();
  std::string getAccessPath(std::string uri);

 private:
  int getMethodByEnum(std::string request_method);
  bool isValidRequestMethod();
  bool isValidRequestVersion();
  bool isRequestMethodAllowed();
  bool isUriOnlySlash();
  bool isFileExist(std::string host_path);
  bool isPathAccessable(std::string path);
  void setResponseBodyFromFile(std::string filepath);
  void setResponse200();
  void setResponse201();
  void setResponse204();
  void setResponse400();
  void setResponse404();
  void setResponse405();
  void setResponse409();
  void setResponse500();
  std::string getCurrentDate();
  ;
};
}  // namespace ft
#endif
