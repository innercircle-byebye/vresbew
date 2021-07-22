#ifndef HTTP_CONFIG_HPP
#define HTTP_CONFIG_HPP

#include <arpa/inet.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "webserv/config/ServerConfig.hpp"
#include "webserv/config/Tokenizer.hpp"

namespace ft {
class ServerConfig;
class LocationConfig;

class HttpConfig {
 private:
  std::multimap<in_port_t, in_addr_t> must_listens;

  std::string root;
  std::vector<std::string> index;
  bool autoindex;
  unsigned long client_max_body_size;
  std::map<int, std::string> error_page;

  std::map<in_port_t, std::map<in_addr_t, std::vector<ServerConfig *> > > server_configs;

 public:
  HttpConfig(std::string config_file_path);
  ~HttpConfig();

  ServerConfig *getServerConfig(in_port_t port, in_addr_t ip_addr, std::string server_name);
  LocationConfig *getLocationConfig(in_port_t port, in_addr_t ip_addr, std::string server_name, std::string request_uri);

  const std::multimap<in_port_t, in_addr_t> &getMustListens(void) const;
  const std::string &getRoot(void) const;
  const std::vector<std::string> &getIndex(void) const;
  const bool &getAutoindex(void) const;
  const unsigned long &getClientMaxBodySize(void) const;
  const std::map<int, std::string> &getErrorPage(void) const;

  // for debug
  void print_all_server_location_for_debug(void);   // TODO : remove
  void print_status_for_debug(std::string prefix);  // TODO : remove
};
}  // namespace ft
#endif
