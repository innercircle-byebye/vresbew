#include "StatusMessage.hpp"

namespace ft {

const std::map<unsigned int, std::string> StatusMessage::status_messages_ = StatusMessage::makeStatusMessages();

std::map<unsigned int, std::string> StatusMessage::makeStatusMessages(void) {
  std::map<unsigned int, std::string> messages;
  messages[200] = "OK";
  messages[201] = "Created";
  messages[204] = "No Content";
  messages[301] = "Moved Permanently";
  messages[400] = "Bad Request";
  messages[401] = "Unauthorized";
  messages[403] = "Forbidden";
  messages[404] = "Not Found";
  messages[405] = "Not Allowed";
  messages[413] = "Request Entity Too Large";
  messages[500] = "Internal Server Error";
  messages[501] = "Not Implemented";
  messages[503] = "Service Unavailable";
  messages[999] = "POST WIP";
  return messages;
}

StatusMessage::StatusMessage(void) {}

const std::string &StatusMessage::of(unsigned int status_code) {
  return (status_messages_.find(status_code))->second;
}

}  // namespace ft
