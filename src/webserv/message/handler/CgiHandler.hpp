#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <cstdlib>
#include <map>
#include <string>

#include "webserv/socket/Connection.hpp"
#include "webserv/message/handler/MessageHandler.hpp"

namespace ft {

class CgiHandler {
 private:
  static char **setEnviron(Connection *c);
  static char **setCommand(std::string command, std::string path);

 public:
  static void initCgiChild(Connection *c);
  static void handleCgiHeader(Connection *c);

  static void setupCgiMessage(Connection *c);
  static void receiveCgiBody(Connection *c);
};

}  // namespace ft
#endif
