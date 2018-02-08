#pragma once
#include "BaseOcrWithLoader.h"
#include "A2iATextReader.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class A2IA : public BaseOcrWithLoader
			{
				A2iARC_Id mEngineId;

				uint32_t mChannelId;
				A2iARC_DocumentTable mDocumentTable;
			public:
				explicit A2IA(ImageFormatEnum format)
					: BaseOcrWithLoader(format)
				{
				}

				std::unique_ptr<std::string> ProcessThroughOcr(MemoryFileBuffer* imgData) override;
				void InitEngine() override;
			};
		}
	}
}
