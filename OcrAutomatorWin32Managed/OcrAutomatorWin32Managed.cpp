// This is the main DLL file.

#include "stdafx.h"

#include "OcrAutomatorWin32Managed.h"
#include <fstream>
#include <iostream>

int OcrAutomatorPDF::nbpage = 0;
std::string OcrAutomatorPDF::name;
std::vector<OcrResult*> OcrAutomatorPDF::results;

using namespace System::Threading;
int i = 0;
void FileRecieved(OcrResult* file)
{
	if ((*Docapost::OCR::OcrAutomatorModel::pdf).find(file->Name()) != (*Docapost::OCR::OcrAutomatorModel::pdf).end())
	{
		auto pdf = (*Docapost::OCR::OcrAutomatorModel::pdf)[file->Name()];
		pdf->results.push_back(std::move(file));
		std::cout << file->Name() << "\n" << (*file->Text())[0] << "\n";
		std::ofstream stream(std::string("E:\\test") + std::to_string(i++) + ".jpg", std::ios_base::out | std::ios_base::binary);
		stream.write((char*)file->Image()->data(), file->Image()->len());

		if (pdf->results.size() == pdf->nbpage)
			Docapost::OCR::OcrAutomatorModel::event->Set();
	}
}

void main()
{
	/*auto ocr = gcnew Docapost::OCR::OcrAutomatorModel();
	auto conf = ocr->GetConfiguration();*/
	/*auto m_ocr = new OcrAutomatorMaster();
	auto ocrs = m_ocr->GetOcrs();
	auto ocr = ocrs[1];
	m_ocr->CreateFactory(ocr);
	auto params = m_ocr->GetOcrParametersDefinition(ocr);*/
	/*calculated_struct* param = *params.begin();
	std::string c1 = "PSM_SINGLE_CHAR";
	int c = 5;
	auto prop = m_ocr->GetFactoryProperty(param->name);*/
	/*m_ocr->SetFactoryProperty(param->name, c);
	m_ocr->SetFactoryProperty((*params.rbegin())->name, c);
	auto prop2 = m_ocr->GetFactoryProperty(param->name); */
	//m_ocr->CreateInstance();
	std::unordered_map<std::string, std::string> map;
	map["Lang"] = "fra";
	auto m_ocr = new OcrAutomatorMaster();
	m_ocr->QuickSetup("Tesseract", map); 

	std::ifstream stream("E:\\LinuxHeader\\PDF_TEST\\DOS-17075152_ID-3634969_IMPOT.pdf", std::ios::in | std::ios::binary);
	std::string data;
	stream.seekg(0, std::ios::end);
	data.reserve(stream.tellg());
	stream.seekg(0, std::ios::beg);
	data.assign((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
	auto l = data.length();

	OcrAutomatorPDF* pdf = new OcrAutomatorPDF();

	pdf->name = "1234";
	std::vector<std::string> s_uids;
	pdf->nbpage += m_ocr->AddPdf(pdf->name, (char*)data.data(), data.length(), s_uids);

	std::cout << "pages: " << pdf->nbpage << "\n";
	m_ocr->Start(4, FileRecieved);

	Docapost::OCR::OcrAutomatorModel::event->WaitOne();
	std::cout << "END\n";
}


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
	for (auto o : res)
	{
		auto params = mOcr->GetOcrParametersDefinition(o);
		for (auto param : params)
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
