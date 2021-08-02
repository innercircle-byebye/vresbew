#include "Request.hpp"

namespace ft {

Request::Request(): port_("80"), uri_("/"){}

Request::~Request() {
  this->clear();
}

void Request::clear() {
  method_.clear();
  schema_.clear();
  host_.clear();
  port_ = "80";
  uri_ = "/";
  http_version_.clear();
  headers_.clear();
  uri_struct_.schema_.clear();
  uri_struct_.host_.clear();
  uri_struct_.port_.clear();
  uri_struct_.path_.clear();
  uri_struct_.query_string_.clear();
}

std::string &Request::getMsg() { return msg_; }
const std::string &Request::getMethod() const { return method_; }
const std::string &Request::getSchema() const { return uri_struct_.schema_; }
const std::string &Request::getHost() const { return uri_struct_.host_; }
const std::string &Request::getPort() const { return uri_struct_.port_; }
const std::string &Request::getPath() const { return uri_struct_.path_; }
const std::string &Request::getQueryString() const { return uri_struct_.query_string_; }
std::string &Request::getUri() { return uri_; }
const std::string &Request::getHttpVersion() const { return http_version_; }
const std::map<std::string, std::string> &Request::getHeaders() const { return headers_; }
const std::string &Request::getHeaderValue(const std::string &key) { return headers_[key]; }

void Request::setMethod(std::string method) { method_ = method; }
void Request::setSchema(std::string schema) { uri_struct_.schema_ = schema; }
void Request::setHost(std::string host) { uri_struct_.host_ = host; }
void Request::setPort(std::string port) { uri_struct_.port_ = port; }
void Request::setPath(std::string path) { uri_struct_.path_ = path; }
void Request::setQueryString(std::string query_string) { uri_struct_.query_string_ = query_string; }
void Request::setUri(std::string uri) { uri_ = uri; }
void Request::setHttpVersion(std::string http_version) { http_version_ = http_version; }
void Request::setHeader(std::string key, std::string value) { headers_[key] = value; }

}  // namespace ft
