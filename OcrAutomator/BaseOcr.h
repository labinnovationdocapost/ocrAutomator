#pragma once

#include <string>
#include "ImageFormatEnum.h"

using std::string;
#include <tesseract/baseapi.h>

#include <iostream>
#include <boost/format.hpp>
#include "BaseFileStatus.h"
#include "MasterFileStatus.h"

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
				virtual bool ProcessThroughOcr(MemoryFileBuffer* imgData, std::string& text) = 0;
			};
			class OcrFactory
			{
			protected:
				ImageFormatEnum mImageFormat = ImageFormatEnum::JPG;
			public:

				virtual ~OcrFactory() = default;
				virtual BaseOcr* CreateNew() = 0;

				ImageFormatEnum ImageFormat() const { return mImageFormat; }
				void ImageFormat(const ImageFormatEnum image_format) { mImageFormat = image_format; }

				std::string GetExtension()
				{
					if (mImageFormat == ImageFormatEnum::JPG) return ".jpg";
					if (mImageFormat == ImageFormatEnum::PNG) return ".png";
					return "";
				}
			};

		}
	}
}