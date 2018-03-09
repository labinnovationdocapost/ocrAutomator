#pragma once
#include "BaseFileStatus.h"
#include <mutex>
#include "OutputFlags.h"

struct MasterFileStatus : BaseFileStatus
{
protected:
	MasterFileStatus(std::vector<Docapost::IA::Tesseract::OutputFlags> cap, std::string name):BaseFileStatus(), capability(cap), name(name)
	{
		
	};
public:
	std::string name;
	std::string hostname = "";
	int filePosition = -1;
	std::vector<MasterFileStatus*>* siblings;
	std::mutex* mutex_siblings;
	bool isCompleted = false;

	const std::vector<Docapost::IA::Tesseract::OutputFlags> capability;

	virtual ~MasterFileStatus() { }
	//(new_path.parent_path() / fs::change_extension(new_path.filename(), "")).string() + std::to_string(file->filePosition) + new_path.extension()
};
