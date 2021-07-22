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

namespace ft {

class ResponseHandler {
  Response *response_;
  ServerConfig *server_config_;

  struct stat stat_buffer_;

 public:
  ResponseHandler();
  ~ResponseHandler();

  void setResponse(Response *response);
  void setServerConfig(HttpConfig *http_config, struct sockaddr_in &addr, const std::string &host);
  void setResponseFields(Request &request);
  void makeResponseMsg();
  void setStatusLineWithCode(const std::string &status_code);

  std::string getAccessPath(std::string &uri);

 private:
  // Response::response_ setter begin
  // 흐름상 가장 아래에 위치함
  void setResponseStatusLine();
  void setResponseHeader();
  void setResponseBody();
  // Response::response_ setter end

  // making response message begin
  std::string getDefaultErrorBody(std::string status_code, std::string status_message);
  // making response message end

  // 애매함
  void setResponseBodyFromFile(std::string &uri, LocationConfig *&location);
  // 애매함 end

  /*--------------------------EXECUTING METHODS--------------------------------*/

  // blocks for setResponseFields begin
  void processGetAndHeaderMethod(Request &request, LocationConfig *&location);
  void processPutMethod(std::string &uri, LocationConfig *&location);
  void processDeleteMethod(std::string &uri, LocationConfig *&location);
  void processPostMethod(Request &request, LocationConfig *&location);
  // blocks for setResponseFields end

  // executing methods helper begin
  bool isFileExist(std::string &uri);
  bool isFileExist(std::string &uri, LocationConfig *&location);
  std::string getAccessPath(std::string &uri, LocationConfig *&location);
  bool isPathAccessable(std::string &uri, LocationConfig *&location);
  int deletePathRecursive(std::string &path);

  void findIndexForGetWhenOnlySlash(Request &request, LocationConfig *&location);

  int remove_file(std::string file_name);
  int remove_directory(std::string directory_name);
  // executing methods helper end

  /*--------------------------EXECUTING METHODS END--------------------------------*/

  /*--------------------------FOR CGI--------------------------------*/
  char **setEnviron(std::map<std::string, std::string> env);
  char **setCommand(std::string command, std::string path);
  /*--------------------------FOR CGI--------------------------------*/
};
}  // namespace ft
#endif
