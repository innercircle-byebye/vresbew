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
  static char **setEnviron(std::map<std::string, std::string> env);
  static char **setCommand(std::string command, std::string path);

 public:
  static void init_cgi_child(Connection *c);
  static void handle_cgi_header(Connection *c);
  static void handle_cgi_body(Connection *c, ssize_t recv_len);

};

}  // namespace ft
#endif
