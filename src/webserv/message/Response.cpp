#include "webserv/message/Response.hpp"

namespace ft {
Response::Response() {
  this->status_code_ = NOT_SET;
  this->http_version_ = "HTTP/1.1";
  this->initHeaders();
}

Response::~Response() {
  this->clear();
}

void Response::clear() {
  this->status_code_ = NOT_SET;
  this->status_message_.clear();

  this->headers_.clear();
  this->initHeaders();

  this->header_msg_.clear();
}

void Response::initHeaders() {
  this->headers_["Allow"] = "";
  this->headers_["Content-Location"] = "";
  this->headers_["Last-Modified"] = "";
  this->headers_["Location"] = "";
  this->headers_["Retry-After"] = "";
  this->headers_["Server"] = "";
  this->headers_["Transfer-Encoding"] = "";
  this->headers_["WWW-Authenticate"] = "";
  this->headers_["Content-Language"] = "";
  this->headers_["Content-Length"] = "";
  this->headers_["Content-Type"] = "";
}

const int &Response::getStatusCode() const { return status_code_; }
const std::string &Response::getStatusMessage() const { return status_message_; }
const std::string &Response::getHttpVersion() const { return http_version_; }
const std::map<std::string, std::string> &Response::getHeaders() const { return headers_; }
const std::string &Response::getHeaderValue(const std::string &key) { return headers_[key]; }
std::string &Response::getHeaderMsg() { return header_msg_; }

void Response::setStatusCode(int status_code) { status_code_ = status_code; }
void Response::setStatusMessage(std::string status_message) { status_message_ = status_message; }
void Response::setHttpVersion(std::string http_version) { http_version_ = http_version; }
void Response::setHeader(std::string key, std::string value) { headers_[key] = value; }

void Response::setConnectionHeaderByStatusCode(int status_code) {
  if (status_code == 200 ||
      status_code == 201 ||
      status_code == 204 ||
      status_code == 302 ||
      status_code == 403 ||
      status_code == 404 ||
      status_code == 405 ||
      status_code == 409 ||
      status_code == 501 ||
      status_code == 503)
    setHeader("Connection", "keep-alive");
  else
    setHeader("Connection", "close");
}

}  // namespace ft
