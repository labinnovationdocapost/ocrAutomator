#pragma once
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/date_time.hpp>
#include <boost/uuid/uuid.hpp>
#include "Buffer/MemoryFileBuffer.h"

#define MAX_RETRY 3

struct BaseFileStatus
{
	boost::uuids::uuid uuid;
	bool isEnd = false;
	boost::posix_time::ptime start;
	boost::posix_time::ptime end;
	int thread = -1;
	int retry = 0;
	bool abandoned = false;
	boost::posix_time::ptime::time_duration_type ellapsed;
	// Position of the file in it's conteneur
	Docapost::IA::Tesseract::MemoryFileBuffer* buffer = nullptr;
	/**
	* \brief Contain the result as text. Should be a pointer to be deletable after use
	*/
	std::unique_ptr<std::vector<std::string>> result;

	BaseFileStatus() : thread(0) {}
	explicit BaseFileStatus(std::string uuid) : thread(0) { this->uuid = boost::lexical_cast<boost::uuids::uuid>(uuid); }
	explicit BaseFileStatus(boost::uuids::uuid uuid) : uuid(uuid), thread(0) {}
	explicit BaseFileStatus(std::string uuid, Docapost::IA::Tesseract::MemoryFileBuffer* data) : thread(0), buffer(data) { this->uuid = boost::lexical_cast<boost::uuids::uuid>(uuid); }
	explicit BaseFileStatus(boost::uuids::uuid uuid, Docapost::IA::Tesseract::MemoryFileBuffer* data) : uuid(uuid), thread(0), buffer(data) {}
};

