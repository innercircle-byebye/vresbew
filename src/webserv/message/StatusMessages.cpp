#include "StatusMessages.hpp"

ft::StatusMessages::StatusMessages() {
  this->initStatusMessages();
}

ft::StatusMessages::~StatusMessages() {}

void ft::StatusMessages::initStatusMessages() {
  this->status_messages_[200] = "200 OK";
  this->status_messages_[201] = "201 Created";
  this->status_messages_[204] = "204 No Content";
  this->status_messages_[400] = "400 Bad Request";
  this->status_messages_[401] = "401 Unauthorized";
  this->status_messages_[404] = "404 Not Found";
  this->status_messages_[405] = "405 Method Not Allowed";
  this->status_messages_[413] = "413 Request Entity Too Large";
  this->status_messages_[500] = "500 Internal Server Error";
  this->status_messages_[501] = "501 Not Implemented";
  this->status_messages_[503] = "503 Service Unavailable";
}

const char *ft::StatusMessages::getStatusMessage(
    unsigned int status_code) const {
  return ((this->status_messages_.find(status_code))->second);
}
