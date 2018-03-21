#include "Main.h"
#include "Ocr/Tesseract/TesseractFactory.h"

Docapost::IA::Tesseract::OcrFactory* CreateOcrFactory(po::variables_map& vm)
{
	Docapost::IA::Tesseract::TesseractFactory* factory = new Docapost::IA::Tesseract::TesseractFactory();
	factory->Lang(vm["lang"].as<std::string>());
	factory->Oem(static_cast<tesseract::OcrEngineMode>(vm["oem"].as<int>()));
	factory->Psm(static_cast<tesseract::PageSegMode>(vm["psm"].as<int>()));

	return factory;
}