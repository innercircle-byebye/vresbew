#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <iostream>
#include <map>
#include <string>

namespace ft {
class Response {
 private:
  std::string status_code_;
  std::string status_message_;
  std::string http_version_;

  std::map<std::string, std::string> headers_;

  std::string response_body_;

  std::string msg_;

 public:
  Response();
  ~Response();

  void clear();

  void initHeaders();

  const std::string &getStatusCode() const;
  const std::string &getStatusMessage() const;
  const std::string &getHttpVersion() const;
  const std::map<std::string, std::string> &getHeaders() const;
  const std::string &getHeaderValue(const std::string &key);
  std::string &getResponseBody();
  std::string &getMsg();

  void setStatusCode(std::string status_code);
  void setStatusMessage(std::string status_message);
  void setHttpVersion(std::string http_version);
  void setHeader(std::string key, std::string value);
  void setResponseBody(std::string response_body);
  void setMsg(std::string msg);

  void setConnectionHeaderByStatusCode(const std::string &status_code);


 private:
};
}  // namespace ft
#endif
