#pragma once
#include "BaseFileStatus.h"


struct SlaveFileStatus : BaseFileStatus
{
	SlaveFileStatus(std::string uuid, Docapost::IA::Tesseract::MemoryFileBuffer* data) : BaseFileStatus(uuid, data)
	{
	}
};
