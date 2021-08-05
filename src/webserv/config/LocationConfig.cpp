#include "webserv/config/LocationConfig.hpp"
namespace ft {

LocationConfig::LocationConfig(ServerConfig *server_config) {
  this->uri = "/";

  this->root = server_config->getRoot();
  this->index = server_config->getIndex();
  this->autoindex = server_config->getAutoindex();
  this->client_max_body_size = server_config->getClientMaxBodySize();
  this->return_code = -1;
  this->return_value = "";
  this->cgi_path = "";
  
  this->error_page = server_config->getErrorPage();
}

LocationConfig::LocationConfig(std::vector<std::string> tokens, ServerConfig *server_config) {
  // 초기화부분
  this->root = server_config->getRoot();
  this->index = server_config->getIndex();
  this->autoindex = server_config->getAutoindex();
  this->client_max_body_size = server_config->getClientMaxBodySize();
  this->return_code = -1;
  this->return_value = "";
  this->cgi_path = "";

  // 한번이라도 세팅했었는지 체크하는 변수
  bool check_root_setting = false;
  bool check_index_setting = false;
  bool check_autoindex_setting = false;
  bool check_client_max_body_size = false;
  bool check_limit_except = false;
  bool check_return = false;
  bool check_cgi = false;
  bool check_cgi_path = false;

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
      int count = 0;  // error_page 지시어 뒤에오는 단어의 개수
      while (*(it + count + 1) != ";")
        count++;

      if (count < 2) {  // error_page 지시어 뒤에 단어가 2개 미만으로 들어오면 에러발생
        throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"error_page\" directive");
      }

