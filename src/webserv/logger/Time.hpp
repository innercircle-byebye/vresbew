#ifndef TIME_HPP
#define TIME_HPP

#include <iostream>
#include <sstream>
#include <sys/time.h>

namespace ft {
class Time {
public:
	static	std::string		getCurrentTime()
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

  static std::string  getCurrentDate() {
    //TODO: 개선이 필요함
    std::stringstream	ss;
    time_t t;       // t passed as argument in function time()
    struct tm *tt;  // decalring variable for localtime()

    time(&t);       //passing argument to time()
    tt = gmtime(&t);
    // current_time.append(asctime(tt), strlen(asctime(tt)) - 1);
    // current_time.append(" GMT");

    ss << asctime(tt);
    ss << " GMT";
    return ss.str();
  }
};
}
#endif
