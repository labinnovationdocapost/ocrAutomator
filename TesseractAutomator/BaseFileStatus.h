#pragma once
#include <string>
#include <mutex>
#include <boost/date_time.hpp>


struct BaseFileStatus
{
	std::string uuid;
	bool isEnd = false;
	boost::posix_time::ptime start;
	boost::posix_time::ptime end;
	int thread = -1;
	int fileSize = 0;
	boost::posix_time::ptime::time_duration_type ellapsed;
	// Position of the file in it's conteneur
	std::vector<unsigned char>* data = nullptr;

	BaseFileStatus() : thread(0) {}
	explicit BaseFileStatus(std::string uuid) : uuid(uuid), thread(0) {}
	explicit BaseFileStatus(std::string uuid, std::vector<unsigned char>* data) : uuid(uuid), thread(0), data(data) {}
};

