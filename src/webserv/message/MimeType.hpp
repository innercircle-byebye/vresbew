#ifndef MIME_TYPE_HPP
#define MIME_TYPE_HPP

#include <map>
#include <string>

namespace ft {

class MimeType {
 private:
  static const std::map<std::string, std::string> mime_types_;

  static std::map<std::string, std::string> makeMimeTypes(void);
  MimeType(void);

 public:
  static const std::string &of(std::string &extension);
};

}  // namespace ft

#endif
