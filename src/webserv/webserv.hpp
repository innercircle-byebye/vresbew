#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include "webserv/Exceptions.hpp"

typedef int socket_t;

#define DEFAULT_CONNECTIONS 512
#define LISTEN_BACKLOG 511
#define BUF_SIZE 30000
#define nonblocking(s) fcntl(s, F_SETFL, O_NONBLOCK)
#define closeSocket close

#define WEBSERV_CONTINUE 1
#define WEBSERV_OK 0
#define WEBSERV_ERROR -1

#define LOG_STDERR 0
#define LOG_EMERG 1
#define LOG_ALERT 2
#define LOG_CRIT 3
#define LOG_ERR 4
#define LOG_WARN 5
#define LOG_NOTICE 6
#define LOG_INFO 7
#define LOG_DEBUG 8

#define CTRL_C_LIST 4

#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

const char ctrl_c[CTRL_C_LIST] = {0xff, 0xf4, 0xfd, 0x06};

namespace ft {
class Kqueue;
class SocketManager;
class Listening;
class Connection;

const std::string err_levels[9] = {"", "emerge", "alert", "crit", "error", "warn", "notice", "info", "debug"};
}  // namespace ft
#endif
