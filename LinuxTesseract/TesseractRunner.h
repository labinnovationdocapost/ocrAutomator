#pragma once

#if __cplusplus <= 199711L
#error This library needs at least a C++11 compliant compiler
#endif

#include <cstdio>

#include <string>
#include <stack>
#include <iostream>
#include <mutex>
#include <thread>
#include <fstream>
#include <atomic>
#include "FileStatus.h"

#include <boost/program_options.hpp>
#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
using std::string;
#include <tesseract/baseapi.h>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using boost::program_options::value;


namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class TesseractRunner {
			private:
				static std::atomic_int next_id;

				std::stack<FileStatus*> files;
				std::mutex g_stack_mutex;
				std::mutex g_thread_mutex;
				std::map<int, std::thread*> threads;

				int threadToStop = 0;

				int total;
				int skip;

				tesseract::PageSegMode psm;
				tesseract::OcrEngineMode oem;
				std::string lang;
				fs::path output;
				fs::path input;
				boost::posix_time::ptime start;
				boost::posix_time::ptime end;

				void ThreadLoop(int id);
				FileStatus* GetFile();
				void _AddFolder(fs::path folder, bool resume);
			public:
				explicit TesseractRunner(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang = "fra");
				TesseractRunner(std::string lang = "fra") : TesseractRunner(tesseract::PSM_OSD_ONLY, tesseract::OcrEngineMode::OEM_TESSERACT_ONLY, lang) {}
				explicit TesseractRunner(tesseract::PageSegMode psm, std::string lang = "fra") : TesseractRunner(psm, tesseract::OcrEngineMode::OEM_TESSERACT_ONLY, lang) {}
				explicit TesseractRunner(tesseract::OcrEngineMode ocr, std::string lang = "fra") : TesseractRunner(tesseract::PSM_OSD_ONLY, ocr, lang) {}

				void AddFolder(fs::path folder, bool resume = false);
				void Run(int nbThread);
				void AddThread();
				void RemoveThread();
				void Wait();

				void SetOutput(std::string folder);

				void DisplayFiles() const;

				std::vector<std::string> extensions = { ".tif", ".tiff", ".png", ".jpg", ".jpeg" };

				boost::signals2::signal<void(FileStatus*)> onStartProcessFile;
				boost::signals2::signal<void()> onProcessEnd;

				int GetNbFiles() const { return total; }
				int GetNbSkipFiles() const { return skip; }
				fs::path GetInput() const { return input; }
				fs::path GetOutput() const { return output; }
				tesseract::PageSegMode GetPSM() const { return psm; }
				tesseract::OcrEngineMode GetOEM() const { return oem; }
				std::size_t GetNbThread() {
					boost::lock_guard<std::mutex> lock(g_thread_mutex); 
					return threads.size(); 
				}
				boost::posix_time::ptime GetStartTime() const { return start; }
				boost::posix_time::ptime GetEndTime() const { return end; }
				int GetThreadToStop() const { return threadToStop; }
			};
		}
	}
}