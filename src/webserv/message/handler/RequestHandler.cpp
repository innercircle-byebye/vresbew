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
  if (c->getRecvPhase() == MESSAGE_START_LINE_INCOMPLETE) {
    std::cout << "a" << std::endl;
    checkMsgForStartLine(c);
  }
  if (c->getRecvPhase() == MESSAGE_START_LINE_COMPLETE) {
    std::cout << "b" << std::endl;
    parseStartLine(c);
  }
  if (c->getRecvPhase() == MESSAGE_HEADER_INCOMPLETE) {
    std::cout << "c" << std::endl;

    checkMsgForHeader(c);
  }
  if (c->getRecvPhase() == MESSAGE_HEADER_COMPLETE) {
    std::cout << "d" << std::endl;
    parseHeaderLines(c);
  }
  if (c->getRecvPhase() == MESSAGE_CHUNKED) {
    std::cout << "e" << std::endl;
    handleChunked(c);
  }
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
  // else if (request_->getMsg().find(temp_rn_ctrlc) != request_->getMsg().npos)
  //   c->setRecvPhase(MESSAGE_INTERRUPTED);
  else if (request_->getMsg() == "\r\n")
    c->setRecvPhase(MESSAGE_HEADER_COMPLETE);
}

/* PARSE FUNCTIONS */
void RequestHandler::parseStartLine(Connection *c) {
  // schema://host:port/uri?query
  size_t pos = request_->getMsg().find("\r\n");
  std::string const start_line = request_->getMsg().substr(0, pos);
  request_->getMsg().erase(0, pos + 2);
  std::vector<std::string> start_line_split = RequestHandler::splitByDelimiter(start_line, SPACE);

  if ((c->status_code_ = (start_line_split.size() == 3) ? -1 : 400) > 0) {
    std::cout << "aaa hello world hello world" << std::endl;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }
  if ((c->status_code_ = (RequestHandler::isValidMethod(start_line_split[0])) ? -1 : 400) > 0) {
    std::cout << "bbb hello world hello world" << std::endl;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }
  request_->setMethod(start_line_split[0]);
  request_->setUri(start_line_split[1]);
  start_line_split[1].append(" ");

  if ((c->status_code_ = (parseUri(start_line_split[1]) == PARSE_VALID_URI) ? -1 : 400) > 0) {
    std::cout << "ccc hello world hello world" << std::endl;
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }
  if ((c->status_code_ = checkHttpVersionErrorCode(start_line_split[2])) > 0) {
    std::cout << "ddd hello world hello world" << std::endl;
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
  // std::cout << "uri: " << request_->getUri() << std::endl;
  // std::cout << "schema: " << request_->getSchema() << std::endl;
  // std::cout << "host: " << request_->getHost() << std::endl;
  // std::cout << "port: " << request_->getPort() << std::endl;
  // std::cout << "path: " << request_->getPath() << std::endl;
  // std::cout << "query_string: |" << request_->getQueryString() << "|" << std::endl;
  return (PARSE_VALID_URI);
}

void RequestHandler::parseHeaderLines(Connection *c) {
  if (c->interrupted == true) {
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    return;
  }
  size_t pos = request_->getMsg().find("\r\n\r\n");
  std::string header_lines = request_->getMsg().substr(0, pos + 2);

  request_->getMsg().erase(0, pos + 4);
  if (!request_->getMsg().empty()) {
    c->setBodyBuf(request_->getMsg());
    request_->getMsg().clear();
  }

  while ((pos = header_lines.find("\r\n")) != std::string::npos) {
    std::string one_header_line = header_lines.substr(0, pos);
    if ((c->status_code_ = this->parseHeaderLine(one_header_line)) > 0) {
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
      return;
    }
    header_lines.erase(0, pos + 2);
  }

  if (request_->getMethod().compare("GET") && request_->getMethod().compare("HEAD") &&
      request_->getHeaderValue("Content-Length").empty() && !request_->getHeaderValue("Transfer-Encoding").compare("chunked")) {
    c->setRecvPhase(MESSAGE_CHUNKED);
    c->is_chunked_ = true;
  }
  // else
  //   c->setRecvPhase(MESSAGE_HEADER_COMPLETE);
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
  // parse key and validation
  if (key_and_value[0].at(key_and_value[0].size() - 1) == ' ')
    key_and_value[0] = key_and_value[0].substr(0, key_and_value[0].size() - 1);
  key_and_value[0] = key_and_value[0].substr(0, key_and_value[0].size() - 1);
  key = key_and_value[0];
  value = key_and_value[1];

  if (!key.compare("Host") && !request_->getHost().empty())
    value = request_->getHost();
  // insert header
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

bool RequestHandler::isAllowedMethod(LocationConfig *location) {
  return (location->checkAcceptedMethod(request_->getMethod()));
}

void RequestHandler::applyReturnDirectiveStatusCode(Connection *c, LocationConfig *location) {
  if (location->getReturnCode() == 301 || location->getReturnCode() == 302 ||
      location->getReturnCode() == 303 || location->getReturnCode() == 307 ||
      location->getReturnCode() == 308) {
    c->status_code_ = location->getReturnCode();
    // TODO: 필요한지 안한지 결정하기
    // if (c->status_code_ == 302)
    //   c->getResponse().setStatusMessage("Moved Temporarily");
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
  setRequest(&c->getRequest());
  if (c->chunked_checker_ == STR_SIZE) {
    // std::cout << "111111111111111111111111111111" << std::endl;
    // // std::cout << request_->getMsg() << std::endl;
    // std::cout << "111111111111111111111111111111" << std::endl;

    if ((pos = request_->getMsg().find("\r\n")) != std::string::npos) {
      if ((c->chunked_str_size_ = (size_t)strtoul(request_->getMsg().substr(0, pos).c_str(), NULL, 16)) == 0 &&
          request_->getMsg().substr(0, pos).compare("0")) {
        c->getBodyBuf().clear();
        c->status_code_ = 400;
        c->setRecvPhase(MESSAGE_BODY_COMPLETE);
        return;
      }
      request_->getMsg().erase(0, pos + 2);
      if (c->chunked_str_size_ == 0 || *(request_->getMsg().begin()) == '0')
        c->chunked_checker_ = END;
      else
        c->chunked_checker_ = STR;
    }
  }
  if (c->chunked_checker_ == STR) {
    std::cout << "2222222222222222222222222222222" << std::endl;

    if (request_->getMsg().size() >= (c->chunked_str_size_ + 2) && !request_->getMsg().substr(c->chunked_str_size_, 2).compare("\r\n")) {
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
    std::cout << "3333333333333333333333333333" << std::endl;

    if ((pos = request_->getMsg().find("\r\n")) == 0) {
      std::cout << "world world world hello hello hello" << std::endl;
      request_->getMsg().clear();
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    } else if (pos != std::string::npos)
      request_->getMsg().erase(0, pos + 2);
  }
}

void RequestHandler::setupUriStruct(ServerConfig *server, LocationConfig *location) {
  std::string filepath;
  std::cout << "request_uri: [" << request_->getUri() << "]" << std::endl;
  std::cout << "location_uri: [" << location->getUri() << "]" << std::endl;
  std::cout << "location_root: [" << location->getRoot() << "]" << std::endl;

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
  // struct stat stat_buffer_;

  // std::vector<std::string>::const_iterator it_index;
  // std::string temp;
  // for (it_index = location->getIndex().begin(); it_index != location->getIndex().end(); it_index++) {
  //   temp = request_->getFilePath() + *it_index;
  //   std::cout << "temp: [" << temp << "]" << std::endl;
  //   if (stat(temp.c_str(), &stat_buffer_) < 0) {
  //     std::cout << "yo" << std::endl;

  //     while (1) {
  //       ;
  //     }
  //     break;
  //   }
  //   temp.clear();
  // }

  request_->setFilePath(filepath);
  std::cout << "filepath: [" << request_->getFilePath() << "]" << std::endl;
}

}  // namespace ft
