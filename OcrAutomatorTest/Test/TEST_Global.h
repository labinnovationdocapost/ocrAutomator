#pragma once
#include "Main.h"
#include "Master/MasterProcessingWorker.h"
#include "Master/MasterLocalFileStatus.h"
#include <mutex>

extern std::vector<MasterFileStatus*> files;
extern std::vector<SlaveFileStatus*> filesSlave;
extern std::mutex mtx;


void file_received(MasterFileStatus* file);
void file_received_slave(SlaveFileStatus* file);
void file_start(MasterFileStatus* file);
void file_start_slave(SlaveFileStatus* file);

class TEST_OCRAUTOMATOR_SLAVE {
public:
	Docapost::IA::Tesseract::TesseractFactory* factory;
	Docapost::IA::Tesseract::SlaveProcessingWorker* worker;
	TEST_OCRAUTOMATOR_SLAVE(Docapost::IA::Tesseract::TesseractFactory* factory, int port = 12000);

	inline void RUN(int p = 1)
	{
		auto th = worker->Run(p);
		//th->detach();
	}

	inline ~TEST_OCRAUTOMATOR_SLAVE()
	{
		worker->onEndProcessFile.disconnect_all_slots();
		worker->onStartProcessFile.disconnect_all_slots();
		delete worker;
		delete factory;
	}
};
class TEST_OCRAUTOMATOR {
public:
	Docapost::IA::Tesseract::TesseractFactory* factory;
	Docapost::IA::Tesseract::MasterProcessingWorker* worker;
	TEST_OCRAUTOMATOR(Docapost::IA::Tesseract::TesseractFactory* factory, Docapost::IA::Tesseract::OutputFlags flags, int port = 12000);

	inline void RUN(int p = 1)
	{
		auto th = worker->Run(p);
		worker->Wait();
		while (true)
		{
			{
				std::lock_guard<std::mutex> lock(mtx);
				if (files.size() >= 1)
					break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	inline ~TEST_OCRAUTOMATOR()
	{
		worker->onEndProcessFile.disconnect_all_slots();
		worker->onStartProcessFile.disconnect_all_slots();
		delete worker;
		delete factory;
	}
};

struct Fixature
{
	Fixature() {
		std::ios_base::sync_with_stdio(true);
		//Log::InitLogger();
/*#ifdef __linux__
		freopen("/var/log/TesseractAutomatorStdErr_Slave.log", "w", stderr);
#endif
#ifdef _WIN32
		freopen("TesseractAutomatorStdErr_Master.log", "w", stderr);
#endif*/
	}
	int Port() { return port++; }
	~Fixature() {}

private:
	static int port;
};


void OcrMemory(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Image");
void OcrMemoryPdf(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Pdf");

void OcrFile(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Image");
void OcrFilePdf(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Pdf");

void OcrFile_DifferentDir(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Image");
void OcrFilePdf_DifferentDir(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Pdf");

void OcrFile_DifferentDir_Flatern(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Image");
void OcrFilePdf_DifferentDir_Flatern(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Pdf");

void OcrFile_DifferentDir_MultiFile_SingleThread(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Image");
void OcrFilePdf_DifferentDir_MultiFile_SingleThread(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Pdf");

void OcrFile_DifferentDir_MultiFile_MultiThread(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Image");
void OcrFilePdf_DifferentDir_MultiFile_MultiThread(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Pdf");

void OcrFile_Utf8(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Image");
void OcrFilePdf_Utf8(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Pdf");

void OcrFile_Slave(Docapost::IA::Tesseract::TesseractFactory* factory, Docapost::IA::Tesseract::TesseractFactory* factory2, std::string folder = "Image");
void OcrFilePdf_Slave(Docapost::IA::Tesseract::TesseractFactory* factory, Docapost::IA::Tesseract::TesseractFactory* factory2, std::string folder = "Pdf");

void OcrFile_Slave_Inverse(Docapost::IA::Tesseract::TesseractFactory* factory, Docapost::IA::Tesseract::TesseractFactory* factory2, std::string folder = "Image");
void OcrFilePdf_Slave_Inverse(Docapost::IA::Tesseract::TesseractFactory* factory, Docapost::IA::Tesseract::TesseractFactory* factory2, std::string folder = "Pdf");

void OcrFile_Http_Get(Docapost::IA::Tesseract::TesseractFactory* factory);
void OcrFile_Http_Post(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Image");
void OcrFilePdf_Http_Post(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder = "Pdf");


void Api_Test(std::string ocr);