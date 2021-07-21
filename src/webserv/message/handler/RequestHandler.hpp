#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/message/Request.hpp"

namespace ft {

#define START_LINE_DELIMITER ' '
#define HEADER_DELIMITER ':'

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
private:
  void checkMsgForHeader();
  void checkMsgForEntityBody();

  int parseStartLine();
  void parseHeaderLines();
  int parseHeaderLine(std::string &one_header_line);
  void parseEntityBody();

  static int getCountOfDelimiter(std::string const &str, char delimiter);
  static bool isValidHeaderKey(std::string const &key);
  static bool isValidStartLine(int item_ident, std::string const &item);


};
}  // namespace ft
#endif
