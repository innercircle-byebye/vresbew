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
#include "webserv/message/Request.hpp"
#include "webserv/message/Response.hpp"
#include "webserv/message/StatusMessage.hpp"
#include "webserv/socket/Connection.hpp"
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
  void executeMethod(Request *request);
  void setDefaultHeader(Request *request);
  void makeResponseMsg();
  void makeResponseHeader();
  void setResponseBody();
  void setStatusLineWithCode(int status_code);

  // making response message begin
  void setDefaultErrorBody();
  // making response message end

  std::string getAccessPath(const std::string &uri);
  std::string getAccessPath(const std::string &uri, LocationConfig *&location);

 private:
  void setAutoindexBody(const std::string &uri);
  bool isDirectory(const std::string &path);


  // Response::response_ setter begin
  // 흐름상 가장 아래에 위치함
  void setResponseStatusLine();
  void setResponseHeader();
  // void setResponseBody();
  // Response::response_ setter end

  // 애매함
  void setResponseBodyFromFile(const std::string &uri, LocationConfig *&location);
  // 애매함 end

  /*--------------------------EXECUTING METHODS--------------------------------*/

  // blocks for setResponseFields begin
  void processGetAndHeaderMethod(Request *request, LocationConfig *&location);
  void processPutMethod(Request *request, LocationConfig *&location);
  void processDeleteMethod(const std::string &uri, LocationConfig *&location);
  void processPostMethod(Request *request, LocationConfig *&location);
  // blocks for setResponseFields end

  // executing methods helper begin
  bool isFileExist(const std::string &path, LocationConfig *&location);
  bool isFileExist(const std::string &path);
  bool isPathAccessable(std::string &uri, LocationConfig *&location);
  int deletePathRecursive(std::string &path);

  void findIndexForGetWhenOnlySlash(Request *request, LocationConfig *&location);

  int remove_file(std::string file_name);
  int remove_directory(std::string directory_name);
  // executing methods helper end

  /*--------------------------EXECUTING METHODS END--------------------------------*/
};
}  // namespace ft
#endif
