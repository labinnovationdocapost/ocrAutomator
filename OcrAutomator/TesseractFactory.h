#pragma once
#include <string>
#include <memory>
#include "Tesseract.h"
#include <rttr/registration>

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class TesseractFactory : public OcrFactory
			{
			private:
				std::vector<std::string> mExtesnion = {".txt", ".json"};
				tesseract::PageSegMode mPsm = tesseract::PageSegMode::PSM_AUTO;
				tesseract::OcrEngineMode mOem = tesseract::OcrEngineMode::OEM_DEFAULT;
				std::string mLang = "fra";
			public:
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
					mExtension = { ".txt" };
				}


				RTTR_ENABLE(OcrFactory)
			};
		}
	}
}