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
  Request &request_;

  int delimiter_count_;

 public:
  RequestHandler(Request &request);
  void setRequest(std::string msg_header_buffer);
  int parseStartLine(std::string const &start_line);
  int parseHeaderLine(std::string const &header_line);
  bool isValidHeaderKey(std::string const &key);
  int getCountOfDelimiter(std::string const &str, char delimiter) const;
  void clearRequest();
  bool isValidStartLine(int item_ident,
                        std::string const &item);
};
}  // namespace ft
#endif
