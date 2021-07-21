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
  if (request_->getRecvPhase() == MESSAGE_BODY_COMPLETE)
    parseEntityBody();
}

/* CHECK FUNCTIONS */
void RequestHandler::checkMsgForHeader() {
  // append string first
  // check \r\n\r\n
  size_t pos;
  if ((pos = request_->getMsg().find("\r\n\r\n")) != std::string::npos) {
    // std::cout << "find header" << std::endl;
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
  // if (request_->getHeaderValue("Content-Length").size() != 0) {
  //   try {
  //     request_->setHeaderValue("header", stoi(this->request_.headers["Content-Length"]));
  //   } catch (const std::exception &e) {
  //     std::cout << "content-length not available" << std::endl;
  //     this->request_.headers.erase("Content-Length");
  //     this->msg_body_buf_.clear();
  //   }
  //   if (this->recv_phase == MESSAGE_BODY_NO_NEED)
  //     std::cout << "no need" << std::endl;
  //   if (this->recv_phase == MESSAGE_HEADER_COMPLETE)
  //     std::cout << "header complete" << std::endl;
  // }
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
    /* HTTP methods: GET, HEAD, POST */
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
      /* first char */
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


}  // namespace ft
