#include "StatusMessage.hpp"

namespace ft {

const std::map<unsigned int, std::string> StatusMessage::status_messages_ = StatusMessage::makeStatusMessages();

std::map<unsigned int, std::string> StatusMessage::makeStatusMessages(void) {
  std::map<unsigned int, std::string> messages;
  messages[-1] = "";
  messages[200] = "OK";
  messages[201] = "Created";
  messages[204] = "No Content";
  messages[301] = "Moved Permanently";
  messages[302] = "Found";
  messages[400] = "Bad Request";
  messages[401] = "Unauthorized";
  messages[403] = "Forbidden";
  messages[404] = "Not Found";
  messages[405] = "Not Allowed";
  messages[409] = "Conflict";
  messages[413] = "Request Entity Too Large";
  messages[500] = "Internal Server Error";
  messages[501] = "Not Implemented";
  messages[503] = "Service Unavailable";
  messages[505] = "HTTP Version Not Supported";
  return messages;
}

StatusMessage::StatusMessage(void) {}

const std::string &StatusMessage::of(unsigned int status_code) {
  std::map<unsigned int, std::string>::const_iterator it = status_messages_.find(status_code);
	if (it != status_messages_.end())
		return it->second;
	return status_messages_.find(-1)->second;
}

}  // namespace ft
