#include "SlaveProcessingWorker.h"
#include <leptonica/allheaders.h>
#include "Version.h"
#include "Error.h"
#include "TesseractFactory.h"
#include <future>


Docapost::IA::Tesseract::SlaveProcessingWorker::SlaveProcessingWorker(OcrFactory& ocr, int port, std::string ip) :
	BaseProcessingWorker(ocr),
	threadToRun(0),
	mIp(ip)
{
	setMsgSeverity(L_SEVERITY_NONE);
	int i = 0;
	try
	{
		mNetwork = std::make_shared<NetworkClient>(port, ip);
		mNetwork->onMasterConnected.connect(boost::bind(&SlaveProcessingWorker::OnMasterConnectedHandler, this));
		mNetwork->onMasterDisconnect.connect(boost::bind(&SlaveProcessingWorker::OnMasterDisconnectHandler, this));
		mNetwork->onMasterStatusChanged.connect(boost::bind(&SlaveProcessingWorker::OnMasterStatusChangedHandler, this, _1, _2, _3, _4, _5, _6, _7));
		mNetwork->onMasterSynchro.connect(boost::bind(&SlaveProcessingWorker::OnMasterSynchroHandler, this, _1, _2, _3, _4, _5, _6, _7));
	}
	catch (std::exception& e)
	{
		std::cout << "/!\\ Lancement du réseau impossible sur le port " << port << std::endl;
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Network run failed " << e.what();
	}
}

void Docapost::IA::Tesseract::SlaveProcessingWorker::OnMasterConnectedHandler()
{ 
	mNetwork->SendDeclare(mThreads.size(), VERSION);
}
void Docapost::IA::Tesseract::SlaveProcessingWorker::OnMasterDisconnectHandler()
{
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "OnMasterDisconnectHandler Start";
	boost::lock_guard<std::mutex> lock(mStackMutex);
	mPending = 0;
	mFiles.clear();

	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "joining";
	mNetworkThread->join();

	if(mNetwork)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "mNetwork != nullptr";
		mNetwork->Reconnect();
	}
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "OnMasterDisconnectHandler End";
	//onProcessEnd();
}
void Docapost::IA::Tesseract::SlaveProcessingWorker::OnMasterStatusChangedHandler(int threadToRun, int done, int skip, int total, int psm, int oem, std::string lang)
{
	this->mDone = done;
	this->mSkip = skip;
	this->mTotal = total;

	//****************************************************************
	// TODO
	//****************************************************************
	dynamic_cast<TesseractFactory&>(mOcrFactory).Psm(static_cast<tesseract::PageSegMode>(psm));
	dynamic_cast<TesseractFactory&>(mOcrFactory).Oem(static_cast<tesseract::OcrEngineMode>(oem));
	dynamic_cast<TesseractFactory&>(mOcrFactory).Lang(lang);

	this->threadToRun += threadToRun;

	for (; this->threadToRun > 0; --this->threadToRun)
	{
		AddThread();
	}

	mNetworkThread = new boost::thread(&SlaveProcessingWorker::NetwordLoop, this);

	onNewBatch();
	//mNetworkThread->detach();

}
void Docapost::IA::Tesseract::SlaveProcessingWorker::OnMasterSynchroHandler(int thread, int done, int skip, int total, bool end, int pending, boost::unordered_map<std::string, MemoryFileBuffer*> files)
{
	this->mSkip = skip;
	this->mTotal = total;
	this->mDone = done;
	this->mIsEnd = end;

	/*if (files.size() + pending != mPending)
	{
		std::cout << "Pending error : " << files.size() << " : " << pending << " | " << mPending << std::endl;
	}*/

	for (auto& file : files)
	{
		auto f = new SlaveFileStatus(file.first, file.second);
		AddFileBack(f);
		onStartProcessFile(f);
	}

	if (done == total && mIsEnd)
	{
		onProcessEnd();
	}


	mPending -= files.size();
	//mPending = pending;
}

void Docapost::IA::Tesseract::SlaveProcessingWorker::AddFileToSend(SlaveFileStatus* file)
{
	boost::lock_guard<std::mutex> lock(mStackToSendMutex);
	mFilesToSend.push_back(file);
}

