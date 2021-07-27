#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

#include "webserv/config/LocationConfig.hpp"
#include "webserv/config/ServerConfig.hpp"

namespace ft {

class ServerConfig;

class LocationConfig {
 private:
  std::string uri;

  std::string root;
  std::vector<std::string> index;
  bool autoindex;
  unsigned long client_max_body_size;
  std::map<int, std::string> error_page;
  int return_code;
  std::string return_value;
  std::set<std::string> limit_except;
  std::vector<std::string> cgi;
  std::string cgi_path;

 public:
  LocationConfig(ServerConfig *server_config);
  LocationConfig(std::vector<std::string> tokens, ServerConfig *server_config);
  ~LocationConfig(void);

  bool checkPrefixMatchUri(std::string request_uri);

  const std::string &getUri(void) const;
  const std::string &getRoot(void) const;
  const std::vector<std::string> &getIndex(void) const;
  const bool &getAutoindex(void) const;
  const unsigned long &getClientMaxBodySize(void) const;
  const std::map<int, std::string> &getErrorPage(void) const;
  int getReturnCode(void) const;
  const std::string &getReturnValue(void) const;
  const std::string &getCgiPath(void) const;

  bool checkReturn(void) const;
  bool checkAcceptedMethod(const std::string &request_method) const;
  bool checkCgiExtension(const std::string &request_uri) const;

  // for debug
  void print_status_for_debug(std::string prefix);          // TODO : remove
  const std::set<std::string> &getLimitExcept(void) const;  // TODO : remove
};
}  // namespace ft
#endif
