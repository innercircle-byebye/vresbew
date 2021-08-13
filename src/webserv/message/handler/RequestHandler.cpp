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
  if (c->getRecvPhase() == MESSAGE_HEADER_COMPLETE) {
    parseHeaderLines(c);
    checkRequestHeader(c);
  }
}

/* CHECK FUNCTIONS */
void RequestHandler::checkMsgForStartLine(Connection *c) {
  size_t pos;

  if ((pos = request_->getMsg().find(CRLF)) != std::string::npos)
    c->setRecvPhase(MESSAGE_START_LINE_COMPLETE);
}

void RequestHandler::checkMsgForHeader(Connection *c) {
  size_t pos;

  if ((pos = request_->getMsg().find(CRLFCRLF)) != std::string::npos)
    c->setRecvPhase(MESSAGE_HEADER_COMPLETE);
}

/* PARSE FUNCTIONS */
void RequestHandler::parseStartLine(Connection *c) {
  // schema://host:port/uri?query
  size_t pos = request_->getMsg().find(CRLF);
  std::string const start_line = request_->getMsg().substr(0, pos);
  request_->getMsg().erase(0, pos + CRLF_LEN);
  std::vector<std::string> start_line_split = RequestHandler::splitByDelimiter(start_line, SPACE);

  if ((c->req_status_code_ = (start_line_split.size() == 3) ? NOT_SET : 400) != NOT_SET) {
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }
  if ((c->req_status_code_ = (RequestHandler::isValidMethod(start_line_split[0])) ? NOT_SET : 400) != NOT_SET) {
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  request_->setMethod(start_line_split[0]);
  request_->setUri(start_line_split[1]);
  start_line_split[1].append(" ");

  if ((c->req_status_code_ = (parseUri(start_line_split[1]) == PARSE_VALID_URI) ? NOT_SET : 400) != NOT_SET) {
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if ((c->req_status_code_ = checkHttpVersionErrorCode(start_line_split[2])) != NOT_SET) {
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  request_->setHttpVersion(start_line_split[2]);
  c->setRecvPhase(MESSAGE_HEADER_INCOMPLETE);
}

// REQUEST_CHECK #3 -- 함수
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
          return (PARSE_INVALID_URI);
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
          return (PARSE_INVALID_URI);
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
            return (PARSE_INVALID_URI);
        }
        break;
      case port:
        if ((pos = uri_str.find_first_of("/? ")) != std::string::npos) {
          for (size_t i = 1; i < pos; ++i) {
            if (!isdigit(uri_str[i]))
              return (PARSE_INVALID_URI);
          }
          if (pos != 1)
            request_->setPort(uri_str.substr(1, pos - 1));
          uri_str.erase(0, pos);
        } else
          return (PARSE_INVALID_URI);
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
            request_->setPath(uri_str.substr(0, pos));
          uri_str.erase(0, pos);
        } else
          return (PARSE_INVALID_URI);
        switch (uri_str[0]) {
          case '?':
            state = args;
            break;
          case ' ':
            state = uri_complete;
            break;
          default:
            return (PARSE_INVALID_URI);
        }
        break;
      case args:
        if ((pos = uri_str.find(" ")) != std::string::npos) {
          if (pos != 1)
            request_->setQueryString(uri_str.substr(1, pos - 1));
          uri_str.erase(0, pos);
        } else
          return (PARSE_INVALID_URI);
        state = uri_complete;
        break;
      case uri_complete:
        break;
    }
  }
  if (request_->getPath().empty())
    request_->setPath("/");

  return (PARSE_VALID_URI);
}

void RequestHandler::parseHeaderLines(Connection *c) {
  size_t pos = request_->getMsg().find(CRLFCRLF);
  std::string header_lines = request_->getMsg().substr(0, pos + CRLF_LEN);
  request_->getMsg().erase(0, pos + CRLFCRLF_LEN);

  while ((pos = header_lines.find(CRLF)) != std::string::npos) {
    std::string one_header_line = header_lines.substr(0, pos);
    if ((c->req_status_code_ = this->parseHeaderLine(one_header_line)) != NOT_SET) {
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
      return;
    }
    header_lines.erase(0, pos + CRLF_LEN);
  }
}

