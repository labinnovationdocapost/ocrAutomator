#pragma once
#include "BaseFileStatus.h"


struct SlaveFileStatus : BaseFileStatus
{
	std::string result;

	SlaveFileStatus(std::string uuid, std::vector<unsigned char>* data) : BaseFileStatus(uuid, data)
	{
	}
};
