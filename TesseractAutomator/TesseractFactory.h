#pragma once
#include <string>
#include <memory>
#include "Tesseract.h"


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
				tesseract::PageSegMode Psm() const { return mPsm; }
				tesseract::OcrEngineMode Oem() const { return mOem; }
				std::string Lang() const { return mLang; }
				void Psm(tesseract::PageSegMode psm) { mPsm = psm; }
				void Oem(tesseract::OcrEngineMode oem) { mOem = oem; }
				void Lang(std::string lang) { mLang = lang; }

				std::shared_ptr<BaseOcr> CreateNew() override
				{
					return std::make_shared<Tesseract>(mPsm, mOem, mLang);
				}
			};
		}
	}
}
