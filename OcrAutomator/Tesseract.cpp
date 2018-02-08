#include "Tesseract.h"
#include "UninitializedOcrException.h"

#include <leptonica/allheaders.h>

#ifdef MAGICK
#include <Magick++.h>
#endif

#include "MuPDF.h"
#include "Error.h"
#include "ArrayMemoryFileBuffer.h"


Docapost::IA::Tesseract::Tesseract::Tesseract(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang, ImageFormatEnum format) : BaseOcrWithLoader(format), mPsm(psm), mOem(oem), mLang(lang), mTessBaseAPI(tesseract::TessBaseAPI())
{
	setMsgSeverity(L_SEVERITY_NONE);

	mTessBaseAPI.SetVariable("debug_file", "/dev/null");
	mTessBaseAPI.SetVariable("out", "quiet");

}


std::unique_ptr<std::string> Docapost::IA::Tesseract::Tesseract::ProcessThroughOcr(Docapost::IA::Tesseract::MemoryFileBuffer* imgData) {
	Pix *image = pixReadMem(imgData->data(), imgData->len());
	if (image == nullptr)
	{
		return nullptr;
	}

	mTessBaseAPI.SetImage(image);
	auto outtext = mTessBaseAPI.GetUTF8Text();

	auto text = std::unique_ptr<std::string>{ new string(outtext) };

	pixDestroy(&image);
	delete[] outtext;

	return text;
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
