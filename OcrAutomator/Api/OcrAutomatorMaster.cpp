#include "OcrAutomatorMaster.h"
#include "Master/MasterProcessingWorker.h"
#include <rttr/registration.h>
#include "Ocr/Tesseract/Tesseract.h"
#include "Ocr/Tesseract/TesseractFactory.h"
#include "Ocr/OcrFactory.h"

struct OcrAutomatorMaster::Impl
{
	Impl() {}
	~Impl()
	{
		if (mOcr != nullptr)
			delete mOcr;
	}
	std::unique_ptr<Docapost::IA::Tesseract::MasterProcessingWorker> mWorker;
	Docapost::IA::Tesseract::OcrFactory* mOcr = nullptr;
	std::unique_ptr<rttr::type> mOcrType;


	void ReceiveFile(MasterFileStatus* file, std::function<void(OcrResult*)> callback)
	{
		auto result = new OcrResult();
		result->File(file);
		callback(result);
	}
};

OcrAutomatorMaster::OcrAutomatorMaster()
{
	d_ptr = std::move(std::make_unique<OcrAutomatorMaster::Impl>());
	//d_ptr->mWorker = std::unique_ptr<Docapost::IA::Tesseract::MasterProcessingWorker>();
}

OcrAutomatorMaster::~OcrAutomatorMaster()
{
}

void OcrAutomatorMaster::CreateFactory(std::string ocr)
{
	d_ptr->mOcrType = std::make_unique<rttr::type>(rttr::type::get_by_name(ocr.c_str()));
	d_ptr->mOcr = Docapost::IA::Tesseract::OcrFactory::CreateNew(ocr);
}

template<typename T>
void OcrAutomatorMaster::SetFactoryProperty(std::string prop, T param)
{
	auto p = d_ptr->mOcrType->get_property(prop.c_str());
	p.set_value(d_ptr->mOcr, param);
}

void OcrAutomatorMaster::SetFactoryProperty(std::string prop, std::string param)
{
	auto p = d_ptr->mOcrType->get_property(prop.c_str());
	if (p.get_type().is_enumeration())
	{
		char* p_s;
		long converted = strtol(param.c_str(), &p_s, 10);
		if(*p_s)
		{
			auto val = p.get_type().get_enumeration().name_to_value(param);

			p.set_value(d_ptr->mOcr, val);
		}
		else
		{
			
		}
	}
	else
	{
		const auto type = p.get_type();
		rttr::variant var(param);
		if(var.get_type() != type)
			auto res = var.convert(type);
		p.set_value(d_ptr->mOcr, var);
	}
}

void OcrAutomatorMaster::SetFactoryProperty(std::string prop, int param)
{
	auto p = d_ptr->mOcrType->get_property(prop.c_str());
	p.set_value(d_ptr->mOcr, rttr::variant(param));
}

std::string OcrAutomatorMaster::GetFactoryProperty(std::string prop)
{
	auto p = d_ptr->mOcrType->get_property(prop.c_str());
	return p.get_value(d_ptr->mOcr).to_string();
}

void OcrAutomatorMaster::QuickSetup(std::string ocr, std::unordered_map<std::string, std::string> parametersDefinition, Docapost::IA::Tesseract::OutputFlags type, int port)
{
	CreateFactory(ocr);
	for (auto& param : parametersDefinition)
	{
		SetFactoryProperty(param.first, param.second);
	}
	CreateInstance(type, port);
}

void OcrAutomatorMaster::CreateInstance(Docapost::IA::Tesseract::OutputFlags type, int port)
{
	d_ptr->mWorker = std::make_unique<Docapost::IA::Tesseract::MasterProcessingWorker>(*d_ptr->mOcr, type, port);
}

void OcrAutomatorMaster::AddImage(std::string id, char* img, int len, std::string& s_uid)
{
	boost::uuids::uuid uid;
	d_ptr->mWorker->AddImageFile(id, img, len, uid);
	s_uid = boost::uuids::to_string(uid);
}
int OcrAutomatorMaster::AddPdf(std::string id, char* img, int len, std::vector<std::string>& s_uids)
{
	std::vector<boost::uuids::uuid> uids;
	auto nbpages = d_ptr->mWorker->AddPdfFile(id, img, len, uids);
	s_uids.clear();
	for(auto& uid : uids)
	{
		s_uids.push_back(boost::uuids::to_string(uid));
	}
	return nbpages;
}

void OcrAutomatorMaster::Start(int maxThread, std::function<void(OcrResult*)> callback)
{
	d_ptr->mWorker->onEndProcessFile.connect(boost::bind(&Impl::ReceiveFile, this->d_ptr.get(), _1, callback));

	d_ptr->mWorker->Run(maxThread);

}

namespace SetFactoryProperty
{
	template<typename T>
	void SetFactoryProperty(std::string prop, T param)
	{
	}
}