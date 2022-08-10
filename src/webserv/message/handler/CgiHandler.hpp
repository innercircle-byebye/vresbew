#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <cstdlib>
#include <map>
#include <string>

#include "webserv/message/handler/MessageHandler.hpp"
#include "webserv/socket/Connection.hpp"

namespace ft {

class CgiHandler {
 public:
  static void initCgiChild(Connection *c);
  static void handleCgiHeader(Connection *c);
  static void setupCgiMessage(Connection *c);

 private:
  static char **setEnviron(Connection *c);
  static char **setCommand(std::string command, std::string path);
};

}  // namespace ft
#endif
