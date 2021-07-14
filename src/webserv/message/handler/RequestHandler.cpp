#include "webserv/message/handler/RequestHandler.hpp"

namespace ft {
RequestHandler::RequestHandler(Request &request)
    : request_(request){}

void RequestHandler::setRequest(std::string msg_header_buffer) {
  std::istringstream is(msg_header_buffer);
  std::string line;
  if (getline(is, line)) {
    if (line.find('\r') != std::string::npos)
      line.erase(line.size()-1, 1);
    if (this->parseStartLine(line) != 0)  // TODO: 반환값(0) 확인 필요
      this->clearRequest();
  }
  while (getline(is, line)) {
    if (line[0] == '\r')  // TODO: 여기서 문제가 발생할 수 있는지 확인 필요
      break;
    if (this->parseHeaderLine(line) != 0)  // TODO: 반환값(0) 확인 필요
      this->clearRequest();
  }
}

int RequestHandler::parseStartLine(std::string const &start_line) {
  this->delimiter_count_ = this->getCountOfDelimiter(start_line,
                                                     START_LINE_DELIMITER);
  if (this->delimiter_count_ != 2) {
    return (404);
  }
  for (int idx = 0; idx < this->delimiter_count_ + 1; ++idx) {
    int firstDelimiter = start_line.find(START_LINE_DELIMITER);
    int secondDelimiter = start_line.find_last_of(START_LINE_DELIMITER);
    switch (idx) {
      case RQ_METHOD:
        this->request_.method = start_line.substr(0, firstDelimiter);
        // if (!(this->isValidStartLine(idx, this->request_.method)))
        //   return (404);
        break;
      case RQ_URI:
        this->request_.uri = start_line.substr(
            firstDelimiter + 1, (secondDelimiter - 1) - firstDelimiter);
        // if (!(this->isValidStartLine(idx, this->request_.uri)))
        //   return (404);
        break;
      case RQ_VERSION:
        this->request_.version = start_line.substr(
            secondDelimiter + 1, (start_line.size() - 1) - secondDelimiter);
        // if (!(this->isValidStartLine(idx, this->request_.version)))
        //   return (404);
        break;
      default:
        break;
    }
  }
  return (0);  // TODO: 성공했을 때 반환할 값 정의
}

int RequestHandler::getCountOfDelimiter(std::string const &str, char delimiter) const {
  int ret = 0;

  for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
    if ((*it) == delimiter) ++ret;
  }
  return (ret);
}

void RequestHandler::clearRequest(void) {
  this->request_.headers.clear();
  this->request_.uri.clear();
  this->request_.version.clear();
  if (this->request_.headers.size() > 0)
    this->request_.headers.clear();
}

int RequestHandler::parseHeaderLine(std::string const &header_line) {

  std::string key;
  std::string value;
  int delimiter = header_line.find(HEADER_DELIMITER);

  // parse key and validation
  key = header_line.substr(0, delimiter);

  // if (!(this->isValidHeaderKey(key)))
  //   return (404);
  if (header_line.find('\r') != std::string::npos) {
    value = header_line.substr(delimiter + 2,
                               header_line.size() - (delimiter + 3));
  } else
    value = header_line.substr(delimiter + 2,
                               header_line.size() - (delimiter + 2));

  // insert header
  this->request_.headers[key] = value;
  return (0);
}

bool RequestHandler::isValidHeaderKey(std::string const &key) {
  if (!(key == "Accept-Charset" ||
        key == "Accept-Encoding" ||  // TODO: 유효한 헤더인지 확인
        key == "Accept-Language" ||
        key == "Authorization" ||
        key == "Connection" ||  // TODO: 유효한 헤더인지 확인
        key == "Content-Language" ||
        key == "Content-Length" ||
        key == "Content-Type" ||
        key == "Host" ||
        key == "Referer" ||
        key == "User-Agent"))
    return (false);
  return (true);
}

bool RequestHandler::isValidStartLine(int item_ident,
                                      std::string const &item) {
  switch (item_ident) {
    case (RQ_METHOD):
      if (!(item == "GET" || item == "POST" || item == "DELETE" ||
            item == "PUT" || item == "HEAD"))
        return (false);
      break;

    case (RQ_URI):
      // TODO: URI 유효성 검사
      break;

    case (RQ_VERSION):
      if (!(item == "HTTP/1.0" || item == "HTTP/1.1"))
        return (false);
      break;
    default:
      break;
  }
  return (true);
}

}  // namespace ft
