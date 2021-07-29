
#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <map>
#include <string>

namespace ft {

// TODO: getter/setter 반영
// 다른 함수에서 t_uri 구조체 변수 쓰도록 바꾸기..
typedef struct s_uri {

  std::string full_uri;
  std::string schema_;
  std::string host_;
  std::string port_;
  std::string path_;
  std::string query_string;

} t_uri;

struct Request {
 private:
  std::string msg_;
  // start line
  std::string method_;
  std::string schema_;
  std::string host_;
  // std::string host_ip_;
  std::string port_;
  // TODO: og_uri_ 로 변경;
  std::string uri_;
  std::string http_version_;

  t_uri uri_struct_;

  // header
  std::map<std::string, std::string> headers_;

  // TODO: 없앨지 봐야함
  // entity body
  std::string entity_body_;

 public:
  Request();
  ~Request();

  void clear();

  std::string &getMsg();
  const std::string &getMethod() const;
  const std::string &getSchema() const;
  const std::string &getHost() const;
  const std::string &getPort() const;
  std::string &getUri();
  const std::string &getHttpVersion() const;
  const std::map<std::string, std::string> &getHeaders() const;
  const std::string &getHeaderValue(const std::string &key);
  const std::string &getEntityBody() const;
  int getErrorNum() const;


  void setMsg(std::string msg);
  void setMethod(std::string method);
  void setSchema(std::string schema);
  void setHost(std::string host);
  void setPort(std::string port);
  void setUri(std::string uri);
  void setHttpVersion(std::string http_version);
  void setHeader(std::string key, std::string value);
  void setEntityBody(std::string entity_body);
  void appendEntityBody(char *buffer);
  void appendEntityBody(char *buffer, size_t size);
  void setErrorNum(int error_num);
};

}  // namespace ft

#endif
