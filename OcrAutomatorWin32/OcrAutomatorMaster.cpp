#include "OcrAutomatorMaster.h"
#include "MasterProcessingWorker.h"
#include <rttr/registration.h>
#include "Tesseract.h"
#include "TesseractFactory.h"


struct OcrAutomatorMaster::Impl
{
	Impl() {}
	~Impl() {}
	std::unique_ptr<Docapost::IA::Tesseract::MasterProcessingWorker> mWorker;
	Docapost::IA::Tesseract::OcrFactory* mOcr;
	std::unique_ptr<rttr::type> mOcrType;
};

OcrAutomatorMaster::OcrAutomatorMaster()
{
	d_ptr = std::move(std::make_unique<OcrAutomatorMaster::Impl>());
	//d_ptr->mWorker = std::unique_ptr<Docapost::IA::Tesseract::MasterProcessingWorker>();
}

OcrAutomatorMaster::~OcrAutomatorMaster()
{
}

std::vector<std::string> OcrAutomatorMaster::GetOcrs()
{
	auto classs_type = rttr::type::get_by_name("OcrFactory").get_derived_classes();
	std::vector<std::string> ret;
	for(auto& type : classs_type)
	{
		ret.push_back(type.get_name());
	}
	return ret;
}


std::list<calculated_struct*> OcrAutomatorMaster::GetOcrParametersDefinition(std::string ocr)
{
	rttr::type classs_type = rttr::type::get_by_name(ocr.c_str());
	auto props = classs_type.get_properties();

	std::list<calculated_struct*> ret;
	for (auto& prop : props)
	{
		calculated_struct* stru = new calculated_struct();
		ret.push_back(stru);

		stru->name = prop.get_name();
		stru->typeName = prop.get_type().get_name();
		stru->isEnum = prop.get_type().is_enumeration();
		if(stru->isEnum)
		{
			for (auto e : prop.get_type().get_enumeration().get_values())
			{
				stru->enu.push_back(e.to_string());
			}
		}
	}
	return ret;
}

void OcrAutomatorMaster::CreateFactory(std::string ocr)
{
	d_ptr->mOcrType = std::make_unique<rttr::type>(rttr::type::get_by_name(ocr.c_str()));
	d_ptr->mOcr = d_ptr->mOcrType->create().get_value<Docapost::IA::Tesseract::OcrFactory*>();
}

template<typename T>
void OcrAutomatorMaster::SetFactoryProperty(std::string prop, T& param)
{
	auto p = d_ptr->mOcrType->get_property(prop.c_str());
	p.set_value(d_ptr->mOcr, param);
}

template<>
void OcrAutomatorMaster::SetFactoryProperty<std::string>(std::string prop, std::string& param)
{
	auto p = d_ptr->mOcrType->get_property(prop.c_str());
	if(p.get_type().is_enumeration())
	{
		auto val = p.get_type().get_enumeration().name_to_value(param);
		auto i = val.to_int();

		p.set_value(d_ptr->mOcr, val);

		auto t = (Docapost::IA::Tesseract::Tesseract*)d_ptr->mOcr;
	}
	else
	{
		p.set_value(d_ptr->mOcr, param);
	}
}

template<>
void OcrAutomatorMaster::SetFactoryProperty<int>(std::string prop, int& param)
{
	auto p = d_ptr->mOcrType->get_property(prop.c_str());
	p.set_value(d_ptr->mOcr, param);
}

void OcrAutomatorMaster::Init(std::string ocr, std::unordered_map<std::string, void*> parametersDefinition, Docapost::IA::Tesseract::OutputFlags type, int port)
{
}
