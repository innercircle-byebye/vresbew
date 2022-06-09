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
#include "webserv/message/MimeType.hpp"
#include "webserv/message/Request.hpp"
#include "webserv/message/Response.hpp"
#include "webserv/message/StatusMessage.hpp"
#include "webserv/socket/Connection.hpp"

namespace ft {

class ResponseHandler {
  Response *response_;
  LocationConfig *location_config_;
  std::string *body_buf_;

  struct stat stat_buffer_;

 public:
  ResponseHandler();
  ~ResponseHandler();

  void setResponse(Response *response, std::string *msg_body_buf);
  void setLocationConfig(LocationConfig *location_config);
  void setServerNameHeader(void);
  void setDefaultHeader(Connection *c, Request &request);
  void executeMethod(Request &request);

  void makeResponseHeader();
  void setStatusLineWithCode(int status_code);

  void setErrorBody(const std::string &error_page_directory_path);

 private:
  void setResponseStatusLine();
  void setResponseHeader();
  void setDefaultErrorBody();

  /*--------------------------EXECUTING METHODS--------------------------------*/

  // blocks for setResponseFields begin
  void processGetAndHeaderMethod(Request &request, LocationConfig *&location);
  void processPutMethod(Request &request);
  void processDeleteMethod(Request &request);
  void processPostMethod(Request &request, LocationConfig *&location);

  void setAutoindexBody(const std::string &uri, const std::string &filepath);
  void setResponseBodyFromFile(const std::string &filepath);
  // blocks for setResponseFields end

  // executing methods helper begin
  bool isFileExist(const std::string &path);
  int deletePathRecursive(std::string &path);
  static int removeFile(std::string file_name);
  static int removeDirectory(std::string directory_name);
  // executing methods helper end

  void createLocationHeaderFor201(Connection *c, Request &request);
  void createLocationHeaderFor301(Request &request);

  std::string to_string(int x) {
    int length = snprintf(NULL, 0, "%d", x);
    assert(length >= 0);
    char *buf = new char[length + 1];
    snprintf(buf, length + 1, "%d", x);
    std::string str(buf);
    delete[] buf;
    return str;
  }

  /*--------------------------EXECUTING METHODS END--------------------------------*/
};
}  // namespace ft
#endif
