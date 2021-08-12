#include "webserv/config/LocationConfig.hpp"
namespace ft {

LocationConfig::LocationConfig(ServerConfig *server_config) {
  this->uri_ = "/";
  init(server_config);
}

LocationConfig::LocationConfig(std::vector<std::string> tokens, ServerConfig *server_config) {
  init(server_config);

  std::vector<std::string>::iterator it = tokens.begin();
  const std::vector<std::string>::iterator end_it = tokens.end();
  it++;
  this->uri_ = *(it);
  it += 2;

  bool check_root_setting = false;
  bool check_index_setting = false;
  bool check_autoindex_setting = false;
  bool check_client_max_body_size_setting = false;
  bool check_error_page_setting = false;
  bool check_limit_except_setting = false;
  bool check_return_setting = false;
  bool check_cgi_setting = false;
  bool check_cgi_path_setting = false;

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
    else if (*it == "limit_except")
      limitExceptProcess(it, end_it, check_limit_except_setting);
    else if (*it == "return")
      returnProcess(it, end_it, check_return_setting);
    else if (*it == "cgi")
      cgiProcess(it, end_it, check_cgi_setting);
    else if (*it == "cgi_path")
      cgiPathProcess(it, end_it, check_cgi_path_setting);
    else
      throw std::runtime_error("webserv: [emerg] unknown directive \"" + (*it) + "\"");
  }

  if (check_cgi_setting != check_cgi_path_setting) {
    throw std::runtime_error("webserv: [emerg] \"cgi\" and \"cgi_path\" directives must be used together");
  }
}

LocationConfig::~LocationConfig(void) {}

bool LocationConfig::checkPrefixMatchUri(std::string request_uri) {
  if (this->uri_.length() <= request_uri.length()) {
    if (request_uri.compare(0, this->uri_.length(), this->uri_) == 0) {
      return true;
    }
  }
  return false;
}

const std::string &LocationConfig::getUri(void) const {
  return this->uri_;
}

const std::string &LocationConfig::getRoot(void) const {
  return this->root_;
}

const std::vector<std::string> &LocationConfig::getIndex(void) const {
  return this->index_;
}

const bool &LocationConfig::getAutoindex(void) const {
  return this->autoindex_;
}

const unsigned long &LocationConfig::getClientMaxBodySize(void) const {
  return this->client_max_body_size_;
}

const std::string &LocationConfig::getErrorPage(void) const {
  return this->error_page_;
}

int LocationConfig::getReturnCode(void) const {
  return this->return_code_;
}

const std::string &LocationConfig::getReturnValue(void) const {
  return this->return_value_;
}

const std::string &LocationConfig::getCgiPath(void) const {
  return this->cgi_path_;
}

bool LocationConfig::checkReturn(void) const {
  return this->return_code_ != NOT_SET;
}

bool LocationConfig::checkAcceptedMethod(const std::string &request_method) const {
  if (this->limit_except_.size() == 0 || this->limit_except_.count(request_method) == 1)
    return true;
  // if (!request_method.compare("HEAD") && this->limit_except.count("GET") == 1)
  //   return true;
  return false;
}

bool LocationConfig::checkCgiExtension(const std::string &request_uri) const {
  for (std::vector<std::string>::const_iterator i = this->cgi_.begin(); i != this->cgi_.end(); i++) {
    if (request_uri.rfind(*i) + i->length() == request_uri.length())  // request_uri의 suffix가 *i인지 확인
      return true;
  }
  return false;
}

void LocationConfig::init(ServerConfig *server_config) {
  this->root_ = server_config->getRoot();
  this->index_ = server_config->getIndex();
  this->autoindex_ = server_config->getAutoindex();
  this->client_max_body_size_ = server_config->getClientMaxBodySize();
  this->error_page_ = server_config->getErrorPage();
  this->return_code_ = NOT_SET;
  this->return_value_ = "";
  this->cgi_path_ = "";
}

void LocationConfig::rootProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_root_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt != 1)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"root\" directive");
  if (check_root_setting == true)
    throw std::runtime_error("webserv: [emerg] \"root\" directive is duplicate");

  this->root_ = *(it + 1);
  check_root_setting = true;

  it += 3;
}

void LocationConfig::indexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_index_setting) {
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

void LocationConfig::autoindexProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_autoindex_setting) {
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

void LocationConfig::clientMaxBodySizeProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_client_max_body_size_setting) {
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

  for (std::string::iterator i = size_str.begin(); i != size_str.end(); i++) {
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

void LocationConfig::errorPageProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_error_page_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt != 1)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"error_page\" directive");
  if (check_error_page_setting == true)
    throw std::runtime_error("webserv: [emerg] \"error_page\" directive is duplicate");

  this->error_page_ = *(it + 1);
  check_error_page_setting = true;
  it += 3;
}

void LocationConfig::limitExceptProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_limit_except_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");
  if (directiveValueCnt == 0)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"limit_except\" directive");
  if (check_limit_except_setting == true)
    throw std::runtime_error("webserv: [emerg] \"limit_except\" directive is duplicate");

  it++;
  while (*it != ";") {
    if (*it != "GET" && *it != "HEAD" && *it != "POST" && *it != "PUT" && *it != "DELETE") {
      throw std::runtime_error("webserv: [emerg] invalid method \"" + (*it) + "\"");
    }
    this->limit_except_.insert(*it);
    it++;
  }

  check_limit_except_setting = true;
  it++;
}

