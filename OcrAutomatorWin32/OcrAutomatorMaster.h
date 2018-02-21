#pragma once
#include <memory>
#include "OutputFlags.h"
#include <unordered_map>


struct calculated_struct
{
public:
	std::string name;
	std::string typeName;
	bool isEnum;
	std::vector<std::string> enu;
};


class __declspec(dllexport) OcrAutomatorMaster
{
public:
	OcrAutomatorMaster();
	~OcrAutomatorMaster();

	std::vector<std::string> GetOcrs();
	std::list<calculated_struct*> GetOcrParametersDefinition(std::string ocr);
	void CreateFactory(std::string ocr);
	template<typename T>
	void SetFactoryProperty(std::string prop, T& param);
	template<>
	void SetFactoryProperty<std::string>(std::string prop, std::string& param);
	template<>
	void SetFactoryProperty<int>(std::string prop, int& param);
	void Init(std::string ocr, std::unordered_map<std::string, void *> parametersDefinition, Docapost::IA::Tesseract::OutputFlags type = Docapost::IA::Tesseract::OutputFlags::None, int port = 12000);

private:
	struct Impl;
	std::unique_ptr<Impl> d_ptr;
};

