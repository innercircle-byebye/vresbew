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
  static std::string to_string(int x) {
    int length = snprintf(NULL, 0, "%d", x);
    assert(length >= 0);
    char *buf = new char[length + 1];
    snprintf(buf, length + 1, "%d", x);
    std::string str(buf);
    delete[] buf;
    return str;
  }
};

}  // namespace ft
#endif
