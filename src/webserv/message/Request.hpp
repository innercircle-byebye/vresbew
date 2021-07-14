
#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <map>
#include <string>

namespace ft {

struct Request {
  // start line
  std::string method;
  std::string uri;
  std::string version;

  // header
  std::map<std::string, std::string> headers;
};

}  // namespace ft

#endif
