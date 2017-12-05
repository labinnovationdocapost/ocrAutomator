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
	}
}

void Docapost::IA::Tesseract::SlaveProcessingWorker::OnMasterConnectedHandler()
{
	mNetwork->SendDeclare(mThreads.size(), VERSION);
}
void Docapost::IA::Tesseract::SlaveProcessingWorker::OnMasterDisconnectHandler()
{
	std::cerr << "Flux reseau coupe, interuption du programme" << std::endl;
	mNetwork->Stop();
	onProcessEnd();
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


	mStackMutex.lock();
	auto file_buffer = mFiles.size();
	mStackMutex.unlock();

	this->threadToRun += threadToRun;

	for (; this->threadToRun > 0; --this->threadToRun)
	{
		int id = mNextId++;
		mThreads[id] = new std::thread(&SlaveProcessingWorker::ThreadLoop, this, id);
	}

	mNetworkThread = new std::thread(&SlaveProcessingWorker::NetwordLoop, this);

}
void Docapost::IA::Tesseract::SlaveProcessingWorker::OnMasterSynchroHandler(int thread, int done, int skip, int total, bool end, int pending, boost::unordered_map<std::string, std::vector<unsigned char>*> files)
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
		f->fileSize = file.second->size();
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
		mNetwork->SendSynchro(mThreads.size(), -1, ask, toSend);


		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

void Docapost::IA::Tesseract::SlaveProcessingWorker::ThreadLoop(int id)
{
	CatchAllErrorSignals();

	auto ocr = mOcrFactory.CreateNew();

	try
	{
		while (mNetwork != nullptr && mNetwork->IsOpen())
		{
			SlaveFileStatus * file = GetFile();
			if (file == nullptr)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				continue;
			}
			file->thread = id;
			file->start = boost::posix_time::microsec_clock::local_time();

			std::string outText;
			if (!ocr->ProcessThroughOcr(file->data, file->result))
			{
				std::cerr << "Get Tesseract Error" << std::endl;

				continue;
			}

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

		ocr.reset();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Thread - " << id << " " << e.what();
	}

	TerminateThread(id);
}

void Docapost::IA::Tesseract::SlaveProcessingWorker::TerminateThread(int id)
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);

	mThreads[id]->detach();
	delete mThreads[id];
	mThreads.erase(id);

	std::cerr << "Thread remove from list " << id << std::endl;

	if (mThreads.size() == 0)
	{
		mEnd = boost::posix_time::second_clock::local_time();
		mNetwork->Stop();
		mNetworkThread->join();
		delete mNetworkThread;
		onProcessEnd();

		std::unique_lock<std::mutex> lk(mIsWorkDoneMutex);
		mIsWorkDone.notify_all();
	}
}

std::thread* Docapost::IA::Tesseract::SlaveProcessingWorker::Run(int nbThread)
{
	mStart = boost::posix_time::second_clock::local_time();

	this->threadToRun = nbThread;
	return new std::thread([this]()
	{
		CatchAllErrorSignals();
		if (mNetwork != nullptr) mNetwork->Start();
	});
}



Docapost::IA::Tesseract::SlaveProcessingWorker::~SlaveProcessingWorker()
{
	if (mNetwork != nullptr) mNetwork.reset();
}
