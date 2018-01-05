#include "MuPDF.h"
#include <boost/thread.hpp>
#include "Error.h"

std::mutex MuPDF::mStaticContextMutex;

MuPDF::MuPDF()
{
	mDrawLocks.lock = &MuPDF::s_lock;

	mDrawLocks.unlock = &MuPDF::s_unlock;

	mDrawLocks.user = this;

	initLocks();
	mContext = fz_new_context(NULL, &mDrawLocks, FZ_STORE_UNLIMITED);
	
	if (!mContext)
	{
		throw std::runtime_error("cannot create mupdf context");
	}

	/* Register the default file types to handle. */
	fz_try(mContext)
		fz_register_document_handlers(mContext);
	fz_catch(mContext)
	{
		auto msg = fz_caught_message(mContext);
		fz_drop_context(mContext);
		throw std::runtime_error(std::string("cannot register document handlers: ") + msg);
	}
}

MuPDF::~MuPDF()
{
	fz_drop_context(mContext);
}

int MuPDF::GetNbPage(std::string path)
{
	std::lock_guard<std::mutex> lock(mContextMutex);
	fz_document *doc = nullptr;
	/* Open the document. */
	fz_try(mContext)
		doc = fz_open_document(mContext, path.c_str());
	fz_catch(mContext)
	{
		auto msg = fz_caught_message(mContext);
		fz_drop_context(mContext);
		throw std::runtime_error(std::string("cannot open document: ") + msg);
	}

	int pageCount = 0;
	/* Count the number of pages. */
	fz_try(mContext)
		pageCount = fz_count_pages(mContext, doc);
	fz_catch(mContext)
	{
		auto msg = fz_caught_message(mContext);
		fz_drop_document(mContext, doc);
		throw std::runtime_error(std::string("cannot count number of pages: ") + msg);
	}
	fz_drop_document(mContext, doc);
	return pageCount;
}

void MuPDF::Extract(MasterFileStatus* file)
{
	//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Begin extract PDF file";
	std::lock_guard<std::mutex> lock(mContextMutex);
	fz_document *doc = nullptr;
	/* Open the document. */
	fz_try(mContext)
		doc = fz_open_document(mContext, file->name.c_str());
	fz_catch(mContext)
	{
		auto msg = fz_caught_message(mContext);
		fz_drop_context(mContext);
		throw std::runtime_error(std::string("cannot open document: ") + msg);
	}

	int pageCount = 0;
	/* Count the number of pages. */
	fz_try(mContext)
		pageCount = fz_count_pages(mContext, doc);
	fz_catch(mContext)
	{
		auto msg = fz_caught_message(mContext);
		fz_drop_document(mContext, doc);
		fz_drop_context(mContext);
		throw std::runtime_error(std::string("cannot count number of pages: ") + msg);
	}
	//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Count page: " << pageCount;

	boost::thread_group worker_threads;
	for (int pageNumber = 0; pageNumber < pageCount; pageNumber++)
	{
		//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Page: " << pageNumber;
		if ((*file->siblings)[pageNumber] == nullptr)
			continue;

		fz_rect rect;
		fz_irect irect;

		fz_matrix ctm = fz_identity;
		fz_pre_scale(&ctm, 4, 4);


		//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "LoadPage";
		auto* page = fz_load_page(mContext, doc, pageNumber);;
		fz_bound_page(mContext, page, &rect);
		fz_transform_rect(&rect, &ctm);
		fz_round_rect(&irect, &rect);

		fz_display_list *list = fz_new_display_list(mContext, &rect);

		//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "New Device";

		fz_device *dev = nullptr;
		fz_try(mContext)
		{
			dev = fz_new_list_device(mContext, list);
			fz_run_page(mContext, page, dev, &ctm, NULL);
			fz_close_device(mContext, dev);
		}
		fz_always(mContext)
		{
			fz_drop_device(mContext, dev);
			fz_drop_page(mContext, page);
		}
		fz_catch(mContext)
		{
			auto msg = fz_caught_message(mContext);
			fz_drop_context(mContext);
			throw std::runtime_error(std::string("cannot count number of pages: ") + msg);
		}

		//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Destruct page";
		//fz_drop_page(mContext, page);

		fz_pixmap *pix = nullptr;
		fz_try(mContext)
		{ 
			std::lock_guard<std::mutex> staticlock(mStaticContextMutex);
			//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "create pixmap";
			pix = fz_new_pixmap_with_bbox(mContext, fz_device_rgb(mContext), &irect, 0, 0);
			fz_clear_pixmap_with_value(mContext, pix, 0xFF);
		}
		fz_catch(mContext)
		{
			auto msg = fz_caught_message(mContext);
			fz_drop_context(mContext);
			throw std::runtime_error(std::string("Cannot allocate: ") + msg);
		}


		WorkerParam wp;
		wp.pixmap = pix;
		wp.displayList = list;
		wp.pageNumber = pageNumber;
		fz_rect_from_irect(&wp.area, &irect);

		//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "create thread";
		//worker_threads.create_thread(boost::bind(&MuPDF::Worker, this, wp, (*file->siblings)[pageNumber]));
		Worker(wp, (*file->siblings)[pageNumber]);

	}
	fz_drop_document(mContext, doc);
	lock.~lock_guard();
	//worker_threads.join_all();


}

