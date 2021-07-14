#ifndef SOCKETMANAGER_HPP
#define SOCKETMANAGER_HPP

#include "webserv/config/HttpConfig.hpp"
#include "webserv/logger/Logger.hpp"
#include "webserv/socket/Connection.hpp"
#include "webserv/socket/Listening.hpp"
#include "webserv/webserv.hpp"
namespace ft {

class SocketManager {
 private:
  std::vector<Listening *> listening_;

  Connection *connections_;
  size_t connection_n_;
  Connection *free_connections_;
  size_t free_connection_n_;

  void openListeningSockets();
  void closeListeningSockets();

 public:
  SocketManager(HttpConfig *httpconfig);
  ~SocketManager();

  Connection *getConnection(socket_t s);
  void freeConnection(Connection *c);
  void closeConnection(Connection *c);

  const std::vector<Listening *> &getListening() const;
  size_t getListeningSize() const;
};
}  // namespace ft
#endif
