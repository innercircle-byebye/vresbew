/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sucho <sucho@student.42seoul.kr>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/07/03 14:11:56 by kycho             #+#    #+#             */
/*   Updated: 2021/07/14 17:18:28 by sucho            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv/config/LocationConfig.hpp"
namespace ft {

LocationConfig::LocationConfig(std::vector<std::string> tokens, ServerConfig *server_config) {
  // 초기화부분
  this->root = server_config->getRoot();
  this->index = server_config->getIndex();
  this->autoindex = server_config->getAutoindex();
  this->client_max_body_size = server_config->getClientMaxBodySize();

  // 한번이라도 세팅했었는지 체크하는 변수
  bool check_root_setting = false;
  bool check_index_setting = false;
  bool check_autoindex_setting = false;
  bool check_client_max_body_size = false;
  bool check_limit_except = false;

  std::vector<std::string>::iterator it = tokens.begin();  // "location"
  it++;                                                    // path
  this->uri = *(it);
  it++;  // "{"
  it++;  // any directive

  while (*it != "}") {
    if (*it == "root") {
      if (*(it + 1) == ";" || *(it + 2) != ";")
        throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"root\" directive");
      if (check_root_setting == true)
        throw std::runtime_error("webserv: [emerg] \"root\" directive is duplicate");

      this->root = *(it + 1);
      check_root_setting = true;

      it += 3;
    } else if (*it == "index") {
      if (*(it + 1) == ";")
        throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"index\" directive");

      if (check_index_setting == false) {
        this->index.clear();
        check_index_setting = true;
      }

      it++;
      while (*it != ";") {
        this->index.push_back(*it);
        it++;
      }
      it++;
    } else if (*it == "autoindex") {
      if (*(it + 1) == ";" || *(it + 2) != ";")
        throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"autoindex\" directive");
      if (check_autoindex_setting == true)
        throw std::runtime_error("webserv: [emerg] \"autoindex\" directive is duplicate");
      if ((*(it + 1)) != "on" && *(it + 1) != "off")
        throw std::runtime_error("webserv: [emerg] invalid value \"" + *(it + 1) + "\" in \"autoindex\" directive, it must be \"on\" or \"off\"");

      if (*(it + 1) == "on")
        this->autoindex = true;
      else
        this->autoindex = false;

      check_autoindex_setting = true;
      it += 3;
    } else if (*it == "error_page") {
      // TODO : 예외처리해야함

      int count = 2;
      while (*(it + count + 1) != ";")
        count++;

      for (int i = 1; i < count; i++) {
        int status_code = atoi((*(it + i)).c_str());

        if (this->error_page.find(status_code) == this->error_page.end()) {
          this->error_page[status_code] = *(it + count);
        }
      }
      it += (count + 2);
    } else if (*it == "client_max_body_size") {
      // TODO : 예외처리해야함
      if (check_client_max_body_size == true)
        throw std::runtime_error("webserv: [emerg] \"client_max_body_size\" directive is duplicate");

      std::string size_str = *(it + 1);

      this->client_max_body_size = atoi(size_str.c_str());

      char last_char = size_str[size_str.length() - 1];
      if (last_char == 'k') {
        this->client_max_body_size *= 1000;
      } else if (last_char == 'm') {
        this->client_max_body_size *= 1000000;
      } else if (last_char == 'g') {
        this->client_max_body_size *= 1000000000;
      }

      check_client_max_body_size = true;
      it += 3;
    } else if (*it == "limit_except") {
      if (*(it + 1) == ";")
        throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"limit_except\" directive");
      if (check_limit_except == true)
        throw std::runtime_error("webserv: [emerg] \"limit_except\" directive is duplicate");

      it++;
      while (*it != ";") {
        if (*it != "GET" && *it != "HEAD" && *it != "POST" && *it != "PUT" && *it != "DELETE") {
          throw std::runtime_error("webserv: [emerg] invalid method \"" + (*it) + "\"");
        }
        this->limit_except.insert(*it);
        it++;
      }

      check_limit_except = true;
      it++;
    } else {
      throw std::runtime_error("webserv: [emerg] unknown directive \"" + (*it) + "\"");
    }
  }

  for (std::map<int, std::string>::const_iterator i = server_config->getErrorPage().begin(); i != server_config->getErrorPage().end(); i++) {
    int status_code = i->first;
    std::string path = i->second;

    if (this->error_page.find(status_code) == this->error_page.end()) {
      this->error_page[status_code] = path;
    }
  }
}

LocationConfig::~LocationConfig(void) {
  std::cout << "~LocationConfig() 호출~~~" << std::endl;
}

bool LocationConfig::checkPrefixMatchUri(std::string request_uri) {
  if (this->uri.length() <= request_uri.length()) {
    if (request_uri.compare(0, this->uri.length(), this->uri) == 0) {
      return true;
    }
  }
  return false;
}

const std::string &LocationConfig::getUri(void) const {
  return this->uri;
}

const std::string &LocationConfig::getRoot(void) const {
  return this->root;
}

const std::vector<std::string> LocationConfig::getIndex(void) const {
  return this->index;
}

const bool &LocationConfig::getAutoindex(void) const {
  return this->autoindex;
}

const unsigned long &LocationConfig::getClientMaxBodySize(void) const {
  return this->client_max_body_size;
}

const std::map<int, std::string> &LocationConfig::getErrorPage(void) const {
  return this->error_page;
}

bool LocationConfig::checkAcceptedMethod(const std::string request_method) const {
  if (this->limit_except.size() == 0 || this->limit_except.count(request_method) == 1)
    return true;
  return false;
}

// ############## for debug ###################
void LocationConfig::print_status_for_debug(std::string prefix)  // TODO : remove
{
  std::cout << prefix;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ LocationConfig ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  std::cout << prefix;
  std::cout << "uri_path : " << this->uri << std::endl;

  std::cout << prefix;
  std::cout << "root : " << this->root << std::endl;

  std::cout << prefix;
  std::cout << "index : ";
  for (std::vector<std::string>::iterator i = this->index.begin(); i != this->index.end(); i++) {
    std::cout << *i << " ";
  }
  std::cout << std::endl;

  std::cout << prefix;
  std::cout << "autoindex : " << this->autoindex << std::endl;

  std::cout << prefix;
  std::cout << "client_max_body_size : " << this->client_max_body_size << std::endl;

  std::cout << prefix;
  std::cout << "error_page : ";
  for (std::map<int, std::string>::iterator i = this->error_page.begin(); i != this->error_page.end(); i++) {
    std::cout << i->first << ":" << i->second << "  ";
  }
  std::cout << std::endl;

  std::cout << prefix;
  std::cout << "limit_except : ";
  for (std::set<std::string>::iterator i = this->limit_except.begin(); i != this->limit_except.end(); i++) {
    std::cout << *i << " ";
  }
  std::cout << std::endl;

  std::cout << prefix;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
}

const std::set<std::string> &LocationConfig::getLimitExcept(void) const  // TODO : remove
{
  return this->limit_except;
}
}  // namespace ft