      for (int i = 1; i < count; i++) {
        std::string &code = *(it + i);

        for (std::string::iterator i = code.begin(); i != code.end(); i++) {  // code에 숫자만 들어오는지 확인 // 함수로 빼는게 나을듯 ?
          if (!isdigit(*i))
            throw std::runtime_error("webserv: [emerg] invalid value \"" + code + "\"");
        }

        int status_code = atoi(code.c_str());
        if (status_code < 300 || status_code > 599) {  // status_code의 범위 확인
          throw std::runtime_error("webserv: [emerg] value \"" + code + "\" must be between 300 and 599");
        }

        if (this->error_page.find(status_code) == this->error_page.end()) {
          this->error_page[status_code] = *(it + count);
        }
      }
      it += (count + 2);

    } else if (*it == "client_max_body_size") {
      if (*(it + 1) == ";" || *(it + 2) != ";")
        throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"client_max_body_size\" directive");

      if (check_client_max_body_size == true)
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

      this->client_max_body_size = strtoul(size_str.c_str(), NULL, 0);
      if (this->client_max_body_size > LONG_MAX) {
        throw std::runtime_error("webserv: [emerg] \"client_max_body_size\" directive invalid value");
      }
      for (int i = 0; i < num_of_mutifly_by_2; i++) {
        this->client_max_body_size *= 2;
        if (this->client_max_body_size > LONG_MAX) {
          throw std::runtime_error("webserv: [emerg] \"client_max_body_size\" directive invalid value");
        }
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
    } else if (*it == "return") {
      int count = 1;
      while (*(it + count) != ";")
        count++;

      if (count != 2 && count != 3)
        throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"return\" directive");

      if (check_return == true) {
        it += (count + 1);
        continue;
      }
      check_return = true;

      if (count == 2) {
        if ((*(it + 1)).find("http://") == 0 || (*(it + 1)).find("https://") == 0) {
          this->return_code = 302;
          this->return_value = *(it + 1);
        } else {
          std::string &code = *(it + 1);
          if (code.size() > 3)
            throw std::runtime_error("webserv: [emerg] invalid return code \"" + code + "\"");
          for (std::string::iterator i = code.begin(); i != code.end(); i++) {
            if (!isdigit(*i))
              throw std::runtime_error("webserv: [emerg] invalid return code \"" + code + "\"");
          }
          this->return_code = stoi(code);  // TODO : remove (c++11)
        }
        it += 3;
      } else if (count == 3) {
        std::string &code = *(it + 1);
        if (code.size() > 3)
          throw std::runtime_error("webserv: [emerg] invalid return code \"" + code + "\"");
        for (std::string::iterator i = code.begin(); i != code.end(); i++) {
          if (!isdigit(*i))
            throw std::runtime_error("webserv: [emerg] invalid return code \"" + code + "\"");
        }
        this->return_code = stoi(code);   // TODO : remove (c++11)
        this->return_value = *(it + 2);
        it += 4;
      }
    } else if (*it == "cgi") {
      if (check_cgi == true)
        throw std::runtime_error("webserv: [emerg] \"cgi\" directive is duplicate");

      it++;
      if (*it == ";") {
        throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"cgi\" directive");
      }

      for (; *it != ";"; it++) {
        if (it->find(".") != 0)
          throw std::runtime_error("webserv: [emerg] invalid cgi extention \"" + (*it) + "\"");
        this->cgi.push_back(*it);
      }
      it++;
      check_cgi = true;
    } else if (*it == "cgi_path") {
      if (check_cgi_path == true)
        throw std::runtime_error("webserv: [emerg] \"cgi_path\" directive is duplicate");
      if (*(it + 1) == ";" || *(it + 2) != ";")
        throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"cgi_path\" directive");

      this->cgi_path = *(it + 1);
      it += 3;
      check_cgi_path = true;
    } else {
      throw std::runtime_error("webserv: [emerg] unknown directive \"" + (*it) + "\"");
    }
  }

  if (check_cgi != check_cgi_path) {
    throw std::runtime_error("webserv: [emerg] \"cgi\" and \"cgi_path\" directives must be used together");
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

const std::vector<std::string> &LocationConfig::getIndex(void) const {
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

int LocationConfig::getReturnCode(void) const {
  return this->return_code;
}

const std::string &LocationConfig::getReturnValue(void) const {
  return this->return_value;
}

const std::string &LocationConfig::getCgiPath(void) const {
  return this->cgi_path;
}

bool LocationConfig::checkReturn(void) const {
  return this->return_code != -1;
}

bool LocationConfig::checkAcceptedMethod(const std::string &request_method) const {
  if (this->limit_except.size() == 0 || this->limit_except.count(request_method) == 1)
    return true;
  if (!request_method.compare("HEAD") && this->limit_except.count("GET") == 1)
    return true;
  return false;
}

bool LocationConfig::checkCgiExtension(const std::string &request_uri) const {
  for (std::vector<std::string>::const_iterator i = this->cgi.begin(); i != this->cgi.end(); i++) {
    if (request_uri.rfind(*i) + i->length() == request_uri.length())  // request_uri의 suffix가 *i인지 확인
      return true;
  }
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
  std::cout << "return_code : " << this->return_code << std::endl;

  std::cout << prefix;
  std::cout << "return_value : " << this->return_value << std::endl;

  std::cout << prefix;
  std::cout << "checkReturn() : ";
  if (this->checkReturn()) {
    std::cout << "true" << std::endl;
  } else {
    std::cout << "false" << std::endl;
  }

  std::cout << prefix;
  std::cout << "cgi : ";
  for (std::vector<std::string>::iterator i = this->cgi.begin(); i != this->cgi.end(); i++) {
    std::cout << *i << " ";
  }
  std::cout << std::endl;

  std::cout << prefix;
  std::cout << "cgi_path : " << this->cgi_path << std::endl;

  std::cout << prefix;
  std::cout << " - checkCgiExtention - " << std::endl;;
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

const std::set<std::string> &LocationConfig::getLimitExcept(void) const  // TODO : remove
{
  return this->limit_except;
}
}  // namespace ft
