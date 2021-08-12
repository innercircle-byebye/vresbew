#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <exception>
#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/socket/Connection.hpp"
#include "webserv/message/handler/ResponseHandler.hpp"
#include "webserv/message/handler/RequestHandler.hpp"
#include "webserv/message/handler/CgiHandler.hpp"

namespace ft {

class RequestHandler;
class ResponseHandler;

class MessageHandler {
 private:
  MessageHandler();
  ~MessageHandler();

 public:
  static ResponseHandler response_handler_;
  static RequestHandler request_handler_;

  static void handleRequestHeader(Connection *c);
  static void checkRequestHeader(Connection *c);
  static void handleRequestBody(Connection *c);
  static void handleChunkedBody(Connection *c);

  static void executeServerSide(Connection *c);
  static void checkCgiProcess(Connection *c);
  static void setResponseHeader(Connection *c);
  static void setResponseMessage(Connection *c);
  static void sendResponseToClient(Connection *c);

 private:
  static void executePutMethod(std::string path, std::string content);
};
}  // namespace ft
#endif
