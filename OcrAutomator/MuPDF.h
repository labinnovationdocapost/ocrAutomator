#pragma once

#include <mutex>
#include "MasterFileStatus.h"
#include "ImageFormatEnum.h"


extern "C" {
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
#include <mupdf/pdf-tools.h>
#include <mupdf/memento.h>
#include <turbojpeg.h>
}

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
		Docapost::IA::Tesseract::ImageFormatEnum format;
	};

	void Worker(WorkerParam, MasterFileStatus*);
	std::vector<unsigned char>* WriteToJPEG(WorkerParam& pix);
public:
	MuPDF();
	~MuPDF();

	int GetNbPage(std::string);
	void Extract(MasterFileStatus*, Docapost::IA::Tesseract::ImageFormatEnum format);
};