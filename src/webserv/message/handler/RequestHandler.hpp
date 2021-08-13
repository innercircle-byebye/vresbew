#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <sys/stat.h>

#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/socket/Connection.hpp"
#include "webserv/message/Request.hpp"

namespace ft {

#define SPACE ' '

#define PARSE_VALID_URI 1
#define PARSE_INVALID_URI 0

enum StartLineItem {
  RQ_METHOD,
  RQ_URI,
  RQ_VERSION
};

class RequestHandler {
 private:
  Request *request_;

 public:
  RequestHandler();
  ~RequestHandler();

  void setRequest(Request *request);

  void appendMsg(const char *buffer);
  void processByRecvPhase(Connection *c);
  static std::vector<std::string> splitByDelimiter(std::string const &str, char delimiter);

  void setupUriStruct(ServerConfig *server, LocationConfig *location);
  bool isHostHeaderExist(void);
  bool isUriFileExist(void);
  bool isUriDirectory(void);
  bool isAllowedMethod(LocationConfig *location);

  void applyReturnDirectiveStatusCode(Connection *c);
  bool handleChunked(Connection *c);

  void findIndexForGetWhenOnlySlash(LocationConfig *location);

 private:
  void checkMsgForStartLine(Connection *c);
  void checkMsgForHeader(Connection *c);

  void parseStartLine(Connection *c);
  int parseUri(std::string uri_str);
  void parseHeaderLines(Connection *c);
  int parseHeaderLine(std::string &one_header_line);
  void parseEntityBody();

  static bool isValidHeaderKey(std::string const &key);
  static bool isValidMethod(std::string const &method);

  int checkHttpVersionErrorCode(std::string const &http_version);
  void checkCgiRequest(Connection *c);

  bool isFileExist(const std::string &path);
};
}  // namespace ft
#endif
