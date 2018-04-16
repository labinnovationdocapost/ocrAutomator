#pragma once
#include "NoOcr.h"
#include "Ocr/Ocr.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class NoOcrFactory : public OcrFactory
			{
				int num;
			public:
				std::string Name() override { return "NoOcr"; }
				std::string Version() override { return "0.0"; }
				int Num() const
				{
					return num;
				}

				void Num(const int num)
				{
					this->num = num;
				}

				NoOcr* CreateNew() override {
					return new NoOcr(mImageFormat);
				}

				RTTR_ENABLE(OcrFactory)
			};
		}
	}
}
