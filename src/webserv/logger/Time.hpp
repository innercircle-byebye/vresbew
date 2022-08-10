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

  // 형태 : Tue, 25 May 2021 12:29:05 GMT
  static std::string  getCurrentDate() {
    struct timeval		tv;
		time_t				sec;
		std::stringstream	ss;
		char				buf[26];

		gettimeofday(&tv, NULL);
		sec = tv.tv_sec;
		strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S", localtime(&sec));
		ss << buf << " GMT";
		return ss.str();
  }

  // 형태 04-Aug-2021 09:52
  static std::string  getFileModifiedTime(time_t sec) {
		std::stringstream	ss;
    char				buf[18];
    
    strftime(buf, sizeof(buf), "%d-%b-%Y %H:%M", localtime(&sec));
    ss << buf;
    return ss.str();
  }
};
}
#endif
