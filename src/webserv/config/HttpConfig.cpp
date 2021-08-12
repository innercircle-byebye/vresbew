#include "webserv/config/HttpConfig.hpp"

namespace ft {
HttpConfig::HttpConfig(std::string program_name, std::string config_file_path) {
  setProgramName(program_name);
  init();

  std::vector<std::string> tokens = tokenizeConfigFile(config_file_path);
  std::vector<std::string>::iterator it = tokens.begin();
  const std::vector<std::string>::iterator end_it = tokens.end();

  if (!(it != end_it && *it == "http"))
    throw std::runtime_error("webserv: [emerg] must start with \"http\" directive");
  it++;
  if (!(it != end_it && *it == "{"))
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"http\" directive");
  it++;

  std::vector<std::vector<std::string> > servers_tokens;
  bool check_root_setting = false;
  bool check_index_setting = false;
  bool check_autoindex_setting = false;
  bool check_client_max_body_size_setting = false;
  bool check_error_page_setting = false;

  while (it != end_it && *it != "}") {
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
    else if (*it == "server")
      serverProcess(it, end_it, servers_tokens);
    else
      throw std::runtime_error("webserv: [emerg] unknown directive \"" + (*it) + "\"");
  }

  if (it == end_it)
    throw std::runtime_error("webserv: [emerg] unexpected end of file");
  if (it + 1 != end_it)
    throw std::runtime_error("webserv: [emerg] wrong configuration file");

  createServerConfig(servers_tokens);
}

HttpConfig::~HttpConfig() {
  std::cout << "~HttpConfig() 호출~~~" << std::endl;
}

const std::string &HttpConfig::getProgramName(void) const {
  return this->program_name_;
}

ServerConfig *HttpConfig::getServerConfig(in_port_t port, in_addr_t ip_addr, std::string server_name) {
  if (this->server_configs_.find(port) == this->server_configs_.end())
    return NULL;

  std::vector<ServerConfig *> *server_list = NULL;

  if (this->server_configs_[port].find(ip_addr) != this->server_configs_[port].end()) {
    server_list = &this->server_configs_[port][ip_addr];
  } else if (this->server_configs_[port].find(inet_addr("0.0.0.0")) != this->server_configs_[port].end()) {
    server_list = &this->server_configs_[port][inet_addr("0.0.0.0")];
  }

  if (server_list == NULL) {
    return NULL;
  }

  ServerConfig *server_ptr = (*server_list)[0];

  for (std::vector<ServerConfig *>::iterator it = server_list->begin(); it != server_list->end(); it++) {
    if ((*it)->isMatchServerName(server_name)) {
      server_ptr = *it;
    }
  }

  return server_ptr;
}

LocationConfig *HttpConfig::getLocationConfig(in_port_t port, in_addr_t ip_addr, std::string server_name, std::string request_uri) {
  ServerConfig *server_config = this->getServerConfig(port, ip_addr, server_name);

  if (server_config == NULL)
    return NULL;

  LocationConfig *location_config = server_config->getLocationConfig(request_uri);
  return location_config;
}

const std::multimap<in_port_t, in_addr_t> &HttpConfig::getMustListens(void) const {
  return this->must_listens_;
}

const std::string &HttpConfig::getRoot(void) const {
  return this->root_;
}

const std::vector<std::string> &HttpConfig::getIndex(void) const {
  return this->index_;
}

const bool &HttpConfig::getAutoindex(void) const {
  return this->autoindex_;
}

const unsigned long &HttpConfig::getClientMaxBodySize(void) const {
  return this->client_max_body_size_;
}

const std::string &HttpConfig::getErrorPage(void) const {
  return this->error_page_;
}

void HttpConfig::init(void) {
  this->root_ = "html";
  this->index_.push_back("index.html");
  this->autoindex_ = false;
  this->client_max_body_size_ = 1000000;
  this->error_page_ = "";
}

void HttpConfig::setProgramName(const std::string &program_name) {
  size_t program_name_start_pos;
  if ((program_name_start_pos = program_name.rfind('/')) != std::string::npos)
    this->program_name_ = program_name.substr(program_name_start_pos + 1);
  else
    this->program_name_ = program_name;
}

std::vector<std::string> HttpConfig::tokenizeConfigFile(const std::string &config_file_path) {
  std::ifstream ifs(config_file_path);
  std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
  ft::Tokenizer tokenizer;
  return tokenizer.parse(content);
}

void HttpConfig::rootProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_root_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt != 1)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"root\" directive");
  if (check_root_setting == true)
    throw std::runtime_error("webserv: [emerg] \"root\" directive is duplicate");

  this->root_ = *(it + 1);
  check_root_setting = true;

  it += 3;
}

