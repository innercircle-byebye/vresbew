
#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <map>
#include <string>

namespace ft {

// TODO: getter/setter 반영
// 다른 함수에서 t_uri 구조체 변수 쓰도록 바꾸기..
typedef struct s_uri {
  std::string schema_;
  std::string host_;
  std::string port_;
  std::string path_;
  std::string filepath_;
  std::string query_string_;
} t_uri;

// #define LF     (u_char) '\n'
// #define CR     (u_char) '\r'
// #define CRLF   "\r\n"

struct Request {
 private:
  std::string msg_;
  // start line
  std::string method_;
  //TODO: 확인 후 구조체에서 이용하도록..
  std::string port_;
  //og_uri_ 역할
  std::string uri_;
  t_uri uri_struct_;
  std::string http_version_;

  // header
  std::map<std::string, std::string> headers_;

  // TODO: t_uri 구조체가 있으면 필요 없음 (삭제 해도 됨)
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
  const std::string &getPath() const;
  const std::string &getFilePath() const;
  const std::string &getQueryString() const;
  const std::string &getUri();
  const std::string &getHttpVersion() const;
  const std::map<std::string, std::string> &getHeaders() const;
  const std::string &getHeaderValue(const std::string &key);

  void setMsg(std::string msg);
  void setMethod(std::string method);
  void setSchema(std::string schema);
  void setHost(std::string host);
  void setPort(std::string port);
  void setPath(std::string path);
  void setFilePath(std::string path);
  void setQueryString(std::string query_string);
  void setUri(std::string uri);
  void setHttpVersion(std::string http_version);
  void setHeader(std::string key, std::string value);
};

}  // namespace ft

#endif
