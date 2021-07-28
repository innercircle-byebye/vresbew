#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/message/Request.hpp"
#include "webserv/socket/Connection.hpp"

namespace ft {

#define SPACE ' '
// #define START_LINE_DELIMITER ' '
// #define HEADER_DELIMITER ':'

#define PARSE_VALID_URI 1
#define PARSE_INVALID_URI 0

enum StartLineItem {
  RQ_METHOD,
  RQ_URI,
  RQ_VERSION
};

// class HttpConfig;

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

 private:
  void checkMsgForStartLine(Connection *c);
  void checkMsgForHeader(Connection *c);
  void appendMsgToEntityBody();

  void parseStartLine(Connection *c);
  int parseUri(std::string uri_str);
  void parseHeaderLines(Connection *c);
  int parseHeaderLine(std::string &one_header_line);
  void parseEntityBody();
  // static std::vector<std::string> splitByDelimiter(std::string const &str, char delimiter);
  static bool isValidHeaderKey(std::string const &key);
  static bool isValidMethod(std::string const &method);
  static bool isValidHttpVersion(std::string const &http_version);
  void checkCgiRequest(Connection *c);
};
}  // namespace ft
#endif
