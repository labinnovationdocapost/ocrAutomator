#pragma once
#include "Ocr/Ocr.h"
#include "Ocr/A2IA/A2IA.h"
#include "Ocr/OcrFactory.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class A2IAFactory : public OcrFactory
			{
				A2iARC_DocumentTable mDocumentTable;
			public:
				A2IAFactory();
				A2IA* CreateNew() override;
				RTTR_ENABLE(OcrFactory);
			};
		}
	}
}