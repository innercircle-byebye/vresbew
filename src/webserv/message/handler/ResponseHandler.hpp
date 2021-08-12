#ifndef RESPONSE_HANDLER_HPP
#define RESPONSE_HANDLER_HPP

#include <dirent.h>  // opendir()
#include <stdio.h>   // remove()
#include <sys/stat.h>
#include <unistd.h>  // rmdir(2)

#include <fstream>
#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/logger/Time.hpp"
#include "webserv/socket/Connection.hpp"
#include "webserv/message/Request.hpp"
#include "webserv/message/Response.hpp"
#include "webserv/message/StatusMessage.hpp"
#include "webserv/message/MimeType.hpp"

namespace ft {

class ResponseHandler {
  Response *response_;
  ServerConfig *server_config_;
  std::string *body_buf_;

  struct stat stat_buffer_;

 public:
  ResponseHandler();
  ~ResponseHandler();

  void setResponse(Response *response, std::string *msg_body_buf);
  void setServerConfig(HttpConfig *http_config, struct sockaddr_in &addr, const std::string &host);
  void executeMethod(Request &request);
  void setDefaultHeader(Connection *c, Request &request);
  void makeResponseMsg();
  void makeResponseHeader();
  void setResponseBody();
  void setStatusLineWithCode(int status_code);

  void setErrorBody(const std::string &error_page_directory_path);

  std::string getAccessPath(const std::string &uri);

 private:
  void setAutoindexBody(const std::string &uri);
  void setResponseStatusLine();
  void setResponseHeader();
  void setResponseBodyFromFile(const std::string &filepath);
  void setDefaultErrorBody();

  /*--------------------------EXECUTING METHODS--------------------------------*/

  // blocks for setResponseFields begin
  void processGetAndHeaderMethod(Request &request, LocationConfig *&location);
  void processPutMethod(Request &request);
  void processDeleteMethod(const std::string &uri);
  void processPostMethod(Request &request, LocationConfig *&location);
  // blocks for setResponseFields end

  // executing methods helper begin
  bool isFileExist(const std::string &path);
  int deletePathRecursive(std::string &path);
  int removeFile(std::string file_name);
  int removeDirectory(std::string directory_name);
  // executing methods helper end

  void createLocationHeaderFor201(Connection *c, Request &request);
  void createLocationHeaderFor301(Request &request);
  /*--------------------------EXECUTING METHODS END--------------------------------*/
};
}  // namespace ft
#endif
