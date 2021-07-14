/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sucho <sucho@student.42seoul.kr>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/06/25 13:46:49 by kycho             #+#    #+#             */
/*   Updated: 2021/07/14 17:18:11 by sucho            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
  std::vector<std::string> listen;
  std::vector<std::string> server_name;

  std::string root;
  std::vector<std::string> index;
  bool autoindex;
  unsigned long client_max_body_size;
  std::map<int, std::string> error_page;
  // return_value;

  std::vector<LocationConfig *> location_configs;

 public:
  ServerConfig(std::vector<std::string> tokens, HttpConfig *http_config);
  ~ServerConfig(void);

  bool isMatchServerName(std::string request_server_name);
  LocationConfig *getLocationConfig(std::string request_uri);

  const std::vector<std::string> &getListen(void) const;
  const std::vector<std::string> &getServerName(void) const;
  const std::string &getRoot(void) const;
  const std::vector<std::string> getIndex(void) const;
  const bool &getAutoindex(void) const;
  const unsigned long &getClientMaxBodySize(void) const;
  const std::map<int, std::string> &getErrorPage(void) const;

  // for debug
  void print_status_for_debug(std::string prefix);                      // TODO : remove
  const std::vector<LocationConfig *> &getLocationConfigs(void) const;  // TODO : remove

 private:
  static bool compare_uri_for_descending_order_by_length(const LocationConfig *first, const LocationConfig *second);
};
}  // namespace ft
#endif
