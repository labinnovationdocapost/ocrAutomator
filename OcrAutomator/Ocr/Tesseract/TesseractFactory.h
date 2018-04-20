#pragma once
#include <string>
#include <memory>
#include "Ocr/Tesseract/Tesseract.h"
#include <rttr/registration>
#include "Ocr/OcrFactory.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class TesseractFactory : public OcrFactory
			{
			private:
				tesseract::PageSegMode mPsm = tesseract::PageSegMode::PSM_AUTO;
				tesseract::OcrEngineMode mOem = tesseract::OcrEngineMode::OEM_DEFAULT;
				std::string mLang = "fra";
			public:
				std::string Name() override { return "Tesseract"; }
				std::string Version() override { return TESSERACT_VERSION_STR; }
				tesseract::PageSegMode Psm() const { return mPsm; }
				tesseract::OcrEngineMode Oem() const { return mOem; }
				std::string Lang() const { return mLang; }
				void Psm(tesseract::PageSegMode psm) { mPsm = psm; }
				void Oem(tesseract::OcrEngineMode oem) { mOem = oem; }
				void Lang(std::string lang) { mLang = lang; }

				Tesseract* CreateNew() override
				{
					return new Tesseract(mPsm, mOem, mLang, mImageFormat);
				}

				TesseractFactory()
				{
					mExtension = { ".txt", ".json" };
				}


				RTTR_ENABLE(OcrFactory)
			};
		}
	}
}
