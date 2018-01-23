#include "MasterProcessingWorker.h"

#include <iostream>
#include <fstream>
#include <future>
#include <exiv2/exiv2.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/format.hpp>
#include "Error.h"
#include "TesseractFactory.h"
#include "UninitializedOcrException.h"
#include <boost/algorithm/string.hpp>
#include "MuPDF.h"
#include "ExivMemoryFileBuffer.h"


Docapost::IA::Tesseract::MasterProcessingWorker::MasterProcessingWorker(OcrFactory& ocr, Docapost::IA::Tesseract::OutputFlags types, int port) :
	BaseProcessingWorker(ocr),
	mOutputTypes(types)
{
	/*PoDoFo::PdfError::EnableDebug(false);
	PoDoFo::PdfError::EnableLogging(false);*/

	mNetworkThread = new std::thread([this, port]()
	{
		int _port = port;
		while (_port < 65565)
		{
			try
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Try Starting network on port " << _port;
				CatchAllErrorSignals();
				mNetwork = boost::make_shared<Network>(_port);
				mNetwork->onSlaveConnect.connect(boost::bind(&MasterProcessingWorker::OnSlaveConnectHandler, this, _1, _2, _3));

				mNetwork->onSlaveSynchro.connect(boost::bind(&MasterProcessingWorker::OnSlaveSynchroHandler, this, _1, _2, _3, _4));

				mNetwork->onSlaveDisconnect.connect(boost::bind(&MasterProcessingWorker::OnSlaveDisconnectHandler, this, _1, _2));

				mNetwork->InitBroadcastReceiver();
				mNetwork->InitComm();
				mNetwork->Start();
				break;
			}
			catch (std::exception &e)
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "/!\\ Unable to start network on port " << _port;
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "/!\\ Error " << e.what();

				if (mNetwork != nullptr)
				{
					mNetwork->Stop();
					mNetwork.reset();
				}
				_port++;
			}
			catch (...)
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "/!\\ Unable to start network on port " << _port;
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "/!\\ Error " << GET_EXCEPTION_NAME;

				if (mNetwork != nullptr)
				{
					mNetwork->Stop();
					mNetwork.reset();
				}
				_port++;
			}
		}
	});
}




fs::path Docapost::IA::Tesseract::MasterProcessingWorker::ConstructNewTextFilePath(fs::path path) const
{
	fs::path new_path;

	auto it = mOutputs.find(OutputFlags::Text);
	if (it == mOutputs.end() || it->second == mInput)
	{
		new_path = fs::change_extension(path, ".txt");
	}
	else
	{
		auto relative_path = fs::relative(path, mInput);

		new_path = fs::absolute(relative_path, it->second);
		if (mOutputTypes & OutputFlags::Flattern)
			new_path = new_path.parent_path() / boost::replace_all_copy(fs::change_extension(relative_path, ".txt").string(), "/", mSeparator);
		else
			new_path = new_path.parent_path() / fs::change_extension(new_path.filename(), ".txt");

	}
	return new_path;
}

void Docapost::IA::Tesseract::MasterProcessingWorker::MergeResult(MasterFileStatus* file)
{
	if (file->filePosition >= 0)
	{
		std::lock_guard<std::mutex>(*file->mutex_siblings);
		file->isCompleted = true;
		bool completed = true;
		for (int i = 0; i < file->siblings->size(); i++)
		{
			auto item = (*file->siblings)[i];
			if (item != nullptr)
			{
				if (!item->isCompleted)
				{
					completed = false;
					break;
				}
			}
		}

		if (completed)
		{
			auto new_path = ConstructNewTextFilePath(file->name);
			std::ofstream stream{ new_path.string(), std::ios::out | std::ios::trunc };

			for (int i = 0; i < file->siblings->size(); i++)
			{
				auto item = (*file->siblings)[i];
				if (item != nullptr)
				{
					stream << *(item->result) << std::endl;
				}
				else
				{
					auto pdf_path = CreatePdfOutputPath(file->name, i);
					auto output_file = ConstructNewTextFilePath(pdf_path);
					if (!fs::exists(output_file))
					{
						stream.close();
						fs::remove(new_path);
						break;
					}
					std::ifstream ifile(output_file.string(), std::ios::in);
					stream << ifile.rdbuf() << std::endl;
				}
			}
		}
	}
	else
	{
		file->isCompleted = true;
	}
}


