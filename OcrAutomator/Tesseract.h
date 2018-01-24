#pragma once
#include <string>
#include "BaseOcr.h"

using std::string;
#include <tesseract/baseapi.h>

#include <iostream>
#include <boost/format.hpp>
#include "BaseFileStatus.h"
#include "MasterFileStatus.h"
#include "ImageFormatEnum.h"
#include "MemoryFileBuffer.h"


namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class Tesseract : public BaseOcr
			{
			private:
				tesseract::PageSegMode mPsm;
				tesseract::OcrEngineMode mOem;
				std::string mLang;
				tesseract::TessBaseAPI mTessBaseAPI;
				static const int mMaxPdfCreationThread = 4;
				static int mCurrentPdfCreationThread;
				static std::mutex mCreationThreadMutex;

				MemoryFileBuffer* ExtractPdfFromImageMagick(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile);
				MemoryFileBuffer* ExtractPdfFromMuPdf(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile);
				MemoryFileBuffer* GetImage(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile);
			public:
				Tesseract(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang, ImageFormatEnum format);

				/**
				 * \brief 
				 * \param file 
				 * \param AddFile When in asynchrone task, if processed file is return by this function
				 * \return Return nullptr if the processing il delegate to an asynchrone task, else return the file content
				 */
				MemoryFileBuffer* LoadFile(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile) override;
				std::unique_ptr<std::string> ProcessThroughOcr(MemoryFileBuffer* imgData) override;

				void InitEngine() override;

				~Tesseract();
			};
		}
	}
}