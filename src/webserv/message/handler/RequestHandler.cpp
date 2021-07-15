#include "webserv/message/handler/RequestHandler.hpp"

namespace ft {
RequestHandler::RequestHandler() {}

RequestHandler::~RequestHandler() {}

void RequestHandler::setRequest(Request *request) {
  request_ = request;
}

void RequestHandler::appendMsg(const char *buffer) {
  request_->getMsg().append(buffer);
}

void RequestHandler::processByRecvPhase() {
    if (request_->getRecvPhase() == MESSAGE_HEADER_INCOMPLETE)
      checkMsgForHeader();
    if (request_->getRecvPhase() == MESSAGE_HEADER_COMPLETE) {
      parseStartLine();
      parseHeaderLines(); // 내부에서 body 필요한지 체크한 후 content_length랑 recv_phase 변경
    }
    if (request_->getRecvPhase() == MESSAGE_BODY_NO_NEED)
      return ;
    if (request_->getRecvPhase() == MESSAGE_BODY_INCOMING)
      checkMsgForEntityBody();
    if (request_->getRecvPhase() == MESSAGE_BODY_COMPLETE) {  // content_length 초기화하는 부분도 추가하기
      parseEntityBody();
      // request_->recv_phase = MESSAGE_HEADER_INCOMPLETE;  // ?????
    }
  /*
  switch (request_->getRecvPhase()) {
    case MESSAGE_HEADER_INCOMPLETE:
      checkMsgForHeader();
    case MESSAGE_HEADER_COMPLETE:
      std::cout << MESSAGE_HEADER_COMPLETE << request_->getRecvPhase() << std::endl;
      parseStartLine();
      parseHeaderLines(); // 내부에서 body 필요한지 체크한 후 content_length랑 recv_phase 변경
      std::cout << "parse header lines end" << std::endl;
    case MESSAGE_BODY_NO_NEED:
      break ;
    case MESSAGE_BODY_INCOMING:
      checkMsgForEntityBody();
    case MESSAGE_BODY_COMPLETE: // content_length 초기화하는 부분도 추가하기
      parseEntityBody();
      // request_->recv_phase = MESSAGE_HEADER_INCOMPLETE;  // ?????
  }
  */
}

/* CHECK FUNCTIONS */
void RequestHandler::checkMsgForHeader() {
  // append string first
  // check \r\n\r\n
  size_t pos;
  if ((pos = request_->getMsg().find("\r\n\r\n")) != std::string::npos) {
    std::cout << "find header" << std::endl;
    request_->setRecvPhase(MESSAGE_HEADER_COMPLETE);
  }
}

void RequestHandler::checkMsgForEntityBody() {
  if ((size_t)request_->getContentLength() <= request_->getMsg().size()) {
    request_->setRecvPhase(MESSAGE_BODY_COMPLETE);
  }
}

/* PARSE FUNCTIONS */
int RequestHandler::parseStartLine() {
  size_t pos = request_->getMsg().find("\r\n");
  std::string const start_line = request_->getMsg().substr(0, pos);
  request_->getMsg().erase(0, pos + 2);

  int delimiter_count = this->getCountOfDelimiter(start_line,
                                                    START_LINE_DELIMITER);
  if (delimiter_count != 2) {
    return (404);
  }
  int firstDelimiter = start_line.find(START_LINE_DELIMITER);
  int secondDelimiter = start_line.find_last_of(START_LINE_DELIMITER);

  request_->setMethod(start_line.substr(0, firstDelimiter));
  // if (!(this->isValidStartLine(idx, this->request_.method)))
  //   return (404);
  request_->setUri(start_line.substr(firstDelimiter + 1, (secondDelimiter - 1) - firstDelimiter));
  // if (!(this->isValidStartLine(idx, this->request_.uri)))
  //   return (404);
  request_->setHttpVersion(start_line.substr(secondDelimiter + 1));
  // if (!(this->isValidStartLine(idx, this->request_.version)))
  //   return (404);
  return (0);  // TODO: 성공했을 때 반환할 값 정의
}

void RequestHandler::parseHeaderLines() {
  size_t pos = request_->getMsg().find("\r\n\r\n");
  std::string const header_line = request_->getMsg().substr(0, pos);
  request_->getMsg().erase(0, pos + 4);

  std::istringstream is(header_line);
  std::string line;
  while (getline(is, line, '\r')) { // ????????
    if (this->parseHeaderLine(line) != 0)  // TODO: 반환값(0) 확인 필요
      request_->clear();
  }
  // 추가할 부분!!!
  request_->setRecvPhase(MESSAGE_BODY_NO_NEED);
  // request_->recv_phase = MESSAGE_BODY_INCOMING;
}

int RequestHandler::parseHeaderLine(std::string &one_header_line) {
  std::string key;
  std::string value;
  int delimiter = one_header_line.find(HEADER_DELIMITER);

    // parse key and validation
    key = one_header_line.substr(0, delimiter);
    value = one_header_line.substr(delimiter + 2);
  // if (!(this->isValidHeaderKey(key)))
  //   return (404);

  // insert header
  this->request_->setHeader(key, value);
  return (0);
}

void RequestHandler::parseEntityBody() {
  request_->setEntityBody(request_->getMsg().substr(0, request_->getContentLength()));
}

/* UTILS */
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

bool RequestHandler::isValidRequestMethod() {
  const std::string &method = request_->getMethod();
  if (method.compare("GET") && method.compare("POST") &&
      method.compare("DELETE") && method.compare("PUT") && method.compare("HEAD"))
    return (false);
  return (true);
}

bool RequestHandler::isValidRequestVersion() {
  if (!request_->getHttpVersion().compare("HTTP/1.1")) {
    if (request_->getHeaders().count("Host"))
      return (true);
    return (false);
  } else if (!request_->getHttpVersion().compare("HTTP/1.0")) {
    if (!request_->getHeaders().count("Host"))
      request_->setHeader("Host", "");
    return (true);
  }
  return (false);
}

int RequestHandler::getCountOfDelimiter(std::string const &str, char delimiter) {
  int ret = 0;

  for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
    if ((*it) == delimiter) ++ret;
  }
  return (ret);
}

// void RequestHandler::clearRequest(void) {
//   request_->msg.clear();
//   request_->recv_phase = MESSAGE_HEADER_INCOMPLETE;
//   request_->method.clear();
//   request_->uri.clear();
//   request_->version.clear();
//   request_->headers.clear();
//   request_->content_length = 0;
//   request_->entity_body.clear();
// }

}  // namespace ft
