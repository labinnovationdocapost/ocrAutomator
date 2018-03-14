#pragma once
#include "BaseFileStatus.h"
#include <mutex>
#include "OutputFlags.h"
#include <vector>
#include <boost/uuid/random_generator.hpp>

struct MasterFileStatus : BaseFileStatus
{
	boost::uuids::basic_random_generator<boost::mt19937> mGen;
protected:
	MasterFileStatus(std::vector<Docapost::IA::Tesseract::OutputFlags> cap, std::string name):BaseFileStatus(), mGen(boost::uuids::basic_random_generator<boost::mt19937>()), capability(cap), name(name), uid(mGen())
	{
		
	};
public:
	std::string name;
	std::string hostname = "";
	int filePosition = -1;
	std::vector<MasterFileStatus*>* siblings;
	std::mutex* mutex_siblings;
	bool isCompleted = false;
	boost::uuids::uuid uid;

	const std::vector<Docapost::IA::Tesseract::OutputFlags> capability;

	virtual ~MasterFileStatus() { }
	//(new_path.parent_path() / fs::change_extension(new_path.filename(), "")).string() + std::to_string(file->filePosition) + new_path.extension()
};
