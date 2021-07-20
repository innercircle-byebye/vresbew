#ifndef STATUS_MESSAGE_HPP
#define STATUS_MESSAGE_HPP

#include <map>
#include <string>

namespace ft {

class StatusMessage {
 private:
  static const std::map<unsigned int, std::string> status_messages_;

  static std::map<unsigned int, std::string> makeStatusMessages(void);
  StatusMessage(void);

 public:
  static const std::string &of(unsigned int status_code);
};

}  // namespace ft

#endif
