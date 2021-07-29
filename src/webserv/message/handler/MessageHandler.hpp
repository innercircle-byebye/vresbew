#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

// #include <cstring>
#include <exception>
#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/message/handler/RequestHandler.hpp"
#include "webserv/message/handler/ResponseHandler.hpp"
#include "webserv/socket/Connection.hpp"

namespace ft {

class MessageHandler {
 private:
  MessageHandler();
  ~MessageHandler();

 public:
  static ResponseHandler response_handler_;
  static RequestHandler request_handler_;
  static void handle_request_header(Connection *c);
  static void check_cgi_request(Connection *c);
  static void check_body_status(Connection *c);
  static void handle_request_body(Connection *c);
  static void handle_response(Connection *c);

  static void set_response_header(Connection *c);
  static void set_response_body(Connection *c);
  static void send_response_to_client(Connection *c);

 private:
  static void executePutMethod(std::string path, std::string content);
  static bool isValidRequestMethod(const std::string &method);
  static bool isValidRequestVersion(const std::string &http_version, const std::map<std::string, std::string> &headers);
};
}  // namespace ft
#endif
