#include "Main.h"
#include "Ocr/Tesseract/TesseractFactory.h"

Docapost::IA::Tesseract::OcrFactory* CreateOcrFactory(po::variables_map& vm)
{
	auto factory = Docapost::IA::Tesseract::OcrFactory::CreateNew("Tesseract");
	auto props = factory->GetOcrParametersDefinition();
	for (auto& prop : props)
	{
		if (vm.count(prop->name))
		{
			factory->SetFactoryProperty(prop->name, vm[prop->name].as<std::string>());
		}
	}

	return factory;
}