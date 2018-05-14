#include "HttpServer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include <archive_entry.h>
#include "Base/Error.h"
#include <boost/asio/ip/tcp.hpp>
using tcp = boost::asio::ip::tcp;

HttpServer::Request::Request(int nbpage) : mNbPage(nbpage), mSemaphore(new Semaphore(0)), mBuff(new char[mBuffLen]), mArchive(archive_write_new())
{
	archive_write_set_format_zip(mArchive);
	//archive_write_set_format_pax_restricted(mArchive);
	archive_write_open_memory(mArchive, mBuff, mBuffLen, &mSize);

}

HttpServer::Request::~Request() 
{
	delete mSemaphore;
	if (mIsDelete == false)
	{
		archive_write_close(mArchive);
		archive_write_free(mArchive);
		delete mBuff;
	}
}

std::unique_ptr<Docapost::IA::Tesseract::MemoryFileBuffer> HttpServer::Request::GetOutput()
{
	archive_write_close(mArchive);
	archive_write_free(mArchive);
	auto mem = std::make_unique<Docapost::IA::Tesseract::ArrayMemoryFileBuffer>((unsigned char*)mBuff, mSize);
	mIsDelete = true;
	return mem;
}

void HttpServer::Request::AddEntry(MasterFileStatus* file, void* data, size_t len, std::string ext)
{
	if (mIsDelete)
		return;
	struct archive_entry *entry = archive_entry_new();
	auto filenameNoExt = boost::filesystem::change_extension(file->name, "").string();
	if(file->filePosition >= 0)
	{
		archive_entry_set_pathname(entry, (boost::format("%s/%s-%i%s") % filenameNoExt % filenameNoExt % file->filePosition % ext).str().c_str());
	}
	else
	{
		archive_entry_set_pathname(entry, (boost::format("%s/%s%s") % filenameNoExt % filenameNoExt % ext).str().c_str());
	}
	archive_entry_set_size(entry, len);
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_write_header(mArchive, entry);
	archive_write_data(mArchive, data, len);
	archive_entry_free(entry);
}

void HttpServer::FileProcessed(MasterFileStatus* file)
{
	auto re = mRequests[file->uid];

	if (re == nullptr)
		return;

	re->AddEntry(file, file->buffer->data(), file->buffer->len(), mTesseractRunner.ocrFactory().GetExtension());
	for(int i = 0; i < file->result->size(); i++)
	{
		auto& text = (*file->result)[i];
		re->AddEntry(file, (char*)text.data(), text.size(), mTesseractRunner.ocrFactory().GetTextExtension()[i]);
	}
	re->Release();
	mRequests.erase(file->uid);
}

HttpServer::HttpServer(Docapost::IA::Tesseract::MasterProcessingWorker& tessR, std::wstring u, int p) : mTesseractRunner(tessR)
{
	onEndProcessFile = mTesseractRunner.onEndProcessFile.connect(boost::bind(&HttpServer::FileProcessed, this, _1));
	std::thread t { [&] ()
	{
		CatchAllErrorSignals();
		CatchAllExceptions();
		svr.get("/", [&](const httplib::Request& req, httplib::Response& res) {
			rapidjson::StringBuffer s;
			rapidjson::Writer<rapidjson::StringBuffer> writer(s);
			writer.StartObject();
			writer.Key("Total");
			writer.Uint(mTesseractRunner.Total());
			writer.Key("Done");
			writer.Uint(mTesseractRunner.Done());
			writer.Key("Remote");
			writer.Uint(mTesseractRunner.TotalRemoteThreads());
			writer.Key("Local");
			writer.Uint(mTesseractRunner.NbThreads());
			writer.EndObject();
			res.set_content(s.GetString(), "text/plain");
		});
		svr.post ("/", [&](const httplib::Request& req, httplib::Response& res) {
			CatchAllErrorSignals();
			CatchAllExceptions();
			int i = 0;
			for(auto& f : req.files)
			{
				auto file = boost::filesystem::path(f.second.filename);				
				if(f.first == "image" && file.has_extension() && mTesseractRunner.extensions.count(file.extension().string()))
				{
					if(file.extension().string() == ".pdf")
					{
						char* img = new char[f.second.length];
						memcpy(img, req.body.data() + f.second.offset, f.second.length);


						std::vector<boost::uuids::uuid> uids;
						auto nbPage = mTesseractRunner.AddPdfFile(f.second.filename, img, f.second.length, uids);

						Request* re = new Request(nbPage);
						for (auto& uid : uids)
							mRequests[uid] = re;

						re->Wait();

						auto buff = re->GetOutput();
						res.set_content((char*)buff->data(), buff->len(), "application/zip");
						delete re;
						continue;
					} 
					else
					{
						char* img = new char[f.second.length];
						memcpy(img, req.body.data() + f.second.offset, f.second.length);

						boost::uuids::uuid uid;
						mTesseractRunner.AddImageFile(f.second.filename, img, f.second.length, uid);
						
						Request* re = new Request(1);
						mRequests[uid] = re;

						re->Wait();

						auto buff = re->GetOutput();
						res.set_content((char*)buff->data(), buff->len(), "application/zip");
						delete re;
						continue;
					}
				}
			}
			
		});

		svr.listen("0.0.0.0", 8888);
		svr.stop();
	}};
	t.detach();
}

HttpServer::~HttpServer()
{
	onEndProcessFile.disconnect();
	svr.stop();
}