fs::path Docapost::IA::Tesseract::MasterProcessingWorker::ConstructNewExifFilePath(fs::path path) const
{
	fs::path new_path;

	auto relative_path = fs::relative(path, mInput);
	auto it = mOutputs.find(OutputFlags::Exif);
	if (it == mOutputs.end() || it->second == mInput)
	{
		return path;
	}
	else
	{
		new_path = fs::absolute(relative_path, it->second);

		if (mOutputTypes & OutputFlags::Flattern)
			new_path = new_path.parent_path() / boost::replace_all_copy(relative_path.string(), "/", mSeparator);
		else
			new_path = new_path.parent_path() / new_path.filename();

	}
	return new_path;
}

bool Docapost::IA::Tesseract::MasterProcessingWorker::FileExist(fs::path path) const
{
	auto new_path = ConstructNewTextFilePath(path);

	if (fs::exists(new_path))
	{
		return true;
	}
	return false;
}

bool Docapost::IA::Tesseract::MasterProcessingWorker::ExifExist(fs::path path) const
{
	auto new_path = ConstructNewExifFilePath(path);

	if (!fs::exists(new_path))
	{
		return false;
	}

	Exiv2::Image::AutoPtr o_image = Exiv2::ImageFactory::open(new_path.string());
	o_image->readMetadata();
	Exiv2::ExifData &exifData = o_image->exifData();

	Exiv2::ExifKey key("Exif.Image.ProcessingSoftware");
	Exiv2::ExifData::iterator pos = exifData.findKey(key);
	if (pos != exifData.end() && pos->value().toString() == mSoftName)
	{
		return true;
	}
	return false;
}

string Docapost::IA::Tesseract::MasterProcessingWorker::CreatePdfOutputPath(fs::path path, int i)
{
	return (boost::format("%s/%s-%d%d") % path.parent_path().string() % fs::change_extension(path.filename(), "").string() % i % mOcrFactory.GetExtension()).str();
}

void Docapost::IA::Tesseract::MasterProcessingWorker::_AddFolder(fs::path folder, bool resume)
{
	if (!fs::is_directory(folder) || mIsTerminated)
	{
		return;
	}

	for (auto& entry : boost::make_iterator_range(fs::directory_iterator(folder), {}))
	{
		fs::path path = entry.path();
		if (path.filename_is_dot() || path.filename_is_dot_dot())
		{
			continue;
		}
		if (fs::is_directory(path))
		{
			this->_AddFolder(path, resume);
			continue;
		}
		auto ext = path.extension().string();
		if (!extensions.count(ext))
		{
			continue;
		}
		if (ext == ".pdf")
		{
			std::vector<MasterFileStatus*>* siblings = nullptr;
			std::mutex* mutex_siblings = new std::mutex();
			try
			{
				MuPDF::MuPDF pdf;
				auto nbPages = pdf.GetNbPage(path.string());

				/*PoDoFo::PdfMemDocument document;
				document.Load(path.string().c_str());
				//PoDoFo::PdfStreamedDocument document(path.string().c_str());
				auto nbPages = document.GetPageCount();*/

				siblings = new std::vector<MasterFileStatus*>(nbPages);

				bool added = false;
				for (int i = 0; i < nbPages; i++)
				{
					auto output_file = CreatePdfOutputPath(path, i);

					bool toProcess = true;
					if (resume)
					{
						toProcess = false;
						if (mOutputTypes & OutputFlags::Text)
						{
							toProcess = toProcess || !FileExist(output_file);
						}

						if (mOutputTypes & OutputFlags::Exif)
						{
							toProcess = toProcess || !ExifExist(output_file);
						}

						if (!toProcess)
						{
							mSkip++;
						}
					}

					if (toProcess)
					{
						mTotal++;
						auto file = new MasterFileStatus(path.string(), fs::relative(path, mInput).string(), output_file);
						file->filePosition = i;
						(*siblings)[i] = file;
						file->siblings = siblings;
						file->mutex_siblings = mutex_siblings;
						if (added == false)
						{
							added = true;
							AddFileBack(file);
						}
					}
				}
				if (added == false)
				{
					delete mutex_siblings;
					delete siblings;
				}
			}
			catch (...)
			{
				auto n = GET_EXCEPTION_NAME;
				//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Impossible de decoder le fichier, le fichier ne sera pas inclue pour traitement " << path.string() << " | " << n;
				std::cerr << "Impossible de decoder le fichier, le fichier ne sera pas inclue pour traitement " << path.string() << " | " << n << std::endl;
			}
		}
		else
		{
			bool toProcess = true;
			if (resume)
			{
				toProcess = false;
				if (mOutputTypes & OutputFlags::Text)
				{
					toProcess = toProcess || !FileExist(path);
				}

				if (mOutputTypes & OutputFlags::Exif)
				{
					toProcess = toProcess || !ExifExist(path);
				}

				if (!toProcess)
				{
					mSkip++;
				}
			}
			if (toProcess)
			{
				mTotal++;
				AddFileBack(new MasterFileStatus(path.string(), fs::relative(path, mInput).string()));
			}
		}
	}
}
void Docapost::IA::Tesseract::MasterProcessingWorker::AddFolder(fs::path folder, bool resume)
{
	ListingThread = new std::thread([this, folder, resume]()
	{
		CatchAllErrorSignals();
		if (mFiles.size() > 0)
			return;
		
		mInput = folder;
		_AddFolder(folder, resume);
		mIsEnd = true;

		if (!mIsTerminated)
		{
			ListingThread->detach();
			delete ListingThread;
		}
	});
}