void LocationConfig::returnProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_return_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");

  if (directiveValueCnt != 1 && directiveValueCnt != 2)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"return\" directive");

  if (check_return_setting == true) {
    it += (directiveValueCnt + 2);
    return;
  }
  check_return_setting = true;

  if (directiveValueCnt == 1) {
    if ((*(it + 1)).find("http://") == 0 || (*(it + 1)).find("https://") == 0) {
      this->return_code_ = 302;
      this->return_value_ = *(it + 1);
    } else {
      std::string &code = *(it + 1);
      if (code.size() > 3)
        throw std::runtime_error("webserv: [emerg] invalid return code \"" + code + "\"");
      for (std::string::iterator i = code.begin(); i != code.end(); i++) {
        if (!isdigit(*i))
          throw std::runtime_error("webserv: [emerg] invalid return code \"" + code + "\"");
      }
      this->return_code_ = atoi(code.c_str());
    }
  } else if (directiveValueCnt == 2) {
    std::string &code = *(it + 1);
    if (code.size() > 3)
      throw std::runtime_error("webserv: [emerg] invalid return code \"" + code + "\"");
    for (std::string::iterator i = code.begin(); i != code.end(); i++) {
      if (!isdigit(*i))
        throw std::runtime_error("webserv: [emerg] invalid return code \"" + code + "\"");
    }
    this->return_code_ = atoi(code.c_str());
    this->return_value_ = *(it + 2);
  }
  it += directiveValueCnt + 2;
}

void LocationConfig::cgiProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_cgi_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");

  if (check_cgi_setting == true)
    throw std::runtime_error("webserv: [emerg] \"cgi\" directive is duplicate");

  if (directiveValueCnt == 0) {
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"cgi\" directive");
  }

  it++;
  for (; *it != ";"; it++) {
    if (it->find(".") != 0)
      throw std::runtime_error("webserv: [emerg] invalid cgi extention \"" + (*it) + "\"");
    this->cgi_.push_back(*it);
  }
  it++;
  check_cgi_setting = true;
}

void LocationConfig::cgiPathProcess(std::vector<std::string>::iterator &it, const std::vector<std::string>::iterator &end_it, bool &check_cgi_path_setting) {
  int directiveValueCnt = getDirectiveValueCnt(it, end_it, ";");

  if (check_cgi_path_setting == true)
    throw std::runtime_error("webserv: [emerg] \"cgi_path\" directive is duplicate");
  if (directiveValueCnt != 1)
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"cgi_path\" directive");

  this->cgi_path_ = *(it + 1);
  it += 3;
  check_cgi_path_setting = true;
}

int LocationConfig::getDirectiveValueCnt(std::vector<std::string>::iterator it, std::vector<std::string>::iterator end_it, std::string terminator) const {
  it++;
  int cnt = 0;
  for (; it != end_it && *it != terminator; it++)
    cnt++;
  if (it == end_it)
    throw std::runtime_error("webserv: [emerg] unexpected end of file");
  return cnt;
}

// ############## for debug ###################
void LocationConfig::print_status_for_debug(std::string prefix) {
  std::cout << prefix;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ LocationConfig ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  std::cout << prefix;
  std::cout << "uri_path : " << this->uri_ << std::endl;

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
  std::cout << "limit_except : ";
  for (std::set<std::string>::iterator i = this->limit_except_.begin(); i != this->limit_except_.end(); i++) {
    std::cout << *i << " ";
  }
  std::cout << std::endl;

  std::cout << prefix;
  std::cout << "return_code : " << this->return_code_ << std::endl;

  std::cout << prefix;
  std::cout << "return_value : " << this->return_value_ << std::endl;

  std::cout << prefix;
  std::cout << "checkReturn() : ";
  if (this->checkReturn()) {
    std::cout << "true" << std::endl;
  } else {
    std::cout << "false" << std::endl;
  }

  std::cout << prefix;
  std::cout << "cgi : ";
  for (std::vector<std::string>::iterator i = this->cgi_.begin(); i != this->cgi_.end(); i++) {
    std::cout << *i << " ";
  }
  std::cout << std::endl;

  std::cout << prefix;
  std::cout << "cgi_path : " << this->cgi_path_ << std::endl;

  std::cout << prefix;
  std::cout << " - checkCgiExtention - " << std::endl;
  std::cout << prefix;
  std::cout << ".bin : " << this->checkCgiExtension(".bin") << std::endl;
  std::cout << prefix;
  std::cout << ".bla : " << this->checkCgiExtension(".bla") << std::endl;
  std::cout << prefix;
  std::cout << ".cgi : " << this->checkCgiExtension(".cgi") << std::endl;
  std::cout << prefix;
  std::cout << ".test : " << this->checkCgiExtension(".test") << std::endl;

  std::cout << prefix;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
}

const std::set<std::string> &LocationConfig::getLimitExcept(void) const {
  return this->limit_except_;
}
}  // namespace ft
