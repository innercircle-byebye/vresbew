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
#include "webserv/logger/Time.hpp"

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
  Response      *response_;
  ServerConfig  *server_config_;
  std::string rootpath_;
  std::string error400;
  std::string error404;
  std::string error405;
  std::string error409;
  std::string error500;
  struct stat stat_buffer_;

 public:
  ResponseHandler();
  ~ResponseHandler();

  void setResponse(Response *response);
  void setServerConfig(HttpConfig *http_config, struct sockaddr_in &addr, const std::string &host);
  void setResponseFields(const std::string &method, std::string &uri, const std::string &version);
  void setResponseMsg();

  void setResponse400();
  std::string getAccessPath(std::string uri);

 private:
  void setResponseStatusLine();
  void setResponseHeader();
  void setResponseBody();

  bool isRequestMethodAllowed(const std::string &uri, const std::string &method);
  int getMethodByEnum(std::string request_method);
  // static bool isUriOnlySlash(std::string &uri);
  bool isFileExist(std::string &uri);
  bool isPathAccessable(std::string path, std::string &uri);
  void setResponseBodyFromFile(std::string &uri);
  void setResponse200();
  void setResponse201();
  void setResponse204();
  void setResponse404();
  void setResponse405();
  void setResponse409();
  void setResponse500();
  // std::string getCurrentDate();
};
}  // namespace ft
#endif
