#include "webserv/config/ServerConfig.hpp"

namespace ft {

ServerConfig::ServerConfig(std::vector<std::string> tokens, HttpConfig *http_config) {
  init(http_config);

  std::vector<std::string>::iterator it = tokens.begin();
  const std::vector<std::string>::iterator end_it = tokens.end();
  it += 2;

  std::vector<std::vector<std::string> > locations_tokens;
  bool check_root_setting = false;
  bool check_index_setting = false;
  bool check_autoindex_setting = false;
  bool check_client_max_body_size_setting = false;
  bool check_error_page_setting = false;
  bool check_listen_setting = false;
  bool check_server_name_setting = false;

  while (*it != "}") {
    if (*it == "root")
      rootProcess(it, end_it, check_root_setting);
    else if (*it == "index")
      indexProcess(it, end_it, check_index_setting);
    else if (*it == "autoindex")
      autoindexProcess(it, end_it, check_autoindex_setting);
    else if (*it == "client_max_body_size")
      clientMaxBodySizeProcess(it, end_it, check_client_max_body_size_setting);
    else if (*it == "error_page")
      errorPageProcess(it, end_it, check_error_page_setting);
    else if (*it == "listen")
      listenProcess(it, end_it, check_listen_setting);
    else if (*it == "server_name")
      serverNameProcess(it, end_it, check_server_name_setting);
    else if (*it == "location")
      locationProcess(it, end_it, locations_tokens);
    else
      throw std::runtime_error("webserv: [emerg] unknown directive \"" + (*it) + "\"");
  }

  createLocationConfig(locations_tokens);
}

ServerConfig::~ServerConfig(void) {
  for (std::vector<LocationConfig *>::iterator it = this->location_configs_.begin(); it != this->location_configs_.end(); it++) {
    delete (*it);
  }
}

bool ServerConfig::isMatchServerName(std::string request_server_name) {
  for (std::vector<std::string>::iterator it = this->server_name_.begin(); it != this->server_name_.end(); it++) {
    if (*it == request_server_name) {
      return true;
    }
  }
  return false;
}

LocationConfig *ServerConfig::getLocationConfig(std::string request_uri) {
  for (std::vector<LocationConfig *>::iterator it = this->location_configs_.begin(); it != this->location_configs_.end(); it++) {
    if ((*it)->checkPrefixMatchUri(request_uri)) {
      return *it;
    }
  }
  return NULL;
}

const std::vector<std::string> &ServerConfig::getListen(void) const {
  return this->listen_;
}

const std::vector<std::string> &ServerConfig::getServerName(void) const {
  return this->server_name_;
}

const std::string &ServerConfig::getRoot(void) const {
  return this->root_;
}

const std::vector<std::string> &ServerConfig::getIndex(void) const {
  return this->index_;
}

const bool &ServerConfig::getAutoindex(void) const {
  return this->autoindex_;
}

const unsigned long &ServerConfig::getClientMaxBodySize(void) const {
  return this->client_max_body_size_;
}

const std::string &ServerConfig::getErrorPage(void) const {
  return this->error_page_;
}

void ServerConfig::init(HttpConfig *http_config) {
  this->root_ = http_config->getRoot();
  this->index_ = http_config->getIndex();
  this->autoindex_ = http_config->getAutoindex();
  this->client_max_body_size_ = http_config->getClientMaxBodySize();
  this->error_page_ = http_config->getErrorPage();
  this->listen_.push_back("0.0.0.0:80");
  this->server_name_.push_back("");
}

void ServerConfig::rootProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_root_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt != 1)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"root\" directive");
  if (check_root_setting == true)
    throw std::runtime_error("webserv: [emerg] \"root\" directive is duplicate");

  this->root_ = *(it + 1);
  check_root_setting = true;

  it += 3;
}

void ServerConfig::indexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_index_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt == 0)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"index\" directive");

  if (check_index_setting == false) {
    this->index_.clear();
    check_index_setting = true;
  }

  it++;
  for (; *it != ";"; it++)
    this->index_.push_back(*it);
  it++;
}