std::thread* Docapost::IA::Tesseract::MasterProcessingWorker::Run(int nbThread)
{
	mStart = boost::posix_time::second_clock::local_time();

	for (const auto& kv : mOutputs) {
		fs::create_directories(kv.second);
	};
	for (int i = 0; i < nbThread; i++)
	{
		AddThread();
	}
	return nullptr;
}

void Docapost::IA::Tesseract::MasterProcessingWorker::CreateOutput(MasterFileStatus* file, std::string& outText)
{
	fs::path new_path;
	if (mOutputTypes & OutputFlags::Text)
	{
		new_path = ConstructNewTextFilePath(file->new_name);

		fs::create_directories(new_path.parent_path());

		std::ofstream output;
		output.open(new_path.string(), std::ofstream::trunc | std::ofstream::out);
		output << outText;
		output.close();

		file->output.push_back(new_path.string());
		file->relative_output.push_back(fs::relative(new_path, mOutputs[OutputFlags::Text]).string());
	}

	if (mOutputTypes & OutputFlags::Exif)
	{

		Exiv2::ExifData exifData;
		auto v = Exiv2::Value::create(Exiv2::asciiString);
		v->read(outText);
		Exiv2::ExifKey key("Exif.Image.ImageDescription");
		exifData.add(key, v.get());
		exifData["Exif.Image.ImageID"] = fs::relative(file->name, mInput).string();
		exifData["Exif.Image.ProcessingSoftware"] = mSoftName;

		auto o_image = Exiv2::ImageFactory::open(file->buffer->data(), file->buffer->len());
		o_image->setExifData(exifData);
		o_image->writeMetadata();
		auto buffersize = o_image->io().size();
		o_image->io().seek(0, Exiv2::BasicIo::beg);
		auto exif_data = o_image->io().read(buffersize);

		if (!(mOutputTypes & OutputFlags::MemoryImage))
		{
			new_path = ConstructNewExifFilePath(file->new_name);
			auto it = mOutputs.find(OutputFlags::Exif);
			if (it != mOutputs.end() && it->second != mInput)
			{
				fs::create_directories(new_path.parent_path());
				if (fs::exists(new_path))
				{
					fs::remove(new_path);
				}
			}
			std::ofstream stream(new_path.string(), std::ios_base::out | std::ios_base::binary);
			stream.write((char*)exif_data.pData_, buffersize);

			file->output.push_back(new_path.string());
			file->relative_output.push_back(fs::relative(new_path, mOutputs[OutputFlags::Exif]).string());
		}
		else
		{
			delete file->buffer;
			file->buffer = new ExivMemoryFileBuffer(exif_data);
		}
	}
}

void Docapost::IA::Tesseract::MasterProcessingWorker::FreeBuffers(MasterFileStatus* file, int memoryImage, int memoryText)
{
	if (file->buffer == nullptr)
		return;

	if (!memoryImage)
	{
		delete file->buffer;
		file->buffer = nullptr;
	}
	/*if (!memoryText)
	{
		file->result.reset();
	}*/
}

