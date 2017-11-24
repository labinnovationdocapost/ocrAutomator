#include "TesseractSlaveRunner.h"
#include <leptonica/allheaders.h>
#include "Version.h"
#include "Error.h"


Docapost::IA::Tesseract::TesseractSlaveRunner::TesseractSlaveRunner(int port) :
	threadToRun(0)
{
	setMsgSeverity(L_SEVERITY_NONE);
	int i = 0;
	try
	{
		mNetwork = std::make_shared<NetworkClient>(port);
		mNetwork->onMasterConnected.connect(boost::bind(&TesseractSlaveRunner::OnMasterConnectedHandler, this));
		mNetwork->onMasterDisconnect.connect(boost::bind(&TesseractSlaveRunner::OnMasterDisconnectHandler, this));
		mNetwork->onMasterStatusChanged.connect(boost::bind(&TesseractSlaveRunner::OnMasterStatusChangedHandler, this, _1, _2, _3, _4, _5, _6, _7));
		mNetwork->onMasterSynchro.connect(boost::bind(&TesseractSlaveRunner::OnMasterSynchroHandler, this, _1, _2, _3, _4, _5, _6));
	}
	catch(std::exception& e)
	{
		std::cout << "/!\\ Lancement du réseau impossible sur le port " << port << std::endl;
	}
}

void Docapost::IA::Tesseract::TesseractSlaveRunner::OnMasterConnectedHandler()
{
	mNetwork->SendDeclare(mThreads.size(), VERSION); 
}
void Docapost::IA::Tesseract::TesseractSlaveRunner::OnMasterDisconnectHandler()
{
	//std::cout << "Flux reseau coupe, interuption du programme" << std::endl;
	mNetwork->Stop();
	onProcessEnd();
}
void Docapost::IA::Tesseract::TesseractSlaveRunner::OnMasterStatusChangedHandler(int threadToRun, int done, int skip, int total, int psm, int oem, std::string lang)
{
	this->mDone = done;
	this->mSkip = skip;
	this->mTotal = total;
	this->mPsm = static_cast<tesseract::PageSegMode>(psm);
	this->mOem = static_cast<tesseract::OcrEngineMode>(oem);
	this->mLang = lang;

	/*std::cout << "Status :" << std::endl
		<< "\tDone: " << done
		<< "\tSkip: " << skip
		<< "\tTotal: " << total
		<< "\tPSM: " << psm
		<< "\tOEM: " << oem
		<< "\tLang: " << lang
		<< std::endl;*/

	mStackMutex.lock();
	auto file_buffer = mFiles.size();
	mStackMutex.unlock();
	
	this->threadToRun += threadToRun;

	for (; this->threadToRun > 0; --this->threadToRun)
	{
		int id = mNextId++;
		mThreads[id] = new std::thread(&TesseractSlaveRunner::ThreadLoop, this, id);
	}

	//std::cout << "Nb File ask : " << std::max(0, static_cast<int>(threads.size() * 2 - file_buffer));
	mNetwork->SendSynchro(mThreads.size(), -1, std::max(0, static_cast<int>(mThreads.size() * 2 - file_buffer)),std::vector<SlaveFileStatus*>());

	//std::cout << "connected" << std::endl;
}
void Docapost::IA::Tesseract::TesseractSlaveRunner::OnMasterSynchroHandler(int thread, int done, int skip, int total, bool end, boost::unordered_map<std::string, std::vector<unsigned char>*> files)
{
	this->mSkip = skip;
	this->mTotal = total;
	this->mDone = done;
	this->mIsEnd = end;
	//std::cout << "Synchro : " << files.size() << std::endl;
	for(auto& file : files)
	{
		//std::cout << "File : " << file.first << " | " << file.second->size() << std::endl;
		auto f = new SlaveFileStatus(file.first, file.second);
		f->fileSize = file.second->size();
		AddFile(f);
		onStartProcessFile(f);
	}

	mStackMutex.lock();
	auto file_buffer = files.size();
	mStackMutex.unlock();

	if(done == total && mIsEnd)
	{
		onProcessEnd();
	}
	else if(files.size() == 0 && file_buffer == 0 && mNetwork->GetSynchroWaiting() == 0)
	{
		//std::this_thread::sleep_for(std::chrono::milliseconds(300));
		mNetwork->SendSynchroIfNone(mThreads.size(), -1, std::max(0, static_cast<int>(mThreads.size() * 2 - file_buffer)), std::vector<SlaveFileStatus*>());
	}
}


