#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <exception>

namespace ft {
class WebservCycleException : public std::exception {
  virtual const char *what() const throw() {
    return ("Webserv cycle error happen.");
  }
};

class KqueueException : public std::exception {
  virtual const char *what() const throw() {
    return ("kqueue() error.");
  }
};

class KeventException : public std::exception {
  virtual const char *what() const throw() {
    return ("kqueue() error.");
  }
};

class SocketException : public std::exception {
  virtual const char *what() const throw() {
    return ("socket() error.");
  }
};

class BindException : public std::exception {
  virtual const char *what() const throw() {
    return ("bind() error.");
  }
};

class ListenException : public std::exception {
  virtual const char *what() const throw() {
    return ("listen() error.");
  }
};

class NonblockingException : public std::exception {
  virtual const char *what() const throw() {
    return ("nonblocking() error.");
  }
};

class AcceptExcception : public std::exception {
  virtual const char *what() const throw() {
    return ("accept() error.");
  }
};

class ConnNotEnoughException : public std::exception {
  virtual const char *what() const throw() {
    return ("connection not enough error.");
  }
};

class CloseSocketException : public std::exception {
  virtual const char *what() const throw() {
    return ("close socket error.");
  }
};

class FileOpenException : public std::exception {
  virtual const char *what() const throw() {
    return ("file open error.");
  }
};
}  // namespace ft
#endif
