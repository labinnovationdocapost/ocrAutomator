#pragma once
#include "Ocr/BaseOcrWithLoader.h"
#include <rapidjson/writer.h>

using std::string;
#include <tesseract/baseapi.h>

#include "Base/BaseFileStatus.h"
#include "Base/ImageFormatEnum.h"
#include "Buffer/MemoryFileBuffer.h"


namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class Tesseract : public BaseOcrWithLoader
			{
			private:
				tesseract::PageSegMode mPsm;
				tesseract::OcrEngineMode mOem;
				std::string mLang;
				tesseract::TessBaseAPI mTessBaseAPI;

				struct Box
				{
					double score;
					char* text;
					int x1, x2, y1, y2;
					std::list<std::shared_ptr<Box>> childs;

					~Box()
					{
						if (text != nullptr)
							delete text;
					}
				};
				void CreateJSON(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::list<std::shared_ptr<Box>>& boxs);
				void CreateTree(tesseract::ResultIterator* result, std::deque<tesseract::PageIteratorLevel> stack, std::list<std::shared_ptr<Box>>& boxs);
				std::list<std::shared_ptr<Box>> CreateTree(tesseract::ResultIterator* result, std::deque<tesseract::PageIteratorLevel> stack);
			public:
				Tesseract(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang, ImageFormatEnum format);

				std::unique_ptr<std::vector<std::string>> ProcessThroughOcr(MemoryFileBuffer* imgData) override;

				void InitEngine() override;

				~Tesseract();
			};
		}
	}
}