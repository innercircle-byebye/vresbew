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
  if (request_->getRecvPhase() == MESSAGE_START_LINE_INCOMPLETE)
    checkMsgForStartLine();
  if (request_->getRecvPhase() == MESSAGE_START_LINE_COMPLETE)
    parseStartLine();
  if (request_->getRecvPhase() == MESSAGE_HEADER_INCOMPLETE)
    checkMsgForHeader();
  if (request_->getRecvPhase() == MESSAGE_HEADER_COMPLETE)
    parseHeaderLines();
  if (request_->getRecvPhase() == MESSAGE_BODY_INCOMING)
    checkMsgForEntityBody();
  if (request_->getRecvPhase() == MESSAGE_BODY_COMPLETE)
    parseEntityBody();
}

/* CHECK FUNCTIONS */
void RequestHandler::checkMsgForStartLine() {
  size_t pos;
  if ((pos = request_->getMsg().find("\r\n")) != std::string::npos)
    request_->setRecvPhase(MESSAGE_START_LINE_COMPLETE);
}

void RequestHandler::checkMsgForHeader() {
  // append string first
  // check \r\n\r\n
  size_t pos;
  if ((pos = request_->getMsg().find("\r\n\r\n")) != std::string::npos)
    request_->setRecvPhase(MESSAGE_HEADER_COMPLETE);
}

void RequestHandler::checkMsgForEntityBody() {
  if ((size_t)request_->getContentLength() <= request_->getMsg().size()) {
    request_->setRecvPhase(MESSAGE_BODY_COMPLETE);
  }
}

/* PARSE FUNCTIONS */
int RequestHandler::parseStartLine() {
  // schem://host:port/uri?query

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
  parseUri(start_line.substr(firstDelimiter + 1, secondDelimiter - firstDelimiter));  // 맨뒤에 space 포함시킴
  // request_->setUri(start_line.substr(firstDelimiter + 1, (secondDelimiter - 1) - firstDelimiter));
  // if (!(this->isValidStartLine(idx, this->request_.uri)))
  //   return (404);
  request_->setHttpVersion(start_line.substr(secondDelimiter + 1));
  // if (!(this->isValidStartLine(idx, this->request_.version)))
  //   return (404);
  request_->setRecvPhase(MESSAGE_HEADER_INCOMPLETE);
  return (0);  // TODO: 성공했을 때 반환할 값 정의
}

int RequestHandler::parseUri(std::string uri_str) {
  enum {
    scheme = 0,
    host,
    port,
    uri,
    args,
    uri_complete
  } state;

  if (uri_str[0] == '/')
    state = uri;
  else
    state = scheme;
  size_t pos;
  while (state != uri_complete) {
    switch (state) {
      case scheme:
        if ((pos = uri_str.find("://")) == std::string::npos)
          return PARSE_INVALID_URI;
        request_->setSchema(uri_str.substr(0, pos));
        uri_str.erase(0, pos + 3);
        state = host;
        break ;
      case host:
        if ((pos = uri_str.find_first_of(":/? ")) != std::string::npos) {
          if (pos != 0)
            request_->setHost(uri_str.substr(0, pos));
          uri_str.erase(0, pos);
        }
        else
          return PARSE_INVALID_URI;
        switch (uri_str[0]) {
          case ':':
            state = port;
            break ;
          case '/':
            state = uri;
            break ;
          case '?':
            state = args;
            break ;
          case ' ':
            state = uri_complete;
            break ;
          default:
            return PARSE_INVALID_URI;
        }
        break ;
      case port:
        if ((pos = uri_str.find_first_of("/? ")) != std::string::npos) {
          if (pos != 1)
            request_->setPort(uri_str.substr(1, pos - 1));
          uri_str.erase(0, pos);
        }
        else
          return PARSE_INVALID_URI;
        switch (uri_str[0]) {
          case '/':
            state = uri;
            break ;
          case '?':
            state = args;
            break ;
          case ' ':
            state = uri_complete;
            break ;
          default:
            return PARSE_INVALID_URI;
        }
        break ;
      case uri:
        if ((pos = uri_str.find_first_of("? ")) != std::string::npos) {
          if (pos != 1)
            request_->setUri(uri_str.substr(0, pos));     //  /넣어햐하는지??
          uri_str.erase(0, pos);
        }
        else
          return PARSE_INVALID_URI;
        switch (uri_str[0]) {
          case '?':
            state = args;
            break ;
          case ' ':
            state = uri_complete;
            break ;
          default:
            return PARSE_INVALID_URI;
        }
        break ;
      case args:
        if ((pos = uri_str.find(" ")) != std::string::npos) {
          if (pos != 1)
            request_->setEntityBody(uri_str.substr(1, pos - 1));
          uri_str.erase(0, pos);
        }
        else
          return PARSE_INVALID_URI;
        state = uri_complete;
        break ;
      case uri_complete:
        break ;
    }
  }
  // std::cout << "schema: " << request_->getSchema() << std::endl;
  // std::cout << "host: " << request_->getHost() << std::endl;
  // std::cout << "port: " << request_->getPort() << std::endl;
  // std::cout << "uri: " << request_->getUri() << std::endl;
  // std::cout << "entitybody: |" << request_->getEntityBody() << "|" << std::endl;
  return (PARSE_VALID_URI);
}

void RequestHandler::parseHeaderLines() {
  size_t pos = request_->getMsg().find("\r\n\r\n");
  std::string header_lines = request_->getMsg().substr(0, pos);

  request_->getMsg().erase(0, pos + 4);

  while ((pos = header_lines.find("\r\n")) != std::string::npos) {
    std::string one_header_line = header_lines.substr(0, pos);
    if (this->parseHeaderLine(one_header_line) != 0)  // TODO: 반환값(0) 확인 필요
      request_->clear();
    header_lines.erase(0, pos + 2);
  }
  if (this->parseHeaderLine(header_lines) != 0)  // TODO: 반환값(0) 확인 필요
    request_->clear();

  if (request_->getContentLength() == 0)
    request_->setRecvPhase(MESSAGE_BODY_COMPLETE);
  else
    request_->setRecvPhase(MESSAGE_BODY_INCOMING);
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
  if (!key.compare("Content-Length") && request_->getMethod().compare("GET") && request_->getMethod().compare("DELETE")) {
    try {
      request_->setContentLength(stoi(value));
    } catch (std::exception &e) {
      std::cout << e.what() << std::endl;
    }
  }
  // insert header
  this->request_->setHeader(key, value);
  return (0);
}

void RequestHandler::parseEntityBody() {
  if (request_->getContentLength() > 0)
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
