#pragma once

#include "Base/ImageFormatEnum.h"

#include "Base/BaseFileStatus.h"
#include "Master/MasterFileStatus.h"
#include <rttr/type>


namespace Docapost {
	namespace IA {
		namespace Tesseract {

			class Ocr
			{
			protected:
				ImageFormatEnum mImageFormat;
			public:
				virtual ~Ocr() = default;

				Ocr(ImageFormatEnum format) : mImageFormat(format)
				{

				}

				virtual MemoryFileBuffer* LoadFile(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile) = 0;
				virtual std::unique_ptr<std::vector<std::string>> ProcessThroughOcr(MemoryFileBuffer* imgData) = 0;
				virtual void InitEngine() = 0;
			};

		}
	}
}