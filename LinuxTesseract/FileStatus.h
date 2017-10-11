#pragma once
#include <string>
#include <boost/date_time.hpp>

struct FileStatus
{
	std::string name;
	std::string output;
	boost::posix_time::ptime start;
	boost::posix_time::ptime end;
	int thread;
	boost::posix_time::ptime::time_duration_type ellapsed;

	FileStatus(std::string name) : name(name) {}
};