void Docapost::IA::Tesseract::MasterProcessingWorker::ThreadLoop(int id)
{
	CatchAllErrorSignals();

	BaseOcr* ocr = nullptr;
	try
	{
		ocr = mOcrFactory.CreateNew();
	}
	catch (UninitializedOcrException& e)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Thread " << id << " - " << e.message();
	}

	while (ocr != nullptr && !mIsTerminated)
	{
		try
		{
			MasterFileStatus * file = GetFile();
			if (file == nullptr)
			{
				{
					boost::lock_guard<std::mutex> lock(mThreadMutex);
					if (mNbThreadToStop > 0)
					{
						--mNbThreadToStop;
						//mThreads.erase(id);

						break;
					}
				}
				if (mDone == mTotal && mFileSend.size() == 0 && mIsEnd)
				{
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(300));
				continue;
			}
			file->uuid = boost::uuids::uuid();
			file->thread = id;
			onStartProcessFile(file);
			file->start = boost::posix_time::microsec_clock::local_time();

			if (nullptr == ocr->LoadFile(file, [this](MasterFileStatus* file)
			{
				this->AddFile(file);
			}))
			{
				AddFileBack(file);
				continue;
			}

			file->result = ocr->ProcessThroughOcr(file->buffer);
			if (!file->result)
			{
				AddFileBack(file);
				continue;
			}

			CreateOutput(file, *file->result);

			MergeResult(file);

			FreeBuffers(file, mOutputTypes & OutputFlags::MemoryImage, mOutputTypes & OutputFlags::MemoryText);

			mDone++;

			file->end = boost::posix_time::microsec_clock::local_time();
			file->ellapsed = file->end - file->start;
			file->isEnd = true;


			boost::lock_guard<std::mutex> lock(mThreadMutex);
			if (mNbThreadToStop > 0)
			{
				--mNbThreadToStop;
				//mThreads.erase(id);

				break;
			}
		}
		catch (const std::exception& e)
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Thread - " << id << " " << e.what();
		}
		catch (...)
		{
			auto n = GET_EXCEPTION_NAME;
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Thread - " << id << " | " << n;
		}
	}

	try
	{
		// Destroy Ocr class
		std::cerr << "Thread end api " << id << std::endl;
		delete ocr;
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Thread - " << id << " " << e.what();
	}
	catch (...)
	{
		auto n = GET_EXCEPTION_NAME;
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Thread - " << id << " | " << n;
	}

	TerminateThread(id);
}

void Docapost::IA::Tesseract::MasterProcessingWorker::TerminateThread(int id)
{
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Thread " << id << " begin terminate procedure";

	{
		std::unique_lock<std::mutex> lockManagement(mThreadManagementMutex, std::try_to_lock);
		if (lockManagement.owns_lock())
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Thread " << id << " owned, removed from thread list";
			mThreads[id]->detach();
			delete mThreads[id];
		}
		else
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Thread " << id << " not owned, removed and delete delegate to caller";
		}
		mThreads.erase(id);
	}

	boost::lock_guard<std::mutex> lock(mThreadMutex);
	if (mThreads.size() == 0)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Last thread running stopped";
		while (mFileSend.size() > 0)
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Wait for " << mFileSend.size() << " file to be retrieved, retry in 300ms";
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
		}
		mEnd = boost::posix_time::second_clock::local_time();
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Stoping network";
		onProcessEnd();

		std::unique_lock<std::mutex> lk(mIsWorkDoneMutex);
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Notify everyone of the shutdown";
		mIsWorkDone.notify_all();
	}


	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Thread " << id << " terminate";
}

void Docapost::IA::Tesseract::MasterProcessingWorker::SetOutput(boost::unordered_map<OutputFlags, fs::path> folders)
{
	this->mOutputs = folders;
}

Docapost::IA::Tesseract::MasterProcessingWorker::~MasterProcessingWorker()
{
	mIsTerminated = true;
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Destroying MasterProcessingWorker";

	boost::lock_guard<std::mutex> lock(mThreadManagementMutex);
	const auto ths = mThreads;
	for (auto th : ths)
	{
		if(th.second->joinable())
			th.second->join();

		delete th.second;
	}
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Stoping network";
	mNetwork->Stop();
	mNetwork.reset();
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "network Stop";
	if (mNetworkThread != nullptr)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Waiting for network Thread to complete";
		mNetworkThread->join();
		delete mNetworkThread;
	}
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "MasterProcessingWorker Destroyed";
}