#pragma once
#include "BaseOcr.h"
#include "A2IA.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class A2IAFactory : public OcrFactory
			{
			public:
				A2IA* CreateNew() override;
			};
		}
	}
}
