// This is the main DLL file.

#include "stdafx.h"

#include "OcrAutomatorWin32Managed.h"

//#include "MasterProcessingWorker.h"

Docapost::OCR::OcrAutomatorModel::OcrAutomatorModel()
{
	mOcr = new OcrAutomatorMaster();
}

Docapost::OCR::OcrAutomatorModel::~OcrAutomatorModel()
{
	//delete mpw;
}

List<OcrType^>^  Docapost::OCR::OcrAutomatorModel::GetConfiguration()
{
	auto res = mOcr->GetOcrs();
	auto list = gcnew List<OcrType^>();
	for(auto o : res)
	{
		auto params = mOcr->GetOcrParametersDefinition(o);
		for(auto param : params)
		{
			list->Add(gcnew OcrType(param));
		}
	}

	return list;
}
void Docapost::OCR::OcrAutomatorModel::RunOcr()
{
	mOcr->CreateFactory("Tesseract");
	auto str = std::string("PSM_AUTO_ONLY");
	mOcr->SetFactoryProperty(std::string("PSM"), str);
	//mOcr->SetFactoryProperty(std::string("Lang"), "fra");
	/*mpw = new Docapost::IA::Tesseract::MasterProcessingWorker(factory, Docapost::IA::Tesseract::OutputFlags::Exif | Docapost::IA::Tesseract::OutputFlags::Text);*/
}
