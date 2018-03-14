#pragma once

#include <unordered_map>

#include "MasterProcessingWorker.h"
/*#include "cpprest/json.h"
#include "cpprest/http_listener.h"
#include "cpprest/uri.h"
#include "cpprest/asyncrt_utils.h"
#include "cpprest/json.h"
#include "cpprest/filestream.h"
#include "cpprest/containerstream.h"
#include "cpprest/producerconsumerstream.h"*/

#include "httplib.h"
#include <archive.h>
#include "AutoResetEvent.h"
#include "ArrayMemoryFileBuffer.h"

class HttpServer 
{
public:
	class Request
	{
		int mNbPage;
		bool mIsDelete = false;
		Semaphore* mSemaphore;

		int mBuffLen = 1024 * 1024 * 100;
		char* mBuff;

		struct archive *mArchive;

		size_t mSize;
	public:

		Request(int nbpage);
		~Request();

		void Release()
		{
			mSemaphore->Release(1);
		}
		void Wait()
		{
			mSemaphore->Wait(mNbPage);
		}

		std::unique_ptr<Docapost::IA::Tesseract::MemoryFileBuffer> GetOutput();

		void AddEntry(MasterFileStatus* file, void* data, size_t len, std::string ext);
	};
private:

	Docapost::IA::Tesseract::MasterProcessingWorker& mTesseractRunner;
	//std::list<std::unique_ptr<web::http::experimental::listener::http_listener>> m_listeners;
	boost::unordered_map<boost::uuids::uuid, Request*> mRequests;

	void FileProcessed(MasterFileStatus *);
	boost::signals2::connection onEndProcessFile;
	httplib::Server svr;
public:
	explicit HttpServer(Docapost::IA::Tesseract::MasterProcessingWorker &tessR, std::wstring uri, int port);
	~HttpServer();


	//void handle_get(web::http::http_request message);

};
