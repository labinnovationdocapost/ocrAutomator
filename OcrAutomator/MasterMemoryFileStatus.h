#pragma once
#include "MasterFileStatus.h"
#include <vector>

struct MasterMemoryFileStatus : MasterFileStatus
{
private:
	char* original_file;
	int length;
public:
	char* OriginalFile() const
	{
		return original_file;
	}

	int Length() const
	{
		return length;
	}

	MasterMemoryFileStatus(std::string id, char* original_file, int len) : MasterFileStatus({ Docapost::IA::Tesseract::OutputFlags::MemoryText, Docapost::IA::Tesseract::OutputFlags::MemoryImage },id), original_file(original_file), length(len)
	{
	}

	~MasterMemoryFileStatus()
	{
		if(original_file != nullptr)
			delete original_file;
	}
};
