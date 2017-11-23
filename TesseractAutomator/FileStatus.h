#pragma once
#include "BaseFileStatus.h"

struct FileStatus : BaseFileStatus
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
	std::string result;
	bool isCompleted = false;

	//(new_path.parent_path() / fs::change_extension(new_path.filename(), "")).string() + std::to_string(file->filePosition) + new_path.extension()
	FileStatus(std::string name, std::string relative) : BaseFileStatus(), relative_name(relative), name(name), new_name(name)
	{
	}
	FileStatus(std::string name, std::string relative, std::string new_name) : BaseFileStatus(), relative_name(relative), name(name), new_name(new_name)
	{
	}
};
