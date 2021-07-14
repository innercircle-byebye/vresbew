#include "webserv/message/Response.hpp"

namespace ft {
Response::Response() {
  this->http_ver_ = "HTTP/1.1";
  this->header_["Allow"] = "";
  this->header_["Content-Location"] = "";
  this->header_["Last-Modified"] = "";
  this->header_["Location"] = "";
  this->header_["Retry-After"] = "";
  this->header_["Server"] = "vresbew";
  this->header_["Transfer-Encoding"] = "";
  this->header_["WWW-Authenticate"] = "";
  this->header_["Content-Language"] = "";
  this->header_["Content-Length"] = "";
  this->header_["Content-Type"] = "";
}

}  // namespace ft
