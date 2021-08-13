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

class ResponseHandler;
class RequestHandler;

class MessageHandler {
 private:
  MessageHandler();
  ~MessageHandler();

 public:
  static ResponseHandler response_handler_;
  static RequestHandler request_handler_;

  static void handleRequestHeader(Connection *c);
  static void handleRequestBody(Connection *c);
  static bool handleChunked(Connection *c);

  static void checkCgiProcess(Connection *c);

  static void executeServerSide(Connection *c);
  static void setResponseMessage(Connection *c);
  static bool sendResponseToClient(Connection *c);

};
}  // namespace ft
#endif
