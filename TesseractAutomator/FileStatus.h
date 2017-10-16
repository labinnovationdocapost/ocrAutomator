#pragma once
#include <string>
#include <boost/date_time.hpp>

struct FileStatus
{
	bool isEnd = false;
	std::string relative_name;
	std::string name;
	std::vector<std::string> output;
	std::vector<std::string> relative_output;
	boost::posix_time::ptime start;
	boost::posix_time::ptime end;
	int thread;
	boost::posix_time::ptime::time_duration_type ellapsed;

	FileStatus(std::string name, std::string relative) : relative_name(relative), name(name), thread(0)
	{
	}
};
