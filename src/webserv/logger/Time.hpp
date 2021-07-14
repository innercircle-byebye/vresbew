#ifndef TIME_HPP
#define TIME_HPP

#include <iostream>
#include <sstream>
#include <sys/time.h>

namespace ft {
class Time {
public:
	static	std::string		get_current_time()
	{
		struct timeval		tv;
		time_t				sec;
		std::stringstream	ss;
		char				buf[20];

		gettimeofday(&tv, NULL);
		sec = tv.tv_sec;
		strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", localtime(&sec));
		ss << buf;
		return ss.str();
	}

};
}
#endif
