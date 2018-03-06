#include "Main.h"
#include "A2IAFactory.h"
#include "NoOcrFactory.h"

Docapost::IA::Tesseract::OcrFactory* CreateOcrFactory(po::variables_map& vm)
{
	//Docapost::IA::Tesseract::A2IAFactory* factory = new Docapost::IA::Tesseract::A2IAFactory();
	Docapost::IA::Tesseract::NoOcrFactory* factory = new Docapost::IA::Tesseract::NoOcrFactory();
	/*factory->Lang(vm["lang"].as<std::string>());
	factory->Oem(static_cast<tesseract::OcrEngineMode>(vm["oem"].as<int>()));
	factory->Psm(static_cast<tesseract::PageSegMode>(vm["psm"].as<int>()));*/

	return factory;
}
