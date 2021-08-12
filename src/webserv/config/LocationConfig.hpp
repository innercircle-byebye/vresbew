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
  std::string error_page;
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
  const std::string &getErrorPage(void) const;
  int getReturnCode(void) const;
  const std::string &getReturnValue(void) const;
  const std::string &getCgiPath(void) const;

  bool checkReturn(void) const;
  bool checkAcceptedMethod(const std::string &request_method) const;
  bool checkCgiExtension(const std::string &request_uri) const;

 private:
  void init(ServerConfig* server_config);
  void rootProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_root_setting);
  void indexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_index_setting);
  void autoindexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_autoindex_setting);
  void clientMaxBodySizeProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_client_max_body_size_setting);
  void errorPageProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_error_page_setting);
  void limitExceptProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_limit_except_setting);
  void returnProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_return_setting);
  void cgiProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_cgi_setting);
  void cgiPathProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_cgi_path_setting);
  int getDirectiveValueCnt(std::vector<std::string>::iterator it, std::vector<std::string>::iterator end_it, std::string terminator) const;

 public:
  // for debug
  void print_status_for_debug(std::string prefix);
  const std::set<std::string> &getLimitExcept(void) const;
};
}  // namespace ft
#endif
