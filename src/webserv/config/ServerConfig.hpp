#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/config/LocationConfig.hpp"

namespace ft {

class HttpConfig;

class ServerConfig {
 private:
  std::vector<LocationConfig *> location_configs_;

  std::string root_;
  std::vector<std::string> index_;
  bool autoindex_;
  unsigned long client_max_body_size_;
  std::string error_page_;
  std::vector<std::string> listen_;
  std::vector<std::string> server_name_;

 public:
  ServerConfig(std::vector<std::string> tokens, HttpConfig *http_config);
  ~ServerConfig();

  bool isMatchServerName(std::string request_server_name);
  LocationConfig *getLocationConfig(std::string request_uri);
  const std::string &getRoot(void) const;
  const std::vector<std::string> &getIndex(void) const;
  const bool &getAutoindex(void) const;
  const unsigned long &getClientMaxBodySize(void) const;
  const std::string &getErrorPage(void) const;
  const std::vector<std::string> &getListen(void) const;
  const std::vector<std::string> &getServerName(void) const;

 private:
  void init(HttpConfig *http_config);
  void rootProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_root_setting);
  void indexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_index_setting);
  void autoindexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_autoindex_setting);
  void clientMaxBodySizeProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_client_max_body_size_setting);
  void errorPageProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_error_page_setting);
  void listenProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_listen_setting);
  void serverNameProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_server_name_setting);
  void locationProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, std::vector<std::vector<std::string> > &locations_tokens);
  void createLocationConfig(std::vector<std::vector<std::string> > &locations_tokens);
  int getDirectiveValueCnt(std::vector<std::string>::iterator it, std::vector<std::string>::iterator end_it, std::string terminator) const;
  static bool compareUriForDescendingOrderByLength(const LocationConfig *first, const LocationConfig *second);

 public:
  // for debug
  void print_status_for_debug(std::string prefix);
  const std::vector<LocationConfig *> &getLocationConfigs(void) const;
};
}  // namespace ft
#endif
