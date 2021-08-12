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

  if ((pos = request_->getMsg().find(CRLF)) != std::string::npos)
    c->setRecvPhase(MESSAGE_START_LINE_COMPLETE);
}

void RequestHandler::checkMsgForHeader(Connection *c) {
  size_t pos;

  // 일단 주석
  // std::string temp_rn_ctrlc = CRLF;
  // temp_rn_ctrlc += ctrl_c[0];
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

  if ((c->status_code_ = (start_line_split.size() == 3) ? -1 : 400) > 0) {
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }
  if ((c->status_code_ = (RequestHandler::isValidMethod(start_line_split[0])) ? -1 : 400) > 0) {
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  request_->setMethod(start_line_split[0]);
  request_->setUri(start_line_split[1]);
  start_line_split[1].append(" ");

  if ((c->status_code_ = (parseUri(start_line_split[1]) == PARSE_VALID_URI) ? -1 : 400) > 0) {
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }

  if ((c->status_code_ = checkHttpVersionErrorCode(start_line_split[2])) > 0) {
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
  std::string header_lines = request_->getMsg().substr(0, pos + 2);
  request_->getMsg().erase(0, pos + CRLFCRLF_LEN);

  while ((pos = header_lines.find(CRLF)) != std::string::npos) {
    std::string one_header_line = header_lines.substr(0, pos);
    if ((c->status_code_ = this->parseHeaderLine(one_header_line)) > 0) {
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
      return;
    }
    header_lines.erase(0, pos + CRLF_LEN);
  }
}

// 실패 시 c->status_code_에 에러 코드가 발생 하도록
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
  return (-1);
}

/* UTILS */

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
    return (-1);
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

bool RequestHandler::isHostHeaderExist() {
  if (request_->getHttpVersion().compare("HTTP/1.1") == 0 &&
      !request_->getHeaderValue("Host").empty()) {
    return (true);
  } else if (request_->getHttpVersion().compare("HTTP/1.0") == 0) {
    return (true);
  }
  return (false);
}

bool RequestHandler::isUriFileExist(LocationConfig *location) {
  (void)location;
  struct stat stat_buffer_;

  if (stat(request_->getFilePath().c_str(), &stat_buffer_) < 0) {
    return (false);
  }
  return (true);
}

bool RequestHandler::isUriDirectory(LocationConfig *location) {
  (void)location;
  struct stat stat_buffer_;

  stat(request_->getFilePath().c_str(), &stat_buffer_);
  if (S_ISDIR(stat_buffer_.st_mode))
    return (true);
  return (false);
}

bool RequestHandler::isAllowedMethod(LocationConfig *location) {
  return (location->checkAcceptedMethod(request_->getMethod()));
}

void RequestHandler::applyReturnDirectiveStatusCode(Connection *c, LocationConfig *location) {
  if (location->getReturnCode() == 301 || location->getReturnCode() == 302 ||
      location->getReturnCode() == 303 || location->getReturnCode() == 307 ||
      location->getReturnCode() == 308) {
    c->status_code_ = location->getReturnCode();
    if (!location->getReturnValue().empty())
      c->getResponse().setHeader("Location", location->getReturnValue());
    else
      c->getResponse().setHeader("Location", " ");
    return;
  }
  if (!location->getReturnValue().empty()) {
    c->setBodyBuf(location->getReturnValue());
  }
  c->status_code_ = location->getReturnCode();
}

void RequestHandler::handleChunked(Connection *c) {
  size_t pos;

  while ((pos = request_->getMsg().find(CRLF)) != std::string::npos) {
    if (c->chunked_checker_ == STR_SIZE) {
      if ((pos = request_->getMsg().find(CRLF)) != std::string::npos) {
        if (c->client_max_body_size < c->getBodyBuf().length()) {
          c->getBodyBuf().clear();
          c->status_code_ = 413;
          c->setRecvPhase(MESSAGE_BODY_COMPLETE);
          c->is_chunked_ = false;
          return;
        }
        c->chunked_str_size_ = (size_t)strtoul(request_->getMsg().substr(0, pos).c_str(), NULL, 16);
        if (c->chunked_str_size_ == 0) {
          for (size_t i = 0; i < pos; ++i) {
            if (request_->getMsg()[i] != '0') {
              c->getBodyBuf().clear();
              c->status_code_ = 400;
              c->setRecvPhase(MESSAGE_BODY_COMPLETE);
              c->is_chunked_ = false;
              return;
            }
          }
        }
        request_->getMsg().erase(0, pos + 2);
        if (c->chunked_str_size_ == 0)
          c->chunked_checker_ = END;
        else
          c->chunked_checker_ = STR;
      }
    }
    if (c->chunked_checker_ == STR) {
      // 조건문 확인 부탁드립니다
      if (request_->getMsg().size() >= (c->chunked_str_size_ + 2) && !request_->getMsg().substr(c->chunked_str_size_, 2).compare(CRLF)) {
        c->appendBodyBuf((char *)request_->getMsg().c_str(), c->chunked_str_size_);
        request_->getMsg().erase(0, c->chunked_str_size_ + 2);
        c->chunked_checker_ = STR_SIZE;
      }
      if (request_->getMsg().size() >= c->chunked_str_size_ + 4) {
        c->getBodyBuf().clear();
        c->status_code_ = 400;
        c->setRecvPhase(MESSAGE_BODY_COMPLETE);
        return;
      }
    }
    if (c->chunked_checker_ == END) {
      if ((pos = request_->getMsg().find(CRLF)) == 0) {
        request_->getMsg().clear();
        if (c->status_code_ > 0) {
          c->setRecvPhase(MESSAGE_START_LINE_INCOMPLETE);
          c->status_code_ = -1;
        } else
          c->setRecvPhase(MESSAGE_BODY_COMPLETE);

        c->is_chunked_ = false;
        c->chunked_checker_ = STR_SIZE;
      } else if (pos != std::string::npos)
        request_->getMsg().erase(0, pos + CRLF_LEN);
    }
  }
}

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

void RequestHandler::findIndexForGetWhenOnlySlash(LocationConfig *&location) {
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

}  // namespace ft
