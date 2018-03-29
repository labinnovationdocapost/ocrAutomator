#include "OcrResult.h"
#include "Master/MasterFileStatus.h"


OcrResult::OcrResult()
{
}

OcrResult::~OcrResult()
{
}

BaseFileStatus* OcrResult::File()
{
	return file;
}

void OcrResult::File(BaseFileStatus* file)
{
	this->file =  file;
}

std::string OcrResult::Name()
{
	if(auto f = static_cast<MasterFileStatus*>(this->file))
		return f->name;
	return "";
}
