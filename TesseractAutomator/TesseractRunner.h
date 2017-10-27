#pragma once

#if __cplusplus <= 199711L
#error This library needs at least a C++11 compliant compiler
#endif

#include <cstdio>

#include <string>
#include <queue>
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
#include <boost/unordered_map.hpp>
using std::string;
#include <tesseract/baseapi.h>
#include "Network.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using boost::program_options::value;


namespace Docapost {
	namespace IA {
		namespace Tesseract {
			enum TesseractOutputFlags
			{
				Flattern = 4,
				Exif = 2,
				Text = 1,
				None = 0
			};
			inline TesseractOutputFlags operator|(TesseractOutputFlags a, TesseractOutputFlags b)
			{
				return static_cast<TesseractOutputFlags>(static_cast<int>(a) | static_cast<int>(b));
			}
			inline TesseractOutputFlags& operator|=(TesseractOutputFlags& a, TesseractOutputFlags b)
			{
				return a = a | b;
			}

			class TesseractRunner {
			private:
				const std::string softName = "Docapost Tesseract Automator";

				static std::atomic_int next_id;

				std::deque<FileStatus*> files;
				std::mutex g_stack_mutex;
				std::mutex g_thread_mutex;
				std::mutex g_network_mutex;
				std::map<int, std::thread*> threads;

				int threadToStop = 0;

				TesseractOutputFlags outputTypes;

				fs::path input;
				boost::unordered_map<TesseractOutputFlags, fs::path> outputs;

				int total;
				int skip;
				int done;

				bool isEnd = false;

				tesseract::PageSegMode psm;
				tesseract::OcrEngineMode oem;
				std::string lang;
				std::string separator = "__";
				boost::posix_time::ptime start;
				boost::posix_time::ptime end;


				void ThreadLoop(int id);
				void AddFile(FileStatus* file);
				void AddFileBack(FileStatus* file);
				FileStatus* GetFile();
				void _AddFolder(fs::path folder, bool resume);
				bool FileExist(fs::path path)  const;
				bool ExifExist(fs::path path)  const;
				fs::path ConstructNewExifFilePath(fs::path path) const;
				fs::path ConstructNewTextFilePath(fs::path path) const;

				bool GetTextFromTesseract(tesseract::TessBaseAPI* api, std::vector<unsigned char>* image, std::string& text);
				std::vector<unsigned char>* OpenFileForLeptonica(FileStatus* file);
				void CreateOutput(FileStatus* file, std::string outText);

				boost::unordered_map<std::string, FileStatus*> fileSend;

				boost::uuids::basic_random_generator<boost::mt19937> gen = boost::uuids::basic_random_generator<boost::mt19937>();
				boost::shared_ptr<Network> network;

				boost::unordered_map<std::string, int> slaves;

				std::thread* networkThread;
			public:
				explicit TesseractRunner(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang = "fra", TesseractOutputFlags type = TesseractOutputFlags::None);
				TesseractRunner(std::string lang = "fra") : TesseractRunner(tesseract::PSM_OSD_ONLY, tesseract::OcrEngineMode::OEM_TESSERACT_ONLY, lang) {}
				explicit TesseractRunner(tesseract::PageSegMode psm, std::string lang = "fra") : TesseractRunner(psm, tesseract::OcrEngineMode::OEM_TESSERACT_ONLY, lang) {}
				explicit TesseractRunner(tesseract::OcrEngineMode ocr, std::string lang = "fra") : TesseractRunner(tesseract::PSM_OSD_ONLY, ocr, lang) {}

				void OnSlaveConnectHandler(NetworkSession* ns, int thread, std::string hostname);
				void OnSlaveDisconnectHandler(NetworkSession* ns, boost::unordered_map<std::string, bool>& noUsed);
				void OnSlaveSynchroHandler(NetworkSession* ns, int thread, int required, std::vector<std::tuple<std::string, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::string>>& results);

				void AddFolder(fs::path folder, bool resume = false);
				void Run(int nbThread);
				void AddThread();
				void RemoveThread();
				void Wait();

				void SetOutput(boost::unordered_map<TesseractOutputFlags, fs::path> folders);

				void DisplayFiles() const;

				std::set<std::string> extensions = { ".tif", ".tiff", ".png", ".jpg", ".jpeg", ".pdf" };

				boost::signals2::signal<void(FileStatus*)> onFileCanceled;
				boost::signals2::signal<void(FileStatus*)> onStartProcessFile;
				boost::signals2::signal<void()> onProcessEnd;

				void SetSeparator(std::string separator) { this->separator = separator; }
				std::string GetSeparator() const { return separator; }
				TesseractOutputFlags GetOutputTypes() const { return outputTypes; }
				int GetNbFiles() const { return total; }
				int GetDone() const { return done; }
				int GetNbSkipFiles() const { return skip; }
				fs::path GetInput() const { return input; }
				boost::unordered_map<TesseractOutputFlags, fs::path> GetOutput() const { return outputs; }
				tesseract::PageSegMode GetPSM() const { return psm; }
				tesseract::OcrEngineMode GetOEM() const { return oem; }
				std::size_t GetNbThread() {
					boost::lock_guard<std::mutex> lock(g_thread_mutex);
					return threads.size();
				}
				boost::posix_time::ptime GetStartTime() const { return start; }
				boost::posix_time::ptime GetEndTime() const { return end; }
				int GetThreadToStop() const { return threadToStop; }
				int GetRemoteThread() const
				{
					int total = 0;
					for (auto& slave : slaves)
						total += slave.second;
					return total;
				}
				boost::unordered_map<std::string, int> GetSlaves() const {
					return slaves;
				}
				~TesseractRunner();
			};
		}
	}
}