void MuPDF::s_lock(void* user, int lock)
{
	if (user == nullptr)
		return;

	MuPDF* mupdf = static_cast<MuPDF*>(user);

	if (mupdf == nullptr)
		return;

	mupdf->lock(user, lock);
}
void MuPDF::s_unlock(void* user, int lock)
{
	if (user == nullptr)
		return;

	MuPDF* mupdf = static_cast<MuPDF*>(user);

	if (mupdf == nullptr)
		return;

	mupdf->unlock(user, lock);
}

void MuPDF::lock(void* user, int lock)
{
	mMutexes[lock].lock();
}

void MuPDF::unlock(void* user, int lock)
{
	mMutexes[lock].unlock();
}

void MuPDF::initLocks()
{
	int i;
	int failed = 0;
}

void MuPDF::Worker(WorkerParam wp, MasterFileStatus* file)
{
	CatchAllErrorSignals();
	CatchAllExceptions();
	//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "clone context";
	auto local_ctx = fz_clone_context(mContext);

	fz_device *dev = nullptr;
	fz_try(local_ctx)
	{
		//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "create thread device";
		dev = fz_new_draw_device(local_ctx, &fz_identity, wp.pixmap);
		fz_run_display_list(local_ctx, wp.displayList, dev, &fz_identity, &wp.area, NULL);
		//fz_run_page(ctx, page, dev, &fz_identity, NULL);
		fz_close_device(local_ctx, dev);
	}
	fz_always(local_ctx)
	{
		fz_drop_device(local_ctx, dev);
	}
	fz_catch(local_ctx)
	{
		auto msg = fz_caught_message(mContext);
		fz_drop_pixmap(local_ctx, wp.pixmap);
		throw std::runtime_error(std::string("cannot draw pages: ") + msg);
	}

	//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "writing to jpeg";

	 
	file->data = WriteToJPEG(wp);
	file->fileSize = file->data->size();

	//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "clean";

	fz_drop_context(local_ctx);
	//std::lock_guard<std::mutex> lock(mContextMutex);
	fz_drop_pixmap(mContext, wp.pixmap);
	fz_drop_display_list(mContext, wp.displayList);

}

// Copier de https://github.com/chris-allan/libjpeg-turbo/blob/master/turbojpeg.c ligne 421 -> tjCompress2
std::vector<unsigned char>* MuPDF::WriteToJPEG(WorkerParam& wp)
{
	long unsigned int _jpegSize = 0;
	unsigned char* _compressedImage = NULL;
	tjhandle _jpegCompressor = tjInitCompress();
	tjCompress2(_jpegCompressor, wp.pixmap->samples, wp.area.x1, 0, wp.area.y1, TJPF_RGB,
		&_compressedImage, &_jpegSize, TJSAMP_444, 80,
		TJFLAG_FASTDCT);

	auto res = new std::vector<unsigned char>(_compressedImage, _compressedImage + _jpegSize);

	tjDestroy(_jpegCompressor);
	tjFree(_compressedImage);

	return res;
}
