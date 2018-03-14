#pragma once

#include <mutex>
#include "MasterFileStatus.h"
#include "ImageFormatEnum.h"
#include "JpegTurboMemoryFileBuffer.h"
#include "FzBufferMemoryFileBuffer.h"
#include <condition_variable>


extern "C" {
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
#include <mupdf/pdf-tools.h>
#include <mupdf/memento.h>
#include <turbojpeg.h>
}

namespace Docapost {
	namespace IA {
		namespace MuPDF {
			struct ImageQuality
			{
				int jpegQuality;
				ImageQuality() : jpegQuality(80) {};
				explicit ImageQuality(int jpeg) : jpegQuality(jpeg) {}
			};
			class MuPDF
			{
				static void s_lock(void *user, int lock);
				static void s_unlock(void *user, int lock);

				typedef void(*old_lock)(void*, int);

				std::mutex mMutexes[FZ_LOCK_MAX];
				std::mutex mContextMutex;
				
				static std::mutex mStaticContextMutex;

				void lock(void *user, int lock);
				void unlock(void *user, int lock);

				fz_locks_context mDrawLocks;

				void initLocks();

				fz_context* mContext;

				struct WorkerParam
				{
					fz_pixmap* pixmap;
					fz_display_list * displayList;
					fz_rect area;
					int pageNumber;
					ImageQuality quality;
					Tesseract::ImageFormatEnum format;
				};

				int mMaxResolution = 3500;
				void Worker(WorkerParam, MasterFileStatus*);
				Tesseract::MemoryFileBuffer* WriteToJPEG(WorkerParam& pix, int quality) const;
			public:
				MuPDF();
				~MuPDF();

				int GetNbPage(std::string);
				int GetNbPage(char* pdf, int len);
				void Extract(MasterFileStatus*, Tesseract::ImageFormatEnum format, ImageQuality quality = ImageQuality());
			};
		}
	}
}