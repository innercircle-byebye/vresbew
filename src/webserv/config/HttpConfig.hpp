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
  std::string program_name;
  std::multimap<in_port_t, in_addr_t> must_listens;
  std::map<in_port_t, std::map<in_addr_t, std::vector<ServerConfig *> > > server_configs;

  std::string root;
  std::vector<std::string> index;
  bool autoindex;
  unsigned long client_max_body_size;
  std::string error_page;

 public:
  HttpConfig(std::string program_name, std::string config_file_path);
  ~HttpConfig();

  const std::string &getProgramName(void) const;
  ServerConfig *getServerConfig(in_port_t port, in_addr_t ip_addr, std::string server_name);
  LocationConfig *getLocationConfig(in_port_t port, in_addr_t ip_addr, std::string server_name, std::string request_uri);
  const std::multimap<in_port_t, in_addr_t> &getMustListens(void) const;
  const std::string &getRoot(void) const;
  const std::vector<std::string> &getIndex(void) const;
  const bool &getAutoindex(void) const;
  const unsigned long &getClientMaxBodySize(void) const;
  const std::string &getErrorPage(void) const;

 private:
  void init(void);
  void setProgramName(const std::string &program_name);
  std::vector<std::string> tokenizeConfigFile(const std::string &config_file_path);
  void rootProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_root_setting);
  void indexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_index_setting);
  void autoindexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_autoindex_setting);
  void clientMaxBodySizeProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_client_max_body_size_setting);
  void errorPageProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_error_page_setting);
  void serverProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, std::vector<std::vector<std::string> > &servers_tokens);
  void createServerConfig(std::vector<std::vector<std::string> > &servers_tokens);
  void setMustListens(in_addr_t ip_addr, in_port_t port);
  int getDirectiveValueCnt(std::vector<std::string>::iterator it, std::vector<std::string>::iterator end_it, std::string terminator) const;

 public:
  // for debug
  void print_all_server_location_for_debug(void);
  void print_status_for_debug(std::string prefix);
};
}  // namespace ft
#endif
