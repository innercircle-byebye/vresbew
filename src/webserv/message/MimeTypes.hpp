#ifndef MIME_TYPES_HPP
#define MIME_TYPES_HPP

#include <map>

namespace ft {
class MimeTypes {
 private:
  std::map<const char *, const char *> mime_types_;

 public:
  MimeTypes();
  ~MimeTypes();

  const char *getContentType(const char *) const;

 private:
  void initMimeTypes();

  // canonical
  MimeTypes(const MimeTypes &other);
  MimeTypes &operator=(const MimeTypes &other);
};

}  // namespace ft

#endif
