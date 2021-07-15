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
 private:
  RequestHandler  request_handler_;
  ResponseHandler response_handler_;

 public:
  MessageHandler();
  ~MessageHandler();

  void handle_request(Connection *c);
  void handle_response(Connection *c);

 private:
  static void executePutMethod(std::string path, std::string content);
};
}  // namespace ft
#endif
