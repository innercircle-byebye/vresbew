#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/message/Request.hpp"

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
  void processByRecvPhase();
  static std::vector<std::string> splitByDelimiter(std::string const &str, char delimiter);

 private:
  void checkMsgForStartLine();
  void checkMsgForHeader();
  void appendMsgToEntityBody();

  void parseStartLine();
  int parseUri(std::string uri_str);
  void parseHeaderLines();
  int parseHeaderLine(std::string &one_header_line);
  void parseEntityBody();
  // static std::vector<std::string> splitByDelimiter(std::string const &str, char delimiter);
  static bool isValidHeaderKey(std::string const &key);
  static bool isValidMethod(std::string const &method);
  static bool isValidHttpVersion(std::string const &http_version);
};
}  // namespace ft
#endif
