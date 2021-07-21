#ifndef RESPONSE_HANDLER_HPP
#define RESPONSE_HANDLER_HPP

#include <dirent.h>  // opendir()
#include <stdio.h>   // remove()
#include <sys/stat.h>
#include <unistd.h>  // rmdir(2)

#include <fstream>
#include <iostream>
#include <istream>
#include <sstream>  // std::istringstream
#include <string>

#include "webserv/config/HttpConfig.hpp"
#include "webserv/logger/Time.hpp"
#include "webserv/message/Request.hpp"
#include "webserv/message/Response.hpp"
#include "webserv/message/StatusMessage.hpp"

namespace ft {

class ResponseHandler {
  Response *response_;
  ServerConfig *server_config_;

  struct stat stat_buffer_;
  // 함수 내 지역변수로 옮겼습니다. 향후 지워주세요!!
  // struct dirent *entry_; // TODO: 지역변수로 전환 검토
  // DIR *dir_; // TODO: 지역변수로 전환 검토

  // stat_buffer_ 는 request내에서 하나의 url에 한번만 필요하다고 판단하여
  // 멤버변수로 한번만 등후 클래스 내에서 해당 url에 대한 파일/디렉토리의 정보를 조회하기 위해
  // 멤버변수로 설정 하여 놓았습니다.
  // 단순히 하나의 변수를 담아놓는 '공간'으로 사용한다면
  // 클래스의 멤버변수에 위치하는것이 나을 것 같습니다.
  // (= 작성하신 deletePathRecursive 함수를 살펴 보았을 때 문제 없는것 같습니다.)

 public:
  ResponseHandler();
  ~ResponseHandler();

  void setResponse(Response *response);
  void setServerConfig(HttpConfig *http_config, struct sockaddr_in &addr, const std::string &host);
  void setResponseFields(const std::string &method, std::string &uri);
  void makeResponseMsg();
  void setStatusLineWithCode(const std::string &status_code);

  std::string getAccessPath(std::string uri);

 private:
  // Response::response_ setter begin
  void setResponseStatusLine();
  void setResponseHeader();
  void setResponseBody();
  // Response::response_ setter end

  // making response message begin
  // void setStatusLineWithCode(const std::string &status_code);
  std::string getDefaultErrorBody(std::string status_code, std::string status_message);
  // making response message end

  // blocks for setResponseFields begin
  void processGetAndHeaderMethod(const std::string *uri, LocationConfig *location);
  void processPutMethod();
  void processDeleteMethod();
  void processPostMethod();
  // blocks for setResponseFields end



  // 애매함
  void setResponseBodyFromFile(std::string &uri);
  // 애매함 end

  // executing methods helper begin
  bool isFileExist(std::string &uri);
  bool isPathAccessable(std::string path, std::string &uri);
  int deletePathRecursive(std::string &path);
  // executing methods helper end

};
}  // namespace ft
#endif
