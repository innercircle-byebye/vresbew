#ifndef KQUEUE_HPP
#define KQUEUE_HPP

#include "webserv/webserv.hpp"
#include "webserv/socket/SocketManager.hpp"
#include "webserv/socket/Connection.hpp"
#include "webserv/message/handler/MessageHandler.hpp"

namespace ft {
class Kqueue {
private:
	int				kq_, max_changes_, nchanges_, nevents_;
	struct kevent	*change_list_;
	struct kevent	*event_list_;
	struct timespec	ts_;

public:
	Kqueue();
	~Kqueue();

	void	kqueueSetEvent(Connection *c, u_short filter, u_int flags);
	void	kqueueProcessEvents(SocketManager *sm);
};
}
#endif
