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
  std::string filepath = location->getRoot() + request_->getPath();
  struct stat stat_buffer_;

  if (stat(filepath.c_str(), &stat_buffer_) < 0) {
    return (false);
  }
  return (true);
}

bool RequestHandler::isAllowedMethod(LocationConfig *location) {
  return (location->checkAcceptedMethod(request_->getMethod()));
}


/*
int   parse_request_line()
{
  enum {
        sw_start = 0,
        sw_method,
        sw_spaces_before_uri,
        sw_schema,
        sw_schema_slash,
        sw_schema_slash_slash,
        sw_host_start,
        sw_host,
        sw_host_end,
        sw_host_ip_literal,
        sw_port,
        sw_after_slash_in_uri,
        sw_check_uri,
        sw_uri,
        sw_http_09,
        sw_http_H,
        sw_http_HT,
        sw_http_HTT,
        sw_http_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_spaces_after_digit,
        sw_almost_done
    } state;

  std::string msg = request_->getMsg();
  state = request_->getState();
  for (int i = 0; i < msg.size(); ++i) {
    u_char ch = msg[i];

    switch (state) {
    // HTTP methods: GET, HEAD, POST
    case sw_start:
      request_->request_start_idx_ = i;
      if (ch == CR || ch == LF) {
        break ;
      }
      if ((ch < 'A' || ch > 'Z') && ch != '_' && ch != '-') {
        return HTTP_PARSE_INVALID_METHOD;
      }
      state = sw_method;
      break ;

    case sw_method:
      if (ch == ' ') {
        switch (i - request_->request_start_idx_) {
          case 3:
            if (!msg.substr(0, 3).compare("GET")) {
              request_->setMethod("GET");
              break ;
            }
            if (!msg.substr(0, 3).compare("PUT")) {
              request_->setMethod("PUT");
              break ;
            }
            break ;

          case 4:
            if (!msg.substr(0, 4).compare("POST")) {
              request_->setMethod("POST");
              break ;
            }
            if (!msg.substr(0, 4).compare("HEAD")) {
              request_->setMethod("HEAD");
              break ;
            }
            break ;

          case 6:
            if (!msg.substr(0, 6).compare("DELETE")) {
              request_->setMethod("DELETE");
              break ;
            }
            break ;
        }
        state = sw_space_before_uri;
        break ;
      }
      if ((ch < 'A' || ch > 'Z') && ch != '_' && ch != '-') {
        return NGX_HTTP_PARSE_INVALID_METHOD;
      }
    case sw_space_before_uri:
      if (ch == '/') {
        request_->uri_start_idx_ = i;
        state = sw_after_slash_in_uri;
        break ;
      }
      u_char c = (u_char)(ch | 0x20);
      if (c >= 'a' && c <= 'z') {
        request_->shema_start_idx_ = i;
        state = sw_schema;
        break ;
      }
      switch (ch) {
      case ' ':
        break ;
      default:
        return HTTP_PARSE_INVALID_REQUEST;
      }
      break;
    
    case sw_schema:
      c = (u_char)(ch | 0x20);
      if (c >= 'a' && c <= 'z') {
        break ;
      }
      if ((ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == '.') {
        break ;
      }

      switch (ch) {
      case ':':
        r->shema_end_ind_ = i;
        state = sw_shema_slash;
        break ;
      default:
        return HTTP_PARSE_INVALID_REQUEST;
      }
      break;
    
    case sw_schema_slash:
      switch (ch) {
      case '/':
        state = sw_shema_slash_slash;
        break;
      default:
        return HTTP_PARSE_INVALID_REQUEST;
      }
    
    case sw_host_start:
      r->host_start_ind_ = i;

      if (ch == '[') {
        state = sw_host_ip_literal;
        break ;
      }
      state = sw_host;

    case sw_host:
      c = (u_char) (ch | 0x20);
      if (c >= 'a' && c <= 'z') {
        break;
      }
      if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-') {
        break;
      }
    
    case sw_host_end:

    } // switch (state)
  }

}

int   parse_header_line()
{
  enum {
      sw_start = 0,
      sw_name,
      sw_space_before_value,
      sw_value,
      sw_space_after_value,
      sw_ignore_line,
      sw_almost_done,
      sw_header_almost_done
  } state;

  state = request_->state;
  std::string msg = request_->getMsg();

  for (size_t i = 0; i < msg.size(); ++i) {
    u_char ch = msg[i];

    switch (state) {
      //first char
      case sw_start:
        switch (ch) {
          case CR:
            state = sw_header_almost_done;
            break ;
          case LF:
            goto header_done;
          default:
            state = sw_name;
            if (ch <= 0x20 || ch == 0x7f || ch == ':') {
              return HTTP_PARSE_INVALID_HEADER;
            }
            break ;
        }
        break ;

      case sw_name:
        if (ch == ':') {
          state = sw_space_before_value;
          break ;
        }
        if (ch == CR) {
          state = sw_almost_done;
          break ;
        }
        if (ch == LF) {
          goto done;
        }
        if (ch <= 0x20 || ch == 0x7f) {
          return HTTP_PARSE_INVALID_HEADER;
        }
        break ;

      case sw_space_before_value:
        switch(ch) {
          case ' ':
            break;
          case CR:
            state = sw_almost_done;
            break ;
          case LF:
            goto done;
          case '\0':
            return HTTP_PARSE_INVALID_HEADER;
          default:
            state = sw_value;
            break ;
        }
        break ;

      case sw_value:
        switch (ch) {
          case ' ':
            state = sw_space_after_value;
            break ;
          case CR:
            state = sw_almost_done;
            break ;
          case LF:
            goto done;
          case '\0';
            return HTTP_PARSE_INVALID_HEADER;
        }
        break ;

      case sw_space_after_value:
        switch(ch) {
          case ' ':
            break ;
          case CR:
            state = sw_almost_done;
            break ;
          case LF:
            goto done;
          case '\0':
            return HTTP_PARSE_INVALID_HEADER;
          default:
            state = sw_value;
            break ;
        }
        break ;

      case sw_ignore_line:
        switch (ch) {
          case LF:
            state = sw_start;
            break;
          default:
            break;
        }
        break ;
      
      case sw_almost_done:
        switch(ch) {
          case LF:
            goto done;
          case CR:
            break ;
          default:
            return HTTP_PARSE_INVALID_HEADER;
        }
        break ;
      
      case sw_header_almost_done:
        switch(ch) {
          case LF:
            goto header_done;
          default:
            return HTTP_PARSE_INVALID_HEADER;
        }
    }
  }

  return NGX_AGAIN;

done:
  return NGX_OK;
header_done:
  return HTTP_PARSE_HEADER_DONE;
}
*/

}  // namespace ft