// 실패 시 c->req_status_code_에 에러 코드가 발생 하도록
int RequestHandler::parseHeaderLine(std::string &one_header_line) {
  std::vector<std::string> key_and_value = RequestHandler::splitByDelimiter(one_header_line, SPACE);

  if (key_and_value.size() != 2) {
    key_and_value = RequestHandler::splitByDelimiter(one_header_line, ':');
    if (key_and_value.size() != 2)
      return (400);
  }

  std::string key, value;
  if (key_and_value[0].at(key_and_value[0].size() - 1) == ' ')
    key_and_value[0] = key_and_value[0].substr(0, key_and_value[0].size() - 1);
  key_and_value[0] = key_and_value[0].substr(0, key_and_value[0].size() - 1);
  key = key_and_value[0];
  value = key_and_value[1];

  if (!key.compare("Host") && !request_->getHost().empty())
    value = request_->getHost();
  this->request_->setHeader(key, value);
  return (NOT_SET);
}

void RequestHandler::checkRequestHeader(Connection *c) {
  c->setServerConfig(c->getRequest().getHeaderValue("Host"));
  c->setLocationConfig(c->getRequest().getPath());

  //t_uri uri_struct 전체 셋업하는 부분으로...
  setupUriStruct(c->getServerConfig(), c->getLocationConfig());

  //client_max_body_size 셋업
  c->client_max_body_size = c->getLocationConfig()->getClientMaxBodySize();

  if (!c->getRequest().getHeaderValue("Content-Length").empty()) {
    c->setStringBufferContentLength(stoi(c->getRequest().getHeaderValue("Content-Length")));
    if (!c->getRequest().getMsg().empty())
      c->setBodyBuf(c->getRequest().getMsg());
  } else if (c->getRequest().getMethod().compare("HEAD") && c->getRequest().getMethod().compare("GET") && c->getRequest().getMethod().compare("DELETE"))
    c->is_chunked_ = true;

  if (isHostHeaderExist() == false) {
    c->req_status_code_ = 400;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (c->getLocationConfig()->checkReturn()) {
    applyReturnDirectiveStatusCode(c);
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }
  if (*(c->getRequest().getFilePath().rbegin()) == '/') {
    findIndexForGetWhenOnlySlash(c->getLocationConfig());
    if (*(c->getRequest().getFilePath().rbegin()) == '/' &&
        c->getLocationConfig()->getAutoindex() == false) {
      c->req_status_code_ = 404;
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
      return;
    }
  }

  if (isUriFileExist() == false &&
      c->getRequest().getMethod() != "PUT" && c->getRequest().getMethod() != "POST") {
    c->req_status_code_ = 404;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (isUriDirectory() == true &&
      c->getRequest().getMethod().compare("DELETE") &&
      c->getLocationConfig()->getAutoindex() == false) {
    c->req_status_code_ = 301;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (isAllowedMethod(c->getLocationConfig()) == false) {
    c->req_status_code_ = 405;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if (c->getRequest().getMethod().compare("GET") && c->getRequest().getMethod().compare("HEAD") && c->getRequest().getMethod().compare("DELETE") &&
      c->getRequest().getHeaderValue("Content-Length").empty() && !c->getRequest().getHeaderValue("Transfer-Encoding").compare("chunked")) {
    c->setRecvPhase(MESSAGE_CHUNKED);
    c->is_chunked_ = true;
  } else if (c->getRequest().getMethod() == "GET" || c->getRequest().getMethod() == "DELETE") {
    c->is_chunked_ = false;
    c->getBodyBuf().clear();
    c->setStringBufferContentLength(-1);
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  } else if (c->is_chunked_ == true)
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  else if (c->is_chunked_ == false && c->getStringBufferContentLength() != -1 &&
           (c->getStringBufferContentLength() <= (int)c->getBodyBuf().size()))
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
  else
    c->setRecvPhase(MESSAGE_BODY_INCOMING);
  c->client_max_body_size = c->getLocationConfig()->getClientMaxBodySize();
}

/* UTILS */

void RequestHandler::setupUriStruct(ServerConfig *server, LocationConfig *location) {
  std::string filepath;

  filepath = location->getRoot();
  if (location->getRoot() != server->getRoot()) {
    if (!request_->getPath().substr(location->getUri().length()).empty()) {
      if (*(location->getUri().rbegin()) == '/')
        filepath.append(request_->getPath().substr(location->getUri().length() - 1));
      else
        filepath.append(request_->getPath().substr(location->getUri().length()));
    } else
      filepath.append("/");
  } else {
    filepath.append(request_->getPath());
  }

  request_->setFilePath(filepath);
}

bool RequestHandler::isHostHeaderExist(void) {
  if (request_->getHttpVersion().compare("HTTP/1.1") == 0 &&
      !request_->getHeaderValue("Host").empty()) {
    return (true);
  } else if (request_->getHttpVersion().compare("HTTP/1.0") == 0) {
    return (true);
  }
  return (false);
}

bool RequestHandler::isUriFileExist(void) {
  struct stat stat_buffer_;

  if (stat(request_->getFilePath().c_str(), &stat_buffer_) < 0) {
    return (false);
  }
  return (true);
}

bool RequestHandler::isUriDirectory(void) {
  struct stat stat_buffer_;

  stat(request_->getFilePath().c_str(), &stat_buffer_);
  if (S_ISDIR(stat_buffer_.st_mode))
    return (true);
  return (false);
}

bool RequestHandler::isAllowedMethod(LocationConfig *location) {
  return (location->checkAcceptedMethod(request_->getMethod()));
}

void RequestHandler::applyReturnDirectiveStatusCode(Connection *c) {
  if (c->getLocationConfig()->getReturnCode() == 301 || c->getLocationConfig()->getReturnCode() == 302 ||
      c->getLocationConfig()->getReturnCode() == 303 || c->getLocationConfig()->getReturnCode() == 307 ||
      c->getLocationConfig()->getReturnCode() == 308) {
    c->req_status_code_ = c->getLocationConfig()->getReturnCode();
    if (!c->getLocationConfig()->getReturnValue().empty())
      c->getResponse().setHeader("Location", c->getLocationConfig()->getReturnValue());
    else
      c->getResponse().setHeader("Location", " ");
    return;
  }
  if (!c->getLocationConfig()->getReturnValue().empty()) {
    c->setBodyBuf(c->getLocationConfig()->getReturnValue());
  }
  c->req_status_code_ = c->getLocationConfig()->getReturnCode();
}

void RequestHandler::findIndexForGetWhenOnlySlash(LocationConfig *location) {
  std::vector<std::string>::const_iterator it_index;
  std::string temp;
  for (it_index = location->getIndex().begin(); it_index != location->getIndex().end(); it_index++) {
    temp = request_->getFilePath() + *it_index;
    if (isFileExist(temp)) {
      request_->setFilePath(request_->getFilePath() + *it_index);
      break;
    }
    temp.clear();
  }
}

bool RequestHandler::isFileExist(const std::string &path) {
  struct stat stat_buffer_;

  if (stat(path.c_str(), &stat_buffer_) < 0) {
    return (false);
  }
  return (true);
}

/* STATIC FUNCTIONS */

bool RequestHandler::isValidMethod(std::string const &method) {
  int i;
  i = 0;
  while (method[i]) {
    if (!isalpha(method[i]) && !isupper(method[i]))
      return (false);
    i++;
  }
  return (true);
}

int RequestHandler::checkHttpVersionErrorCode(std::string const &http_version) {
  if (http_version.compare(0, 5, "HTTP/") != 0)
    return (400);  // 400 Bad request
  else if (!http_version.compare(5, 3, "1.1") || !http_version.compare(5, 3, "1.0"))
    return (NOT_SET);
  return (505);
}

std::vector<std::string> RequestHandler::splitByDelimiter(std::string const &str, char delimiter) {
  std::vector<std::string> vc;
  std::stringstream ss(str);
  std::string temp;

  while (getline(ss, temp, delimiter)) {
    if (temp.compare(""))
      vc.push_back(temp);
  }
  return vc;
}

}  // namespace ft
