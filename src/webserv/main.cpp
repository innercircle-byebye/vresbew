
#include "webserv/config/HttpConfig.hpp"
#include "webserv/message/Response.hpp"
#include "webserv/socket/Cycle.hpp"
#include "webserv/webserv.hpp"

int main(int argc, char *argv[]) {
  if (argc <= 2) {
    std::string config_file_name;
    config_file_name = ((argc == 1) ? "./config/config_tester.conf" : argv[1]);

    try {
      ft::HttpConfig *httpconfig = new ft::HttpConfig(config_file_name);
      ft::Cycle cycle(httpconfig);

      cycle.webservCycle();
      delete httpconfig;
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
      return (-1);
    }
  }
  return (0);
}
