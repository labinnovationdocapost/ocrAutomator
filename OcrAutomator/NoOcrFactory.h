#pragma once
#include "NoOcr.h"
#include "BaseOcr.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class NoOcrFactory : public OcrFactory
			{
			public:
				NoOcr* CreateNew() override {
					return new NoOcr(mImageFormat);
				}
			};
		}
	}
}
