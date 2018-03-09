#pragma once
#include "MasterFileStatus.h"
#include <vector>

struct MasterMemoryFileStatus : MasterFileStatus
{
private:
	std::vector<Docapost::IA::Tesseract::OutputFlags> _capability = { Docapost::IA::Tesseract::OutputFlags::MemoryText, Docapost::IA::Tesseract::OutputFlags::MemoryImage };

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

	MasterMemoryFileStatus(std::string id, char* original_file, int len) : MasterFileStatus(_capability,id), original_file(original_file), length(len)
	{
	}
};
