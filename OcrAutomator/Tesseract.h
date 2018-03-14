#pragma once
#include "BaseOcrWithLoader.h"

using std::string;
#include <tesseract/baseapi.h>

#include "BaseFileStatus.h"
#include "ImageFormatEnum.h"
#include "MemoryFileBuffer.h"


namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class Tesseract : public BaseOcrWithLoader
			{
			private:
				tesseract::PageSegMode mPsm;
				tesseract::OcrEngineMode mOem;
				std::string mLang;
				tesseract::TessBaseAPI mTessBaseAPI;
			public:
				Tesseract(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang, ImageFormatEnum format);

				std::unique_ptr<std::vector<std::string>> ProcessThroughOcr(MemoryFileBuffer* imgData) override;

				void InitEngine() override;

				~Tesseract();
			};
		}
	}
}