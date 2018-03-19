#pragma once
#include "Ocr/BaseOcrWithLoader.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class NoOcr : public BaseOcrWithLoader
			{
			public:
				explicit NoOcr(ImageFormatEnum format)
					: BaseOcrWithLoader(format)
				{
				}

				std::unique_ptr<std::vector<std::string>> ProcessThroughOcr(MemoryFileBuffer* imgData) override
				{
					return std::make_unique<std::vector<std::string>>();
				}
				void InitEngine() override
				{
				}
			};
		}
	}
}
