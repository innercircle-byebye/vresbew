#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <iostream>
#include <map>
#include <string>

namespace ft {
class Response {
 public:
  std::string status_code_;
  std::string status_message_;
  std::string http_ver_;
  std::map<std::string, std::string> header_;
  std::string response_body_;
  std::string response_temp_;

  Response();

 private:
};
}  // namespace ft
#endif
