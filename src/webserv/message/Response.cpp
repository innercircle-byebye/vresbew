#include "webserv/message/Response.hpp"

namespace ft {
Response::Response() {
  this->http_version_ = "HTTP/1.1";
  this->initHeaders();
}

Response::~Response() {
  this->clear();
}

void Response::clear() {
  status_code_.clear();
  status_message_.clear();

  headers_.clear();
  this->initHeaders();

  header_msg_.clear();
}

void Response::initHeaders() {
  this->headers_["Allow"] = "";
  this->headers_["Content-Location"] = "";
  this->headers_["Last-Modified"] = "";
  this->headers_["Location"] = "";
  this->headers_["Retry-After"] = "";
  this->headers_["Server"] = "vresbew";
  this->headers_["Transfer-Encoding"] = "";
  this->headers_["WWW-Authenticate"] = "";
  this->headers_["Content-Language"] = "";
  this->headers_["Content-Length"] = "";
  this->headers_["Content-Type"] = "";
}

const std::string &Response::getStatusCode() const { return status_code_; }
const std::string &Response::getStatusMessage() const { return status_message_; }
const std::string &Response::getHttpVersion() const { return http_version_; }
const std::map<std::string, std::string> &Response::getHeaders() const { return headers_; }
const std::string &Response::getHeaderValue(const std::string &key) { return headers_[key]; }
std::string &Response::getHeaderMsg() { return header_msg_; }

void Response::setStatusCode(std::string status_code) { status_code_ = status_code; }
void Response::setStatusMessage(std::string status_message) { status_message_ = status_message; }
void Response::setHttpVersion(std::string http_version) { http_version_ = http_version; }
void Response::setHeader(std::string key, std::string value) { headers_[key] = value; }
void Response::setHeaderMsg(std::string header_msg) { header_msg_ = header_msg; }

void Response::setConnectionHeaderByStatusCode(const std::string &status_code) {
  if (!status_code.compare("403") ||
      !status_code.compare("404") ||
      !status_code.compare("405") ||  // TODO: 재확인 필요
      !status_code.compare("409") ||  // TODO: 재확인 필요
      !status_code.compare("200") ||
      !status_code.compare("201") ||
      !status_code.compare("204"))
    setHeader("Connection", "keep-alive");
  else
    setHeader("Connection", "close");
}

}  // namespace ft
