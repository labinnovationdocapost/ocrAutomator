#pragma once
#include <string>
#include <mutex>
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
	// Position of the file in it's conteneur
	std::vector<unsigned char>* data = nullptr;

	AFileStatus() : thread(0) {}
	explicit AFileStatus(std::string uuid) : uuid(uuid), thread(0) {}
	explicit AFileStatus(std::string uuid, std::vector<unsigned char>* data) : uuid(uuid), thread(0), data(data) {}
};


struct FileStatus : AFileStatus
{
	std::string relative_name;
	std::string name;
	std::string new_name;
	std::vector<std::string> output;
	std::vector<std::string> relative_output;
	std::string hostname = "";
	int filePosition = -1;
	std::vector<FileStatus*>* siblings;
	std::mutex* mutex_siblings;

	//(new_path.parent_path() / fs::change_extension(new_path.filename(), "")).string() + std::to_string(file->filePosition) + new_path.extension()
	FileStatus(std::string name, std::string relative) : AFileStatus(), relative_name(relative), name(name), new_name(name)
	{
	}
	FileStatus(std::string name, std::string relative, std::string new_name) : AFileStatus(), relative_name(relative), name(name), new_name(new_name)
	{
	}
};
struct SlaveFileStatus : AFileStatus
{
	std::string result;

	SlaveFileStatus(std::string uuid, std::vector<unsigned char>* data) : AFileStatus(uuid, data)
	{
	}
};
