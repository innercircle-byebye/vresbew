#include "Request.hpp"

namespace ft {

Request::Request(): recv_phase_(MESSAGE_HEADER_INCOMPLETE), content_length_(0) {}

Request::~Request() {
  this->clear();
}

void Request::clear() {
  msg_.clear();
  recv_phase_ = MESSAGE_HEADER_INCOMPLETE;
  method_.clear();
  uri_.clear();
  http_version_.clear();
  headers_.clear();
  content_length_ = 0;
  entity_body_.clear();
}

std::string &Request::getMsg() { return msg_; }
int Request::getRecvPhase() const { return recv_phase_; }
const std::string &Request::getMethod() const { return method_; }
std::string &Request::getUri() { return uri_; }
const std::string &Request::getHttpVersion() const { return http_version_; }
const std::map<std::string, std::string> &Request::getHeaders() const { return headers_; }
const std::string &Request::getHeaderValue(const std::string &key) { return headers_[key]; }
int Request::getContentLength() const { return content_length_; }
const std::string &Request::getEntityBody() const { return entity_body_; }

void Request::setMsg(std::string msg) { msg_ = msg; }
void Request::setRecvPhase(int recv_phase) { recv_phase_ = recv_phase; }
void Request::setMethod(std::string method) { method_ = method; }
void Request::setUri(std::string uri) { uri_ = uri; }
void Request::setHttpVersion(std::string http_version) { http_version_ = http_version; }
void Request::setHeader(std::string key, std::string value) { headers_[key] = value; }
void Request::setContentLength(int content_length) { content_length_ = content_length; }
void Request::setEntityBody(std::string entity_body) { entity_body_ = entity_body; }

}  // namespace ft