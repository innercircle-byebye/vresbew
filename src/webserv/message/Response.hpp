#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <iostream>
#include <map>
#include <string>
#include <cstdlib> //itoa
#include "webserv/config/LocationConfig.hpp"
#include "webserv/webserv.hpp"

namespace ft {
class Response {
 private:
  // status line
  int status_code_;
  std::string status_message_;
  std::string http_version_;

  std::map<std::string, std::string> headers_;

  std::string header_msg_;

  void initHeaders();

 public:
  Response();
  ~Response();

  void clear();

  const int &getStatusCode() const;
  const std::string &getStatusMessage() const;
  const std::string &getHttpVersion() const;
  const std::map<std::string, std::string> &getHeaders() const;
  const std::string &getHeaderValue(const std::string &key);
  std::string &getHeaderMsg();

  void setStatusCode(int status_code);
  void setStatusMessage(std::string status_message);
  void setHttpVersion(std::string http_version);
  void setHeader(std::string key, std::string value);

  void setConnectionHeaderByStatusCode(int status_code);
};
}  // namespace ft
#endif
