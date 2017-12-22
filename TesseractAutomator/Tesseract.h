#pragma once
#include <string>
#include "BaseOcr.h"

using std::string;
#include <tesseract/baseapi.h>

#include <iostream>
#include <boost/format.hpp>
#include "BaseFileStatus.h"
#include "MasterFileStatus.h"


namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class Tesseract : public BaseOcr
			{
				tesseract::PageSegMode mPsm;
				tesseract::OcrEngineMode mOem;
				std::string mLang;
				tesseract::TessBaseAPI mTessBaseAPI;

				std::vector<unsigned char>* ExtractPdfFromImageMagick(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile);
				std::vector<unsigned char>* ExtractPdfFromMuPdf(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile);
			public:
				Tesseract(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang);

				std::vector<unsigned char>* LoadFile(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile) override;
				bool ProcessThroughOcr(std::vector<unsigned char>* imgData, std::string& text) override;

				~Tesseract();
			};
		}
	}
}