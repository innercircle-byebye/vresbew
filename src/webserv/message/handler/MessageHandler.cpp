#include "webserv/message/handler/MessageHandler.hpp"

namespace ft {

RequestHandler MessageHandler::request_handler_ = RequestHandler();
ResponseHandler MessageHandler::response_handler_ = ResponseHandler();

MessageHandler::MessageHandler() {}
MessageHandler::~MessageHandler() {}

void MessageHandler::handleRequestHeader(Connection *c) {
  if (c->getRecvPhase() == MESSAGE_START_LINE_INCOMPLETE && !std::string(c->buffer_).compare(CRLF))
    return;
  // 1. request_handler의 request가 c의 request가 되도록 세팅
  request_handler_.setRequest(&c->getRequest());
  // 2. append (이전에 request가 setting되어야함)
  request_handler_.appendMsg(c->buffer_);
  // 3. process by recv_phase
  request_handler_.processByRecvPhase(c);
}

void MessageHandler::handleRequestBody(Connection *c) {
  if (c->is_chunked_ == false) {
    if ((size_t)c->getStringBufferContentLength() <= strlen(c->buffer_)) {
      c->appendBodyBuf(c->buffer_, c->getStringBufferContentLength());
      c->setStringBufferContentLength(-1);
      c->setRecvPhase(MESSAGE_BODY_COMPLETE);
    } else {
      c->setStringBufferContentLength(c->getStringBufferContentLength() - strlen(c->buffer_));
      c->setBodyBuf(c->buffer_);
    }
  } else
    c->setRecvPhase(MESSAGE_BODY_COMPLETE);
}

bool MessageHandler::handleChunked(Connection *c) {
  size_t pos;

  while ((pos = c->getRequest().getMsg().find(CRLF)) != std::string::npos) {
    if (c->chunked_checker_ == STR_SIZE) {
      if ((pos = c->getRequest().getMsg().find(CRLF)) != std::string::npos) {
        if (c->client_max_body_size < c->getBodyBuf().length()) {
          c->getBodyBuf().clear();
          c->req_status_code_ = 413;
          c->setRecvPhase(MESSAGE_BODY_COMPLETE);
          c->is_chunked_ = false;
          return true;
        }
        c->chunked_str_size_ = (size_t)strtoul(c->getRequest().getMsg().substr(0, pos).c_str(), NULL, 16);
        if (c->req_status_code_ != NOT_SET && c->chunked_str_size_ != 0)
          return false;
        if (c->chunked_str_size_ == 0) {
          for (size_t i = 0; i < pos; ++i) {
            if (c->getRequest().getMsg()[i] != '0') {
              if (c->req_status_code_ == NOT_SET) {
                c->getBodyBuf().clear();
                c->req_status_code_ = 400;
                c->setRecvPhase(MESSAGE_BODY_COMPLETE);
                c->is_chunked_ = false;
                return true;
              }
              else
                return false;
            }
          }
        }
        c->getRequest().getMsg().erase(0, pos + 2);
        if (c->chunked_str_size_ == 0)
          c->chunked_checker_ = END;
        else
          c->chunked_checker_ = STR;
      }
    }
    if (c->chunked_checker_ == STR) {
      // 조건문 확인 부탁드립니다
      if (c->getRequest().getMsg().size() >= (c->chunked_str_size_ + 2) && !c->getRequest().getMsg().compare(c->chunked_str_size_, 2, CRLF)) {
        c->appendBodyBuf((char *)c->getRequest().getMsg().c_str(), c->chunked_str_size_);
        c->getRequest().getMsg().erase(0, c->chunked_str_size_ + CRLF_LEN);
        c->chunked_checker_ = STR_SIZE;
      }
      if (c->getRequest().getMsg().size() >= c->chunked_str_size_ + 4) {
        c->getBodyBuf().clear();
        c->req_status_code_ = 400;
        c->setRecvPhase(MESSAGE_BODY_COMPLETE);
        return true;
      }
    }
    if (c->chunked_checker_ == END) {
      if ((pos = c->getRequest().getMsg().find(CRLF)) == 0) {
        c->getRequest().getMsg().clear();
        if (c->req_status_code_ != NOT_SET) {
          c->setRecvPhase(MESSAGE_START_LINE_INCOMPLETE);
          c->req_status_code_ = NOT_SET;
        } else
          c->setRecvPhase(MESSAGE_BODY_COMPLETE);

        c->is_chunked_ = false;
        c->chunked_checker_ = STR_SIZE;
      } else if (pos != std::string::npos)
        c->getRequest().getMsg().erase(0, pos + CRLF_LEN);
    }
  }
  return true;
}

void MessageHandler::checkCgiProcess(Connection *c) {
  if (!c->getLocationConfig()->getCgiPath().empty() &&
      c->getLocationConfig()->checkCgiExtension(c->getRequest().getFilePath())) {
    CgiHandler::initCgiChild(c);
  }
}

void MessageHandler::executeServerSide(Connection *c) {
  request_handler_.setRequest(&c->getRequest());

  response_handler_.setResponse(&c->getResponse(), &c->getBodyBuf());
  response_handler_.setLocationConfig(c->getLocationConfig());
  response_handler_.setServerNameHeader();

  if (c->req_status_code_ != NOT_SET) {
    response_handler_.setStatusLineWithCode(c->req_status_code_);
    if (c->getLocationConfig()->getReturnValue().empty())
      c->getBodyBuf().clear();
    return;
  }

  response_handler_.executeMethod(c->getRequest());
}

void MessageHandler::setResponseMessage(Connection *c) {
  if (!(c->getResponse().getStatusCode() == 200 ||
        c->getResponse().getStatusCode() == 201 ||
        c->getResponse().getStatusCode() == 204) &&
      c->getBodyBuf().empty()) {
    response_handler_.setErrorBody(c->getLocationConfig()->getErrorPage());
  }

  response_handler_.setDefaultHeader(c, c->getRequest());
  response_handler_.makeResponseHeader();
}

bool MessageHandler::sendResponseToClient(Connection *c) {
  if (send(c->getFd(), c->getResponse().getHeaderMsg().c_str(), c->getResponse().getHeaderMsg().size(), 0) == -1)
    return false;
  if (c->getRequest().getMethod() != "HEAD") {
    if (send(c->getFd(), c->getBodyBuf().c_str(), c->getBodyBuf().size(), 0) == -1)
      return false;
  }
  return true;
}

}  // namespace ft
