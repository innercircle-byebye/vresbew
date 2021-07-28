#include "Request.hpp"

namespace ft {

Request::Request(): recv_phase_(MESSAGE_START_LINE_INCOMPLETE), buffer_content_length_(0), port_("80"), uri_("/"){}

Request::~Request() {
  this->clear();
}

void Request::clear() {
  msg_.clear();
  recv_phase_ = MESSAGE_START_LINE_INCOMPLETE;
  method_.clear();
  schema_.clear();
  host_.clear();
  // host_ip_.clear();
  port_ = "80";
  uri_ = "/";
  http_version_.clear();
  headers_.clear();
  buffer_content_length_ = 0;
  entity_body_.clear();
}

std::string &Request::getMsg() { return msg_; }
int Request::getRecvPhase() const { return recv_phase_; }
const std::string &Request::getMethod() const { return method_; }
const std::string &Request::getSchema() const { return schema_; }
const std::string &Request::getHost() const { return host_; }
// const std::string &Request::getHostIp() const { return host_ip_; }
const std::string &Request::getPort() const { return port_; }
std::string &Request::getUri() { return uri_; }
const std::string &Request::getHttpVersion() const { return http_version_; }
const std::map<std::string, std::string> &Request::getHeaders() const { return headers_; }
const std::string &Request::getHeaderValue(const std::string &key) { return headers_[key]; }
int Request::getBufferContentLength() const { return buffer_content_length_; }
const std::string &Request::getEntityBody() const { return entity_body_; }

void Request::setMsg(std::string msg) { msg_ = msg; }
void Request::setRecvPhase(int recv_phase) { recv_phase_ = recv_phase; }
void Request::setMethod(std::string method) { method_ = method; }
void Request::setSchema(std::string schema) { schema_ = schema; }
void Request::setHost(std::string host) { host_ = host; }
// void Request::setHostIp(std::string host_ip) { host_ip_ = host_ip; }
void Request::setPort(std::string port) { port_ = port; }
void Request::setUri(std::string uri) { uri_ = uri; }
void Request::setHttpVersion(std::string http_version) { http_version_ = http_version; }
void Request::setHeader(std::string key, std::string value) { headers_[key] = value; }
void Request::setBufferContentLength(int content_length) { buffer_content_length_ = content_length; }
void Request::setEntityBody(std::string entity_body) { entity_body_ = entity_body; }
void Request::appendEntityBody(char *buffer) { entity_body_.append(buffer); }
void Request::appendEntityBody(char *buffer, size_t size) { entity_body_.append(buffer, size); }

}  // namespace ft
