#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

// #include <cstring>
#include <exception>
#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/socket/Connection.hpp"
#include "webserv/message/handler/RequestHandler.hpp"
#include "webserv/message/handler/ResponseHandler.hpp"

namespace ft {

class MessageHandler {
 public:
  MessageHandler();
  ~MessageHandler();

  static void handle_request(Connection *c);
  static void handle_response(Connection *c);

 private:
  static void executePutMethod(std::string path, std::string content);
  static bool isValidRequestMethod(const std::string &method);
  static bool isValidRequestVersion(const std::string &http_version, const std::map<std::string, std::string> &headers);
};
}  // namespace ft
#endif
