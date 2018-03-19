#pragma once
#include "NoOcr.h"
#include "Ocr/BaseOcr.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class NoOcrFactory : public OcrFactory
			{
				int num;
			public:
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
