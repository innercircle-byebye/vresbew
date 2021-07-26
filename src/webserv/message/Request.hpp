
#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <map>
#include <string>

namespace ft {

enum MessageFromBufferStatus {
  MESSAGE_START_LINE_INCOMPLETE = 0,
  MESSAGE_START_LINE_COMPLETE,
  MESSAGE_HEADER_INCOMPLETE,
  MESSAGE_HEADER_COMPLETE,
  MESSAGE_CGI_PROCESS,
  MESSAGE_CGI_INCOMING,
  MESSAGE_CGI_COMPLETE,
  MESSAGE_BODY_INCOMING,
  MESSAGE_BODY_COMPLETE
};

struct Request {

private:
  std::string msg_;  // buffer append시킬 string
  int recv_phase_;   // msg가 얼마나 setting되었는지 알 수 있는 변수
  int buffer_content_length_;


  // start line
  std::string method_;
  std::string schema_;
  std::string host_;
  // std::string host_ip_;
  std::string port_;
  std::string uri_;
  std::string http_version_;

  // header
  std::map<std::string, std::string> headers_;

  // entity body
  std::string entity_body_;
  int error_num_;

public:
  bool is_cgi_process;
  Request();
  ~Request();

  void clear();

  std::string &getMsg();
  int getRecvPhase() const;
  const std::string &getMethod() const;
  const std::string &getSchema() const;
  const std::string &getHost() const;
  // const std::string &getHostIp() const;
  const std::string &getPort() const;
  std::string &getUri();
  const std::string &getHttpVersion() const;
  const std::map<std::string, std::string> &getHeaders() const;
  const std::string &getHeaderValue(const std::string &key);
  int getBufferContentLength() const;
  const std::string &getEntityBody() const;
  int getErrorNum() const;

  void setMsg(std::string msg);
  void setRecvPhase(int recv_phase);
  void setMethod(std::string method);
  void setSchema(std::string schema);
  void setHost(std::string host);
  // void setHostIp(std::string host_ip);
  void setPort(std::string port);
  void setUri(std::string uri);
  void setHttpVersion(std::string http_version);
  void setHeader(std::string key, std::string value);
  void setBufferContentLength(int buffer_content_length);
  void setEntityBody(std::string entity_body);
  void appendEntityBody(std::string entity_body);
  void setErrorNum(int error_num);
};

}  // namespace ft

#endif
