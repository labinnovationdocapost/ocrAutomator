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

using std::string;
#include <tesseract/baseapi.h>

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <boost/program_options.hpp>
#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>

namespace po = boost::program_options;
using boost::program_options::value;

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class TesseractRunner {
			private:
				static int next_id;

				std::stack<std::string> files;
				std::string folder;
				std::mutex g_stack_mutex;
				std::vector<boost::shared_ptr<std::thread>> threads;

				int total;
				int current;

				tesseract::PageSegMode psm;
				tesseract::OcrEngineMode oem;
				std::string lang;
				boost::filesystem::path output;
				boost::filesystem::path input;

				void ThreadLoop();
				std::string GetFile();
				void _AddFolder(boost::filesystem::path folder);
			public :
				explicit TesseractRunner(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang = "fra");
				TesseractRunner(std::string lang = "fra") : TesseractRunner(tesseract::PSM_OSD_ONLY, tesseract::OcrEngineMode::OEM_TESSERACT_ONLY, lang) {}
				explicit TesseractRunner(tesseract::PageSegMode psm, std::string lang = "fra") : TesseractRunner(psm, tesseract::OcrEngineMode::OEM_TESSERACT_ONLY, lang) {}
				explicit TesseractRunner(tesseract::OcrEngineMode ocr, std::string lang = "fra") : TesseractRunner(tesseract::PSM_OSD_ONLY, ocr, lang) {}

				void AddFolder(boost::filesystem::path folder);
				void Run(int nbThread);
				void Wait();

				void SetOutput(std::string folder);

				void DisplayFiles() const;

				std::vector<std::string> extensions = { ".tif", ".tiff", ".png", ".jpg", ".jpeg" };
			};
		}
	}
}