void ServerConfig::autoindexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_autoindex_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt != 1)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"autoindex\" directive");
  if (check_autoindex_setting == true)
    throw std::runtime_error("webserv: [emerg] \"autoindex\" directive is duplicate");
  if ((*(it + 1)) != "on" && *(it + 1) != "off")
    throw std::runtime_error("webserv: [emerg] invalid value \"" + *(it + 1) + "\" in \"autoindex\" directive, it must be \"on\" or \"off\"");

  if (*(it + 1) == "on")
    this->autoindex_ = true;
  else
    this->autoindex_ = false;

  check_autoindex_setting = true;
  it += 3;
}

void ServerConfig::clientMaxBodySizeProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_client_max_body_size_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt != 1)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"client_max_body_size\" directive");

  if (check_client_max_body_size_setting == true)
    throw std::runtime_error("webserv: [emerg] \"client_max_body_size\" directive is duplicate");

  std::string &size_str = *(it + 1);
  int num_of_mutifly_by_2 = 0;
  if (*size_str.rbegin() == 'k') {
    num_of_mutifly_by_2 = 10;
  } else if (*size_str.rbegin() == 'm') {
    num_of_mutifly_by_2 = 20;
  } else if (*size_str.rbegin() == 'g') {
    num_of_mutifly_by_2 = 30;
  }
  if (num_of_mutifly_by_2 != 0) {
    size_str = size_str.substr(0, size_str.length() - 1);
  }

  if (size_str.length() > 19) {
    throw std::runtime_error("webserv: [emerg] \"client_max_body_size\" directive invalid value");
  }

  for (std::string::iterator i = size_str.begin(); i != size_str.end(); i++) {  // code에 숫자만 들어오는지 확인 // 함수로 빼는게 나을듯 ?
    if (!isdigit(*i))
      throw std::runtime_error("webserv: [emerg] \"client_max_body_size\" directive invalid value");
  }

  this->client_max_body_size_ = strtoul(size_str.c_str(), NULL, 0);
  if (this->client_max_body_size_ > LONG_MAX) {
    throw std::runtime_error("webserv: [emerg] \"client_max_body_size\" directive invalid value");
  }
  for (int i = 0; i < num_of_mutifly_by_2; i++) {
    this->client_max_body_size_ *= 2;
    if (this->client_max_body_size_ > LONG_MAX) {
      throw std::runtime_error("webserv: [emerg] \"client_max_body_size\" directive invalid value");
    }
  }

  check_client_max_body_size_setting = true;
  it += 3;
}

void ServerConfig::errorPageProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_error_page_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt != 1)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"error_page\" directive");
  if (check_error_page_setting == true)
    throw std::runtime_error("webserv: [emerg] \"error_page\" directive is duplicate");

  this->error_page_ = *(it + 1);
  check_error_page_setting = true;
  it += 3;
}

void ServerConfig::listenProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_listen_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt != 1) {
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"listen\" directive");
  }

  if (check_listen_setting == false) {
    listen_.clear();
    check_listen_setting = true;
  }

  std::string listen_value = *(it + 1);

  if (listen_value.find(':') == std::string::npos) {
    if (listen_value.find('.') == std::string::npos) {
      listen_value = "0.0.0.0:" + listen_value;
    } else {
      listen_value = listen_value + ":80";
    }
  }

  const char *ptr = listen_value.c_str();
  char *tmp;
  int num;
  for (int i = 0; i < 4; i++) {
    num = strtoul(ptr, &tmp, 10);
    if (num < 0 || num > 255)
      throw std::runtime_error("webserv: [emerg] host not found in \"" + *(it + 1) + "\" of the \"listen\" directive");
    if (ptr == tmp)
      break;
    if (i == 3) {
      ptr = tmp;
      break;
    }
    if (*tmp != '.')
      throw std::runtime_error("webserv: [emerg] host not found in \"" + *(it + 1) + "\" of the \"listen\" directive");
    ptr = tmp + 1;
    if (*ptr == '\0')
      break;
  }
  if (*ptr != ':')
    throw std::runtime_error("webserv: [emerg] host not found in \"" + *(it + 1) + "\" of the \"listen\" directive");
  ptr++;
  num = strtoul(ptr, &tmp, 10);
  if (num == 0 || num > 65535 || *tmp != '\0')
    throw std::runtime_error("webserv: [emerg] host not found in \"" + *(it + 1) + "\" of the \"listen\" directive");

  this->listen_.push_back(listen_value);
  it += 3;
}

