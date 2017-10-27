#include "TesseractSlaveRunner.h"
#include <leptonica/allheaders.h>
#include "Version.h"

std::atomic_int Docapost::IA::Tesseract::TesseractSlaveRunner::next_id{ 0 };

Docapost::IA::Tesseract::TesseractSlaveRunner::TesseractSlaveRunner() :
	threadToRun(0)
{
	setMsgSeverity(L_SEVERITY_NONE);
	int i = 0;
	while(true)
	{
		try
		{
			network = std::make_shared<NetworkClient>(12000 + i++);
			break;
		}
		catch(std::exception& e)
		{
			
		}
	}
	network->onMasterConnected.connect(boost::bind(&TesseractSlaveRunner::OnMasterConnectedHandler, this));
	network->onMasterDisconnect.connect(boost::bind(&TesseractSlaveRunner::OnMasterDisconnectHandler, this));
	network->onMasterStatusChanged.connect(boost::bind(&TesseractSlaveRunner::OnMasterStatusChangedHandler, this, _1, _2, _3, _4, _5, _6, _7));
	network->onMasterSynchro.connect(boost::bind(&TesseractSlaveRunner::OnMasterSynchroHandler, this, _1, _2, _3, _4, _5, _6));
}

void Docapost::IA::Tesseract::TesseractSlaveRunner::OnMasterConnectedHandler()
{
	network->SendDeclare(threads.size(), VERSION); 
}
void Docapost::IA::Tesseract::TesseractSlaveRunner::OnMasterDisconnectHandler()
{
	//std::cout << "Flux reseau coupe, interuption du programme" << std::endl;
	network->Stop();
	onProcessEnd();
}
void Docapost::IA::Tesseract::TesseractSlaveRunner::OnMasterStatusChangedHandler(int threadToRun, int done, int skip, int total, int psm, int oem, std::string lang)
{
	this->done = done;
	this->skip = skip;
	this->total = total;
	this->psm = static_cast<tesseract::PageSegMode>(psm);
	this->oem = static_cast<tesseract::OcrEngineMode>(oem);
	this->lang = lang;

	/*std::cout << "Status :" << std::endl
		<< "\tDone: " << done
		<< "\tSkip: " << skip
		<< "\tTotal: " << total
		<< "\tPSM: " << psm
		<< "\tOEM: " << oem
		<< "\tLang: " << lang
		<< std::endl;*/

	g_stack_mutex.lock();
	auto file_buffer = files.size();
	g_stack_mutex.unlock();
	
	this->threadToRun += threadToRun;

	for (; this->threadToRun > 0; --this->threadToRun)
	{
		int id = next_id++;
		threads[id] = new std::thread(&TesseractSlaveRunner::ThreadLoop, this, id);
	}

	//std::cout << "Nb File ask : " << std::max(0, static_cast<int>(threads.size() * 2 - file_buffer));
	network->SendSynchro(threads.size(), -1, std::max(0, static_cast<int>(threads.size() * 2 - file_buffer)),std::vector<SlaveFileStatus*>());

	//std::cout << "connected" << std::endl;
}
void Docapost::IA::Tesseract::TesseractSlaveRunner::OnMasterSynchroHandler(int thread, int done, int skip, int total, bool end, boost::unordered_map<std::string, std::vector<unsigned char>*> files)
{
	this->skip = skip;
	this->total = total;
	this->done = done;
	this->isEnd = end;
	//std::cout << "Synchro : " << files.size() << std::endl;
	for(auto& file : files)
	{
		//std::cout << "File : " << file.first << " | " << file.second->size() << std::endl;
		auto f = new SlaveFileStatus(file.first, file.second);
		f->fileSize = file.second->size();
		AddFile(f);
		onStartProcessFile(f);
	}

	g_stack_mutex.lock();
	auto file_buffer = files.size();
	g_stack_mutex.unlock();

	if(done == total && isEnd)
	{
		onProcessEnd();
	}
	else if(files.size() == 0 && file_buffer == 0 && network->GetSynchroWaiting() == 0)
	{
		//std::this_thread::sleep_for(std::chrono::milliseconds(300));
		network->SendSynchroIfNone(threads.size(), -1, std::max(0, static_cast<int>(threads.size() * 2 - file_buffer)), std::vector<SlaveFileStatus*>());
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
	auto api = new tesseract::TessBaseAPI();
	api->SetVariable("debug_file", "/dev/null");
	api->SetVariable("out", "quiet");
	int res;

	if ((res = api->Init(nullptr, lang.c_str(), oem))) {
		std::stringstream cstring;
		cstring << "Thread " << id << " - " << "Could not initialize tesseract\n";
		std::cerr << cstring.str();
		return;
	}
	api->SetPageSegMode(psm);

	try
	{
		while (network->IsOpen())
		{
			SlaveFileStatus * file = GetFile();
			if(file == nullptr)
			{
				network->SendSynchroIfNone(threads.size(), -1, std::max(0, static_cast<int>(threads.size() * 2)), std::vector<SlaveFileStatus*>());
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

			done++;

			file->end = boost::posix_time::microsec_clock::local_time();
			file->ellapsed = file->end - file->start;
			file->isEnd = true;

			auto list = std::vector<SlaveFileStatus*>();
			list.push_back(file);

			boost::lock_guard<std::mutex> lock(g_thread_mutex);
			if (threadToStop > 0)
			{
				--threadToStop;
				threads.erase(id);

				network->SendSynchro(threads.size(), id, 1, list);
				break;
			}
			network->SendSynchro(threads.size(), id, 1, list);
		}

		api->End();
		delete api;
	}
	catch (const std::exception& e)
	{
		//std::cout << "Thread - " << id << " " << e.what();
	}

	threads.erase(id);
	boost::lock_guard<std::mutex> lock(g_thread_mutex);
	if (threads.size() == 0)
	{
		network->SendSynchro(threads.size(), id, 1, std::vector<SlaveFileStatus*>());
		end = boost::posix_time::second_clock::local_time();
		onProcessEnd();
	}
}


SlaveFileStatus* Docapost::IA::Tesseract::TesseractSlaveRunner::GetFile()
{
	while (true)
	{
		g_stack_mutex.lock();
		if (files.empty())
		{
			g_stack_mutex.unlock();
			return nullptr;
		}

		auto f = files.front();

		files.pop();

		g_stack_mutex.unlock();
		return f;
	}
}

std::thread* Docapost::IA::Tesseract::TesseractSlaveRunner::Run(int nbThread)
{
	//g_thread_mutex.lock();
	start = boost::posix_time::second_clock::local_time();

	this->threadToRun = nbThread;
	return new std::thread([this]()
	{
		network->Start(12000);
	});

	/*std::cout << "waiting connection" << std::endl;
	std::thread([this]() {
		g_thread_mutex.lock();
	}).join();*/
}

void Docapost::IA::Tesseract::TesseractSlaveRunner::AddFile(SlaveFileStatus* file)
{
	g_stack_mutex.lock();
	files.push(file);

	g_stack_mutex.unlock();
}
void Docapost::IA::Tesseract::TesseractSlaveRunner::Wait()
{
	for (auto& th : threads)
		th.second->join();
}

Docapost::IA::Tesseract::TesseractSlaveRunner::~TesseractSlaveRunner()
{
	network.reset();
}

void Docapost::IA::Tesseract::TesseractSlaveRunner::AddThread()
{
	int id = next_id++;
	threads[id] = new std::thread(&TesseractSlaveRunner::ThreadLoop, this, id);
}

void Docapost::IA::Tesseract::TesseractSlaveRunner::RemoveThread()
{
	boost::lock_guard<std::mutex> lock(g_thread_mutex);

	if (threads.size() > threadToStop + 1)
		threadToStop++;
}