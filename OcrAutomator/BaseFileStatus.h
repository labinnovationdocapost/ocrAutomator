#pragma once
#include <string>
#include <mutex>
#include <boost/lexical_cast.hpp>
#include <boost/date_time.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>


struct BaseFileStatus
{
	boost::uuids::uuid uuid;
	bool isEnd = false;
	boost::posix_time::ptime start;
	boost::posix_time::ptime end;
	int thread = -1;
	int fileSize = 0;
	boost::posix_time::ptime::time_duration_type ellapsed;
	// Position of the file in it's conteneur
	std::vector<unsigned char>* data = nullptr;

	BaseFileStatus() : thread(0) {}
	explicit BaseFileStatus(std::string uuid) : thread(0) { this->uuid = boost::lexical_cast<boost::uuids::uuid>(uuid); }
	explicit BaseFileStatus(boost::uuids::uuid uuid) : uuid(uuid), thread(0) {}
	explicit BaseFileStatus(std::string uuid, std::vector<unsigned char>* data) : thread(0), data(data) { this->uuid = boost::lexical_cast<boost::uuids::uuid>(uuid); }
	explicit BaseFileStatus(boost::uuids::uuid uuid, std::vector<unsigned char>* data) : uuid(uuid), thread(0), data(data) {}
};

