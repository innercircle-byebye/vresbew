#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

// #include <cstring>
#include <exception>
#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/message/Request.hpp"
#include "webserv/message/Response.hpp"
#include "webserv/message/handler/RequestHandler.hpp"
#include "webserv/message/handler/ResponseHandler.hpp"

namespace ft {

enum MessageFromBufferStatus {
  MESSAGE_HEADER_INCOMPLETE = 0,
  MESSAGE_HEADER_COMPLETE,
  MESSAGE_BODY_NO_NEED,
  MESSAGE_BODY_INCOMING,
  MESSAGE_BODY_COMPLETE
};

class MessageHandler {
 private:
  Request &request_;
  Response &response_;
//   HttpConfig *http_config_;

  RequestHandler request_handler_;
  ResponseHandler response_handler_;

  int content_length_;

 public:
  // for test
  // Request &request_;
  int recv_phase;
  std::string msg_header_buf_;
  std::string msg_body_buf_;
  std::string *response_buf_;

  // 테스트용
  struct sockaddr_in *sockaddr_to_connect_;

  MessageHandler(Request &request, Response &response);
  ~MessageHandler();

  // setter
//   void setHttpConfig(HttpConfig *http_config);
  void setSockAddr(struct sockaddr_in *addr);
  void setContentLengthToZero();
  // setter done

  // for buffer handling
  void checkBufferForHeader(char *buffer);
  void checkRemainderBufferForHeader(char *buffer);
  void handleBody(char *buffer);
  std::string *createResponseHeaderBuf();
  // for buffer handling done

  // with  RequestHandler
  void createRequest();
  // with RequestHandler done

  // with ResponseHandler
  void createResponse(HttpConfig *http_config);
  void createResponseForResponseHandler();
  void clearBufferStrings(void);
  void clearMsgBodyBuf(void);
  void clearMsgHeaderBuf(void);
  void clearResponseHeaderBuf(void);
  std::string getResponseStatus();
  void HttpConfigPrintTest();
  // with ResponseHandler done
  std::string getRequestUri();

 private:
  void executePutMethod(std::string path);
};
}  // namespace ft
#endif
