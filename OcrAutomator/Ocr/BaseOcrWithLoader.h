#pragma once
#include "Ocr.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {

			class BaseOcrWithLoader : public Ocr
			{
			protected:
				static int mMaxPdfCreationThread;
				static int mCurrentPdfCreationThread;
				static std::mutex mCreationThreadMutex;

				MemoryFileBuffer* ExtractPdfFromImageMagick(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile);
				MemoryFileBuffer* ExtractPdfFromMuPdf(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile);
				MemoryFileBuffer* GetImage(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile);

			public:

				virtual ~BaseOcrWithLoader() = default;

				explicit BaseOcrWithLoader(ImageFormatEnum format)
					: Ocr(format)
				{
				}

				static void AddPdfCreationThread(int nb)
				{
					mMaxPdfCreationThread += nb;
				}
				static void RemovePdfCreationThread(int nb)
				{
					mMaxPdfCreationThread -= nb;
				}


				/**
				* \brief
				* \param file
				* \param AddFile When in asynchrone task, if processed file is return by this function
				* \return Return nullptr if the processing il delegate to an asynchrone task, else return the file content
				*/
				MemoryFileBuffer* LoadFile(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile) override;
				virtual std::unique_ptr<std::vector<std::string>> ProcessThroughOcr(MemoryFileBuffer* imgData) = 0;
				virtual void InitEngine() = 0;
			};
		}
	}
}
