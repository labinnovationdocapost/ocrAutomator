#pragma once
#include "MasterFileStatus.h"
#include <vector>
#include "OutputFlags.h"


struct MasterLocalFileStatus : MasterFileStatus
{
private:
	std::vector<Docapost::IA::Tesseract::OutputFlags> _capability = { Docapost::IA::Tesseract::OutputFlags::Text, Docapost::IA::Tesseract::OutputFlags::Exif };
public:
	std::string relative_name;
	std::string new_name;
	std::vector<std::string> output;
	std::vector<std::string> relative_output;

	MasterLocalFileStatus(std::string name, std::string relative) : MasterFileStatus(_capability,name), relative_name(relative), new_name(name)
	{
	}
	MasterLocalFileStatus(std::string name, std::string relative, std::string new_name) : MasterFileStatus(_capability,name), relative_name(relative), new_name(new_name)
	{
	}
};
