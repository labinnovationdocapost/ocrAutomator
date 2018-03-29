#pragma once
#include <vector>

struct OcrProperty
{
public:
	std::string name;
	std::string typeName;
	bool isEnum;
	std::vector<std::string> enu;
	std::string description;
};
