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

void RequestHandler::processByRecvPhase(Connection *c) {
  if (c->getRecvPhase() == MESSAGE_START_LINE_INCOMPLETE)
    checkMsgForStartLine(c);
  if (c->getRecvPhase() == MESSAGE_START_LINE_COMPLETE)
    parseStartLine(c);
  if (c->getRecvPhase() == MESSAGE_HEADER_INCOMPLETE)
    checkMsgForHeader(c);
  if (c->getRecvPhase() == MESSAGE_HEADER_COMPLETE)
    parseHeaderLines(c);
}

/* CHECK FUNCTIONS */
void RequestHandler::checkMsgForStartLine(Connection *c) {
  size_t pos;

  if (c->interrupted == true) {
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  } else if ((pos = request_->getMsg().find("\r\n")) != std::string::npos)
    c->setRecvPhase(MESSAGE_START_LINE_COMPLETE);
}

void RequestHandler::checkMsgForHeader(Connection *c) {
  size_t pos;

  std::string temp_rn_ctrlc = "\r\n";
  temp_rn_ctrlc += ctrl_c[0];
  if ((pos = request_->getMsg().find("\r\n\r\n")) != std::string::npos)
    c->setRecvPhase(MESSAGE_HEADER_COMPLETE);
  else if (request_->getMsg().find(temp_rn_ctrlc) != request_->getMsg().npos)
    c->setRecvPhase(MESSAGE_INTERRUPTED);
  // TODO: header가 안들어온 경우 check
}

/* PARSE FUNCTIONS */
void RequestHandler::parseStartLine(Connection *c) {
  // schema://host:port/uri?query

  size_t pos = request_->getMsg().find("\r\n");
  std::string const start_line = request_->getMsg().substr(0, pos);
  request_->getMsg().erase(0, pos + 2);

  std::vector<std::string> start_line_split = RequestHandler::splitByDelimiter(start_line, SPACE);

  if (start_line_split.size() != 3)  // 404 error
    ;
  if (!RequestHandler::isValidMethod(start_line_split[0]))  // 405 Not allowed
    ;
  request_->setMethod(start_line_split[0]);
  if (parseUri(start_line_split[1] + " ") == PARSE_INVALID_URI) {  // 400 Bad Request
    return;
  }
  if (!RequestHandler::isValidHttpVersion(start_line_split[2]))  // 505 HTTP Version Not Supported
    ;
  request_->setHttpVersion(start_line_split[2]);

  c->setRecvPhase(MESSAGE_HEADER_INCOMPLETE);
}

int RequestHandler::parseUri(std::string uri_str) {
  enum {
    schema = 0,
    host,
    port,
    uri,
    args,
    uri_complete
  } state;

  if (uri_str[0] == '/')
    state = uri;
  else
    state = schema;
  size_t pos;
  while (state != uri_complete) {
    switch (state) {
      case schema:
        if ((pos = uri_str.find("://")) == std::string::npos)
          return PARSE_INVALID_URI;
        request_->setSchema(uri_str.substr(0, pos));
        uri_str.erase(0, pos + 3);
        state = host;
        break;
      case host:
        if ((pos = uri_str.find_first_of(":/? ")) != std::string::npos) {
          if (pos != 0)
            request_->setHost(uri_str.substr(0, pos));
          uri_str.erase(0, pos);
        } else
          return PARSE_INVALID_URI;
        switch (uri_str[0]) {
          case ':':
            state = port;
            break;
          case '/':
            state = uri;
            break;
          case '?':
            state = args;
            break;
          case ' ':
            state = uri_complete;
            break;
          default:
            return PARSE_INVALID_URI;
        }
        break;
      case port:
        if ((pos = uri_str.find_first_of("/? ")) != std::string::npos) {
          if (pos != 1)
            request_->setPort(uri_str.substr(1, pos - 1));
          uri_str.erase(0, pos);
        } else
          return PARSE_INVALID_URI;
        switch (uri_str[0]) {
          case '/':
            state = uri;
            break;
          case '?':
            state = args;
            break;
          case ' ':
            state = uri_complete;
            break;
          default:
            return PARSE_INVALID_URI;
        }
        break;
      case uri:
        if ((pos = uri_str.find_first_of("? ")) != std::string::npos) {
          if (pos != 1)
            request_->setUri(uri_str.substr(0, pos));  //  /넣어햐하는지??
          uri_str.erase(0, pos);
        } else
          return PARSE_INVALID_URI;
        switch (uri_str[0]) {
          case '?':
            state = args;
            break;
          case ' ':
            state = uri_complete;
            break;
          default:
            return PARSE_INVALID_URI;
        }
        break;
      case args:
        if ((pos = uri_str.find(" ")) != std::string::npos) {
          if (pos != 1)
            request_->setEntityBody(uri_str.substr(1, pos - 1));
          uri_str.erase(0, pos);
        } else
          return PARSE_INVALID_URI;
        state = uri_complete;
        break;
      case uri_complete:
        break;
    }
  }
  // std::cout << "schema: " << request_->getSchema() << std::endl;
  // std::cout << "host: " << request_->getHost() << std::endl;
  // std::cout << "port: " << request_->getPort() << std::endl;
  // std::cout << "uri: " << request_->getUri() << std::endl;
  // std::cout << "entitybody: |" << request_->getEntityBody() << "|" << std::endl;
  return (PARSE_VALID_URI);
}

void RequestHandler::parseHeaderLines(Connection *c) {
  if (c->interrupted == true)
  {
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return ;
  }
  size_t pos = request_->getMsg().find("\r\n\r\n");
  std::string header_lines = request_->getMsg().substr(0, pos);

  request_->getMsg().erase(0, pos + 4);
  if (!request_->getMsg().empty()) {
    c->setBodyBuf(request_->getMsg());
    request_->getMsg().clear();
  }

  while ((pos = header_lines.find("\r\n")) != std::string::npos) {
    std::string one_header_line = header_lines.substr(0, pos);
    if (this->parseHeaderLine(one_header_line) != 0)  // TODO: 반환값(0) 확인 필요
      request_->clear();
    header_lines.erase(0, pos + 2);
  }
  if (this->parseHeaderLine(header_lines) != 0)  // TODO: 반환값(0) 확인 필요
    request_->clear();

  c->setRecvPhase(MESSAGE_HEADER_PARSED);
}

int RequestHandler::parseHeaderLine(std::string &one_header_line) {
  std::vector<std::string> key_and_value = RequestHandler::splitByDelimiter(one_header_line, SPACE);
  if (key_and_value.size() != 2)  // 400 Bad Request
    ;
  std::string key, value;

  // parse key and validation
  key = key_and_value[0].erase(key_and_value[0].size() - 1);
  value = key_and_value[1];

  if (!key.compare("Host") && !request_->getHost().empty())
    value = request_->getHost();
  // insert header
  this->request_->setHeader(key, value);
  return (0);
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

bool RequestHandler::isValidMethod(std::string const &method) {
  return (!method.compare("GET") || !method.compare("POST") ||
          !method.compare("DELETE") || !method.compare("PUT") || !method.compare("HEAD"));
}

bool RequestHandler::isValidHttpVersion(std::string const &http_version) {
  return (!http_version.compare("HTTP/1.1") || !http_version.compare("HTTP/1.0"));
}

std::vector<std::string> RequestHandler::splitByDelimiter(std::string const &str, char delimiter) {
  std::vector<std::string> vc;
  std::stringstream ss(str);
  std::string temp;

  while (getline(ss, temp, delimiter)) {
    vc.push_back(temp);
  }
  return vc;
}
}  // namespace ft