void HttpConfig::indexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_index_setting) {
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

void HttpConfig::autoindexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_autoindex_setting) {
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

void HttpConfig::clientMaxBodySizeProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_client_max_body_size_setting) {
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

void HttpConfig::errorPageProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_error_page_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt != 1)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"error_page\" directive");
  if (check_error_page_setting == true)
    throw std::runtime_error("webserv: [emerg] \"error_page\" directive is duplicate");
  
  this->error_page_ = *(it + 1);
  check_error_page_setting = true;
  it += 3;
}

void HttpConfig::serverProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, std::vector<std::vector<std::string> > &servers_tokens) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, "{");
  if (directiveValueCnt != 0)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"server\" directive");

  std::vector<std::string> server_tokens;

  server_tokens.push_back(*it);
  it++;
  server_tokens.push_back(*it);
  it++;

  int cnt = 1;
  while (it != end_it && cnt != 0) {
    if (*it == "{")
      cnt++;
    else if (*it == "}")
      cnt--;
    server_tokens.push_back(*it);
    it++;
  }
  if (it == end_it)
    throw std::runtime_error("webserv: [emerg] unexpected end of file");
  servers_tokens.push_back(server_tokens);
}

void HttpConfig::createServerConfig(std::vector<std::vector<std::string> > &servers_tokens) {
  std::vector<std::vector<std::string> >::iterator server_it = servers_tokens.begin();
  for (; server_it != servers_tokens.end(); server_it++) {
    ServerConfig *new_server = new ServerConfig(*server_it, this);

    for (std::vector<std::string>::const_iterator i = new_server->getListen().begin(); i != new_server->getListen().end(); i++) {
      std::size_t pos = (*i).find(':');
      std::string ip_addr_str = (*i).substr(0, pos);
      std::string port_str = (*i).substr(pos + 1);

      in_addr_t ip_addr = inet_addr(ip_addr_str.c_str());
      in_port_t port = htons(atoi(port_str.c_str()));

      this->server_configs_[port][ip_addr].push_back(new_server);
      setMustListens(ip_addr, port);
    }
  }
}

void HttpConfig::setMustListens(in_addr_t ip_addr, in_port_t port) {
  if (this->must_listens_.find(port) == this->must_listens_.end() || this->must_listens_.find(port)->second != inet_addr("0.0.0.0")) {
    if (ip_addr == inet_addr("0.0.0.0")) {
      this->must_listens_.erase(port);
    }
    this->must_listens_.insert(std::pair<in_port_t, in_addr_t>(port, ip_addr));
  }
}

int HttpConfig::getDirectiveValueCnt(std::vector<std::string>::iterator it, std::vector<std::string>::iterator end_it, std::string terminator) const {
  it++;
  int cnt = 0;
  for (; it != end_it && *it != terminator; it++)
    cnt++;
  if (it == end_it)
    throw std::runtime_error("webserv: [emerg] unexpected end of file");
  return cnt;
}

// ############## for debug ###################
void HttpConfig::print_all_server_location_for_debug(void)
{
  this->print_status_for_debug("");

  for (std::map<in_port_t, std::map<in_addr_t, std::vector<ServerConfig *> > >::iterator it = server_configs_.begin(); it != server_configs_.end(); it++) {
    in_port_t port = it->first;
    std::map<in_addr_t, std::vector<ServerConfig *> > addr_server_map = it->second;

    std::cout << "port : " << ntohs(port) << std::endl;

    for (std::map<in_addr_t, std::vector<ServerConfig *> >::iterator it2 = addr_server_map.begin(); it2 != addr_server_map.end(); it2++) {
      in_addr_t ip_addr = it2->first;
      std::vector<ServerConfig *> server_list = it2->second;

      struct in_addr addr1;
      addr1.s_addr = ip_addr;
      std::cout << "\tip_addr : " << inet_ntoa(addr1) << std::endl;

      for (std::vector<ServerConfig *>::iterator it3 = server_list.begin(); it3 != server_list.end(); it3++) {
        (*it3)->print_status_for_debug("\t\t");

        std::vector<LocationConfig *> locations = (*it3)->getLocationConfigs();
        for (std::vector<LocationConfig *>::iterator it4 = locations.begin(); it4 != locations.end(); it4++) {
          (*it4)->print_status_for_debug("\t\t\t");
        }
      }
    }
  }
}

void HttpConfig::print_status_for_debug(std::string prefix) {
  std::cout << prefix;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ HttpConfig ~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  std::cout << prefix;
  std::cout << "program_name : " << this->program_name_ << std::endl;

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
}  // namespace ft
