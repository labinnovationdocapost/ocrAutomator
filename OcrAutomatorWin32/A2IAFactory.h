#pragma once
#include "BaseOcr.h"
#include "A2IA.h"

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
