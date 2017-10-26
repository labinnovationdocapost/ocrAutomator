#pragma once
#include <string>
#include <boost/date_time.hpp>


struct AFileStatus
{
	std::string uuid;
	bool isEnd = false;
	boost::posix_time::ptime start;
	boost::posix_time::ptime end;
	int thread = -1;
	int fileSize = 0;
	boost::posix_time::ptime::time_duration_type ellapsed;

	AFileStatus() : thread(0) {}
	explicit AFileStatus(std::string uuid) : uuid(uuid), thread(0) {}
};


struct FileStatus : AFileStatus
{
	std::string relative_name;
	std::string name;
	std::vector<std::string> output;
	std::vector<std::string> relative_output;
	std::string hostname = "";

	FileStatus(std::string name, std::string relative) : AFileStatus(), relative_name(relative), name(name)
	{
	}
};
struct SlaveFileStatus : AFileStatus
{
	std::vector<unsigned char>* data = nullptr;
	std::string result;

	SlaveFileStatus(std::string uuid, std::vector<unsigned char>* data) : AFileStatus(uuid), data(data)
	{
	}
};
