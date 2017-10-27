#pragma once
#include <string>
#include <stack>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <map>
#include <thread>
#include <boost/date_time.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
using std::string;
#include <tesseract/baseapi.h>

#include "FileStatus.h"
#include "NetworkClient.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class TesseractSlaveRunner
			{
			private:
				static std::atomic_int next_id;
				std::mutex g_stack_mutex;
				std::mutex g_thread_mutex;
				std::map<int, std::thread*> threads;

				int threadToStop = 0;
				std::atomic<int> threadToRun;

				std::queue<SlaveFileStatus*> files;

				int total = 0;
				int skip = 0;
				int done = 0;

				bool isEnd = false;

				tesseract::PageSegMode psm = tesseract::PageSegMode::PSM_AUTO;
				tesseract::OcrEngineMode oem = tesseract::OcrEngineMode::OEM_DEFAULT;
				std::string lang;

				boost::posix_time::ptime start;
				boost::posix_time::ptime end;

				std::shared_ptr<NetworkClient> network;

				bool GetTextFromTesseract(tesseract::TessBaseAPI* api, std::vector<unsigned char>* image, std::string& text);
				void ThreadLoop(int id);
				SlaveFileStatus* GetFile();
				void AddFile(SlaveFileStatus* file);

				void OnMasterConnectedHandler();
				void OnMasterDisconnectHandler();
				void OnMasterStatusChangedHandler(int threadToRun, int done, int skip, int total, int psm, int oem, std::string lang);
				void OnMasterSynchroHandler(int thread, int done, int skip, int total, bool end, boost::unordered_map<std::string, std::vector<unsigned char>*> files);
			public:
				boost::signals2::signal<void(SlaveFileStatus*)> onStartProcessFile;
				boost::signals2::signal<void()> onProcessEnd;

				TesseractSlaveRunner();
				~TesseractSlaveRunner();

				std::thread* Run(int nbThread);
				void Wait();
				void AddThread();
				void RemoveThread();
				int GetNbFiles() const { return total; }
				int GetDone() const { return done; }
				int GetNbSkipFiles() const { return skip; }
				int GetThreadToStop() const { return threadToStop; }
				tesseract::PageSegMode GetPSM() const { return psm; }
				tesseract::OcrEngineMode GetOEM() const { return oem; }
				std::size_t GetNbThread() {
					boost::lock_guard<std::mutex> lock(g_thread_mutex);
					return threads.size();
				}

				std::string remote_address() const { return network->GetRemoteAddress(); }
				bool remote_isconnected() const { return network->IsOpen(); }
			};
		}
	}
}
