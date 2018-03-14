#include "Tesseract.h"
#include "UninitializedOcrException.h"

#include <leptonica/allheaders.h>

#ifdef MAGICK
#include <Magick++.h>
#endif

#include "MuPDF.h"
#include "Error.h"
#include <boost/format.hpp>


Docapost::IA::Tesseract::Tesseract::Tesseract(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang, ImageFormatEnum format) : BaseOcrWithLoader(format), mPsm(psm), mOem(oem), mLang(lang), mTessBaseAPI(tesseract::TessBaseAPI())
{
	setMsgSeverity(L_SEVERITY_NONE);

	mTessBaseAPI.SetVariable("debug_file", "/dev/null");
	mTessBaseAPI.SetVariable("out", "quiet");

}


std::unique_ptr<std::vector<std::string>> Docapost::IA::Tesseract::Tesseract::ProcessThroughOcr(Docapost::IA::Tesseract::MemoryFileBuffer* imgData) {
	Pix *image = pixReadMem(imgData->data(), imgData->len());
	if (image == nullptr)
	{
		throw std::runtime_error("ProcessThroughOcr: Cannot open image");
	}

	mTessBaseAPI.SetImage(image);
	auto outtext = mTessBaseAPI.GetUTF8Text();

	//auto text = std::unique_ptr<std::string>{ new string(outtext) };


	std::vector<std::string>* vector = new std::vector<std::string>();
	vector->push_back(string(outtext));

	pixDestroy(&image);
	delete[] outtext;

	return std::unique_ptr<std::vector<std::string>>(vector);
}

void Docapost::IA::Tesseract::Tesseract::InitEngine()
{
	int res;
	if ((res = mTessBaseAPI.Init(nullptr, mLang.c_str(), mOem))) {
		throw UninitializedOcrException((boost::format("Could not initialize tesseract : %d") % res).str());
	}
	mTessBaseAPI.SetPageSegMode(mPsm);
}

Docapost::IA::Tesseract::Tesseract::~Tesseract()
{
	mTessBaseAPI.End();
}
