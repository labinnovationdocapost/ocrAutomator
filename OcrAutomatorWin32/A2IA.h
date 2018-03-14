#pragma once
#include "BaseOcrWithLoader.h"
#include "A2iATextReader.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class A2IA : public BaseOcrWithLoader
			{
				uint32_t mChannelId = 0;
				A2iARC_DocumentTable& mDocumentTable;

				void CreateJSON(rapidjson::Writer<rapidjson::StringBuffer>& writer, A2iARC_TranscriptionResults& result);

				void CreateJSON(rapidjson::Writer<rapidjson::StringBuffer>& writer, A2iARC_TranscriptionLevelResults& result);
			public:
				explicit A2IA(ImageFormatEnum format, A2iARC_DocumentTable& documentTable)
					: BaseOcrWithLoader(format), mDocumentTable(documentTable)
				{
				}

				~A2IA();

				std::unique_ptr<std::vector<std::string>> ProcessThroughOcr(MemoryFileBuffer* imgData) override;
				void InitEngine() override;
			};
		}
	}
}
