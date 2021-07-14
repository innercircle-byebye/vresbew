#ifndef STATUS_CODE_HPP
#define STATUS_CODE_HPP

#include <map>

namespace ft {
class StatusMessages {
 private:
  std::map<unsigned int, const char *> status_messages_;

 public:
  StatusMessages();
  ~StatusMessages();

  const char *getStatusMessage(unsigned int status_code) const;

 private:
  void initStatusMessages();

  // canonical
  StatusMessages(const StatusMessages &other);
  StatusMessages &operator=(const StatusMessages &other);
};

}  // namespace ft

#endif