bool Docapost::IA::Tesseract::TesseractSlaveRunner::GetTextFromTesseract(tesseract::TessBaseAPI * api, std::vector<unsigned char>* imgData, std::string & text)
{
	Pix *image = pixReadMem(imgData->data(), imgData->size());
	if (image == nullptr)
	{
		std::ofstream fs{"wronfile.jpg", std::_Ios_Openmode::_S_bin | std::_Ios_Openmode::_S_trunc | std::_Ios_Openmode::_S_out };
		std::copy(imgData->begin(), imgData->end(), std::ostreambuf_iterator<char>(fs));
		return false;
	}

	api->SetImage(image);
	auto outtext = api->GetUTF8Text();

	text = string(outtext);

	pixDestroy(&image);
	delete outtext;
	delete imgData;

	return true;
}

void Docapost::IA::Tesseract::TesseractSlaveRunner::ThreadLoop(int id)
{
	CatchAllErrorSignals();
	auto api = new tesseract::TessBaseAPI();
	api->SetVariable("debug_file", "/dev/null");
	api->SetVariable("out", "quiet");
	int res;

	if ((res = api->Init(nullptr, mLang.c_str(), mOem))) {
		std::stringstream cstring;
		cstring << "Thread " << id << " - " << "Could not initialize tesseract\n";
		std::cerr << cstring.str();
		return;
	}
	api->SetPageSegMode(mPsm);

	try
	{
		while (mNetwork != nullptr && mNetwork->IsOpen())
		{
			SlaveFileStatus * file = GetFile();
			if(file == nullptr)
			{
				mNetwork->SendSynchroIfNone(mThreads.size(), -1, std::max(0, static_cast<int>(mThreads.size() * 2)), std::vector<SlaveFileStatus*>());
				std::this_thread::sleep_for(std::chrono::milliseconds(300));
				continue; 
			}
			file->thread = id;
			file->start = boost::posix_time::microsec_clock::local_time();
			
			std::string outText;
			//std::cout << "Start Tesseractisation : " << file->uuid << std::endl;
			if (!GetTextFromTesseract(api, file->data, file->result))
			{
				std::cout << "Get Tesseract Error" << std::endl;
				//AddFile(file);

				//TODO

				continue;
			}
			//std::cout << "End Tesseractisation : " << file->uuid << std::endl;

			mDone++;

			file->end = boost::posix_time::microsec_clock::local_time();
			file->ellapsed = file->end - file->start;
			file->isEnd = true;

			auto list = std::vector<SlaveFileStatus*>();
			list.push_back(file);

			boost::lock_guard<std::mutex> lock(mThreadMutex);
			if (mNbThreadToStop > 0)
			{
				--mNbThreadToStop;
				mThreads.erase(id);

				mNetwork->SendSynchro(mThreads.size(), id, 1, list);
				break;
			}
			mNetwork->SendSynchro(mThreads.size(), id, 1, list);
		}

		api->End();
		delete api;
	}
	catch (const std::exception& e)
	{
		//std::cout << "Thread - " << id << " " << e.what();
	}

	mThreads.erase(id);
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	if (mThreads.size() == 0)
	{
		mNetwork->SendSynchro(mThreads.size(), id, 1, std::vector<SlaveFileStatus*>());
		mEnd = boost::posix_time::second_clock::local_time();
		onProcessEnd();
	}
}

std::thread* Docapost::IA::Tesseract::TesseractSlaveRunner::Run(int nbThread)
{
	//mThreadMutex.lock();
	mStart = boost::posix_time::second_clock::local_time();

	this->threadToRun = nbThread;
	return new std::thread([this]()
	{
		CatchAllErrorSignals();
		if (mNetwork != nullptr) mNetwork->Start();
	});

	/*std::cout << "waiting connection" << std::endl;
	std::thread([this]() {
		mThreadMutex.lock();
	}).join();*/
}



Docapost::IA::Tesseract::TesseractSlaveRunner::~TesseractSlaveRunner()
{
	if (mNetwork != nullptr) mNetwork.reset();
}
