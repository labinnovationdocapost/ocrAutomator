#pragma once

#include "Base/ImageFormatEnum.h"

#include "Base/BaseFileStatus.h"
#include "Master/MasterFileStatus.h"
#include <rttr/type>


namespace Docapost {
	namespace IA {
		namespace Tesseract {

			class BaseOcr
			{
			protected:
				ImageFormatEnum mImageFormat;
			public:
				virtual ~BaseOcr() = default;

				BaseOcr(ImageFormatEnum format) : mImageFormat(format)
				{

				}

				virtual MemoryFileBuffer* LoadFile(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile) = 0;
				virtual std::unique_ptr<std::vector<std::string>> ProcessThroughOcr(MemoryFileBuffer* imgData) = 0;
				virtual void InitEngine() = 0;
			};
			class OcrFactory
			{
			protected:
				ImageFormatEnum mImageFormat = ImageFormatEnum::JPG;
				std::vector<std::string> mExtension = { ".txt" };
			public:

				virtual ~OcrFactory() = default;
				virtual BaseOcr* CreateNew() = 0;
				virtual std::vector<std::string>& GetTextExtension()
				{
					return mExtension;
				}

				ImageFormatEnum ImageFormat() const { return mImageFormat; }
				void ImageFormat(const ImageFormatEnum image_format) { mImageFormat = image_format; }

				std::string GetExtension()
				{
					if (mImageFormat == ImageFormatEnum::JPG) return ".jpg";
					if (mImageFormat == ImageFormatEnum::PNG) return ".png";
					return "";
				}
				RTTR_ENABLE();
			};

		}
	}
}