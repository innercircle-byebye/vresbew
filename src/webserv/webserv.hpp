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
#define BUF_SIZE 32769
#define nonblocking(s) fcntl(s, F_SETFL, O_NONBLOCK)
#define closeSocket close

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

#define CRLF "\r\n"
#define CRLFCRLF "\r\n\r\n"
#define CRLF_LEN 2
#define CRLFCRLF_LEN 4

#define NOT_SET -1

// in_addr_t inet_addr2(const char *cp) { return inet_addr(strcmp(cp, "localhost") == 0 ? "127.0.0.1" : cp); }

// ref:
// https://stackoverflow.com/questions/5590381/easiest-way-to-convert-int-to-string-in-c
// #define SSTR(x) static_cast<std::ostringstream &>(           \
//                     (std::ostringstream() << std::dec << x)) \
//                     .str()

const char ctrl_c[CTRL_C_LIST] = {0xff, 0xf4, 0xfd, 0x06};

namespace ft {
class Kqueue;
class SocketManager;
class Listening;
class Connection;

const std::string err_levels[9] = {"", "emerge", "alert", "crit", "error", "warn", "notice", "info", "debug"};
}  // namespace ft
#endif
