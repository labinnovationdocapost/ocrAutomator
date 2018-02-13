#pragma once
#include "BaseOcrWithLoader.h"
#include "A2iATextReader.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class A2IA : public BaseOcrWithLoader
			{
				uint32_t mChannelId = 0;
				A2iARC_DocumentTable& mDocumentTable;
			public:
				explicit A2IA(ImageFormatEnum format, A2iARC_DocumentTable& documentTable)
					: BaseOcrWithLoader(format), mDocumentTable(documentTable)
				{
				}

				~A2IA();

				std::unique_ptr<std::string> ProcessThroughOcr(MemoryFileBuffer* imgData) override;
				void InitEngine() override;
			};
		}
	}
}
