#include "webserv/config/HttpConfig.hpp"

namespace ft {
HttpConfig::HttpConfig(std::string config_file_path) {
  // 초기화부분
  this->root = "html";
  this->index.push_back("index.html");
  this->autoindex = false;
  this->client_max_body_size = 1000000;

  // 한번이라도 세팅했었는지 체크하는 변수
  bool check_root_setting = false;
  bool check_index_setting = false;
  bool check_autoindex_setting = false;
  bool check_client_max_body_size = false;

  // config파일을 읽어서 content 변수에 담는다.
  std::ifstream ifs(config_file_path);
  std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

  // content를 유의미한 단위로 자른다.(토큰화한다)
  ft::Tokenizer tokenizer;
  std::vector<std::string> tokens = tokenizer.parse(content);

  std::vector<std::vector<std::string> > servers_tokens;  // ServerConfig 객체 생성할떄 사용할 토큰들

  std::vector<std::string>::iterator it = tokens.begin();

  if (*it != "http")
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"" + (*it) + "\" directive");
  if (*(it + 1) != "{")
    throw std::runtime_error("webserv: [emerg] invalid number of arguments in \"http\" directive");

  it += 2;
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

    } else if (*it == "server") {
      // TODO : 예외처리해야함

      std::vector<std::string> server_tokens;

      server_tokens.push_back(*it);
      it++;
      server_tokens.push_back(*it);
      it++;

      int cnt = 1;
      while (cnt != 0) {
        if (*it == "{")
          cnt++;
        else if (*it == "}")
          cnt--;
        server_tokens.push_back(*it);
        it++;
      }
      servers_tokens.push_back(server_tokens);

    } else {
      throw std::runtime_error("webserv: [emerg] unknown directive \"" + (*it) + "\"");
    }
  }

  std::vector<std::vector<std::string> >::iterator server_it = servers_tokens.begin();
  for (; server_it != servers_tokens.end(); server_it++) {
    ServerConfig *new_server = new ServerConfig(*server_it, this);

    for (std::vector<std::string>::const_iterator i = new_server->getListen().begin(); i != new_server->getListen().end(); i++) {
      std::size_t pos = (*i).find(':');
      std::string ip_addr_str = (*i).substr(0, pos);
      std::string port_str = (*i).substr(pos + 1);

      in_addr_t ip_addr = inet_addr(ip_addr_str.c_str());
      in_port_t port = htons(atoi(port_str.c_str()));

      this->server_configs[port][ip_addr].push_back(new_server);

      if (this->must_listens.find(port) == this->must_listens.end() || this->must_listens.find(port)->second != inet_addr("0.0.0.0")) {
        if (ip_addr == inet_addr("0.0.0.0")) {
          this->must_listens.erase(port);
        }
        this->must_listens.insert(std::pair<in_port_t, in_addr_t>(port, ip_addr));
      }
    }
  }
}

HttpConfig::~HttpConfig() {
  std::cout << "~HttpConfig() 호출~~~" << std::endl;
}

ServerConfig *HttpConfig::getServerConfig(in_port_t port, in_addr_t ip_addr, std::string server_name) {
  if (this->server_configs.find(port) == this->server_configs.end())
    return NULL;

  std::vector<ServerConfig *> *server_list = NULL;

  if (this->server_configs[port].find(ip_addr) != this->server_configs[port].end()) {
    server_list = &this->server_configs[port][ip_addr];
  } else if (this->server_configs[port].find(inet_addr("0.0.0.0")) != this->server_configs[port].end()) {
    server_list = &this->server_configs[port][inet_addr("0.0.0.0")];
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
  return this->must_listens;
}

const std::string &HttpConfig::getRoot(void) const {
  return this->root;
}

const std::vector<std::string> &HttpConfig::getIndex(void) const {
  return this->index;
}

const bool &HttpConfig::getAutoindex(void) const {
  return this->autoindex;
}

const unsigned long &HttpConfig::getClientMaxBodySize(void) const {
  return this->client_max_body_size;
}

const std::map<int, std::string> &HttpConfig::getErrorPage(void) const {
  return this->error_page;
}

// ############## for debug ###################
void HttpConfig::print_all_server_location_for_debug(void)  // TODO : remove
{
  this->print_status_for_debug("");

  for (std::map<in_port_t, std::map<in_addr_t, std::vector<ServerConfig *> > >::iterator it = server_configs.begin(); it != server_configs.end(); it++) {
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

void HttpConfig::print_status_for_debug(std::string prefix) {  // TODO : remove
  std::cout << prefix;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ HttpConfig ~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

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
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
}
}  // namespace ft
