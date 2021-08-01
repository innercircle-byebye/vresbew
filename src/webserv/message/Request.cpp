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
  uri_.clear();
  http_version_.clear();
  headers_.clear();
  entity_body_.clear();
}

std::string &Request::getMsg() { return msg_; }
const std::string &Request::getMethod() const { return method_; }
const std::string &Request::getSchema() const { return schema_; }
const std::string &Request::getHost() const { return host_; }
const std::string &Request::getPort() const { return port_; }
std::string &Request::getUri() { return uri_; }
const std::string &Request::getHttpVersion() const { return http_version_; }
const std::map<std::string, std::string> &Request::getHeaders() const { return headers_; }
const std::string &Request::getHeaderValue(const std::string &key) { return headers_[key]; }
const std::string &Request::getEntityBody() const { return entity_body_; }

void Request::setMethod(std::string method) { method_ = method; }
void Request::setSchema(std::string schema) { schema_ = schema; }
void Request::setHost(std::string host) { host_ = host; }
void Request::setPort(std::string port) { port_ = port; }
void Request::setUri(std::string uri) { uri_ = uri; }
void Request::setHttpVersion(std::string http_version) { http_version_ = http_version; }
void Request::setHeader(std::string key, std::string value) { headers_[key] = value; }
void Request::setEntityBody(std::string entity_body) { entity_body_ = entity_body; }
void Request::appendEntityBody(char *buffer) { entity_body_.append(buffer); }
void Request::appendEntityBody(char *buffer, size_t size) { entity_body_.append(buffer, size); }

}  // namespace ft