void ServerConfig::serverNameProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_server_name_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt == 0) {
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"server_name\" directive");
  }

  if (check_server_name_setting == false) {
    this->server_name_.clear();
    check_server_name_setting = true;
  }

  it++;
  for (; *it != ";"; it++)
    this->server_name_.push_back(*it);
  it++;
}
void ServerConfig::locationProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, std::vector<std::vector<std::string> > &locations_tokens) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, "{");
  if (directiveValueCnt != 1)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"location\" directive");

  std::vector<std::string> location_tokekns;

  location_tokekns.push_back(*it);
  it++;
  location_tokekns.push_back(*it);
  it++;
  location_tokekns.push_back(*it);
  it++;

  int cnt = 1;
  while (it != end_it && cnt != 0) {
    if (*it == "{")
      cnt++;
    else if (*it == "}")
      cnt--;
    location_tokekns.push_back(*it);
    it++;
  }
  if (it == end_it)
    throw std::runtime_error("webserv: [emerg] unexpected end of file");
  locations_tokens.push_back(location_tokekns);
}

void ServerConfig::createLocationConfig(std::vector<std::vector<std::string> > &locations_tokens) {
  std::set<std::string> location_uri_set;
  std::vector<std::vector<std::string> >::iterator location_it = locations_tokens.begin();
  for (; location_it != locations_tokens.end(); location_it++) {
    LocationConfig *new_location = new LocationConfig(*location_it, this);

    if (location_uri_set.count(new_location->getUri()) == 1)
      throw std::runtime_error("nginx: [emerg] duplicate location \"" + new_location->getUri() + "\"");
    location_uri_set.insert(new_location->getUri());

    this->location_configs_.push_back(new_location);
  }

  if (location_uri_set.count("/") == 0) {
    LocationConfig *default_location = new LocationConfig(this);
    this->location_configs_.push_back(default_location);
  }

  std::sort(this->location_configs_.begin(), this->location_configs_.end(), this->compareUriForDescendingOrderByLength);
}

int ServerConfig::getDirectiveValueCnt(std::vector<std::string>::iterator it, std::vector<std::string>::iterator end_it, std::string terminator) const {
  it++;
  int cnt = 0;
  for (; it != end_it && *it != terminator; it++)
    cnt++;
  if (it == end_it)
    throw std::runtime_error("webserv: [emerg] unexpected end of file");
  return cnt;
}

bool ServerConfig::compareUriForDescendingOrderByLength(const LocationConfig *first, const LocationConfig *second) {
  return first->getUri().length() > second->getUri().length();
}

// ############## for debug ###################
void ServerConfig::print_status_for_debug(std::string prefix) {
  std::cout << prefix;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ServerConfig ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  std::cout << prefix;
  std::cout << "listen : ";
  for (std::vector<std::string>::iterator i = this->listen_.begin(); i != this->listen_.end(); i++) {
    std::cout << *i << "  ";
  }
  std::cout << std::endl;

  std::cout << prefix;
  std::cout << "server_name : ";
  for (std::vector<std::string>::iterator i = this->server_name_.begin(); i != this->server_name_.end(); i++) {
    std::cout << *i << "  ";
  }
  std::cout << std::endl;

  std::cout << prefix;
  std::cout << "root : " << this->root_ << std::endl;

  std::cout << prefix;
  std::cout << "index : ";
  for (std::vector<std::string>::iterator i = this->index_.begin(); i != this->index_.end(); i++) {
    std::cout << *i << " ";
  }
  std::cout << std::endl;

  std::cout << prefix;
  std::cout << "autoindex : " << this->autoindex_ << std::endl;

  std::cout << prefix;
  std::cout << "client_max_body_size : " << this->client_max_body_size_ << std::endl;

  std::cout << prefix;
  std::cout << "error_page : " << this->error_page_ << std::endl;

  std::cout << prefix;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
}

const std::vector<LocationConfig *> &ServerConfig::getLocationConfigs(void) const {
  return this->location_configs_;
}
}  // namespace ft