SlaveFileStatus* Docapost::IA::Tesseract::SlaveProcessingWorker::GetFileToSend()
{
	boost::lock_guard<std::mutex> lock(mStackToSendMutex);
	if (mFilesToSend.empty())
	{
		return nullptr;
	}

	auto f = mFilesToSend.front();

	mFilesToSend.pop_front();

	return f; 
}

void Docapost::IA::Tesseract::SlaveProcessingWorker::NetwordLoop()
{
	CatchAllErrorSignals();

	while (mNetwork != nullptr && mNetwork->IsOpen())
	{
		std::vector<SlaveFileStatus*> toSend;
		SlaveFileStatus * file;
		while ((file = GetFileToSend()) != nullptr)
		{
			toSend.push_back(file);
		}

		auto ask = std::max(0, static_cast<int>(mThreads.size() * 2 - mFiles.size() - mPending));
		mPending += ask;

		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::debug) << "Ask " << ask << " file(s)";
		mNetwork->SendSynchro(mThreads.size(), -1, ask, toSend);

		for (auto fileSend : toSend)
		{
			fileSend->result.~basic_string();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

void Docapost::IA::Tesseract::SlaveProcessingWorker::ThreadLoop(int id)
{
	CatchAllErrorSignals();

	auto ocr = mOcrFactory.CreateNew();

	try
	{
		while (mAsioThread != nullptr)
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Thread ASIO: " << (mAsioThread != nullptr);
			SlaveFileStatus * file = GetFile(); 
			if (file == nullptr)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				continue;
			}
			file->thread = id;
			file->start = boost::posix_time::microsec_clock::local_time();

			std::string outText;
			if (!ocr->ProcessThroughOcr(file->buffer, file->result))
			{
				std::cerr << "Get Tesseract Error" << std::endl;

				continue;
			}

			delete file->buffer;

			mDone++;

			file->end = boost::posix_time::microsec_clock::local_time();
			file->ellapsed = file->end - file->start;
			file->isEnd = true;

			AddFileToSend(file);

			boost::lock_guard<std::mutex> lock(mThreadMutex);

			if (mNbThreadToStop > 0)
			{
				--mNbThreadToStop;
				break;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Thread - " << id << " " << e.what();
	}
	delete ocr;

	TerminateThread(id);
}

void Docapost::IA::Tesseract::SlaveProcessingWorker::TerminateThread(int id)
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);

	mThreads[id]->detach();
	delete mThreads[id];
	mThreads.erase(id);

	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Thread " << id << " Terminated";

	if (mThreads.size() == 0)
	{
		mEnd = boost::posix_time::second_clock::local_time();
		/*mNetwork->Stop();
		mNetworkThread->join();
		delete mNetworkThread;*/
		onProcessEnd();

		std::unique_lock<std::mutex> lk(mIsWorkDoneMutex);
		mIsWorkDone.notify_all();

		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "All Thread Terminated";
	}
}

std::thread* Docapost::IA::Tesseract::SlaveProcessingWorker::Run(int nbThread)
{
	mStart = boost::posix_time::second_clock::local_time();

	this->threadToRun = nbThread;
	return mAsioThread = new std::thread([this]()
	{
		CatchAllErrorSignals();
		while(mNetwork != nullptr)
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Starting Service";
			mNetwork->Start();
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	});
}



Docapost::IA::Tesseract::SlaveProcessingWorker::~SlaveProcessingWorker()
{ 
	auto ptr = mNetwork;
	if (mNetwork)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Disconnect Signals";
		onProcessEnd.disconnect_all_slots();
		onStartProcessFile.disconnect_all_slots();

		mNetwork.reset();

		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Network Stop & Reset";
		ptr->Stop();
		ptr.reset();
		

		/*BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Destructor Join" << " file(s)";
		mNetworkThread->join();*/
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Delete Network thread";
		if(mNetworkThread != nullptr)
		{
			if (mNetworkThread->joinable())
				mNetworkThread->join();

			delete mNetworkThread;
		}


		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Join : " << mAsioThread->joinable();
		if (mAsioThread->joinable())
		{
			mAsioThread->join();
			delete mAsioThread;
			mAsioThread = nullptr;
		}


		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "End" << " file(s)";
	}
}
