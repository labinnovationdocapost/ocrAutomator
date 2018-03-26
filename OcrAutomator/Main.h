#pragma once

#include <mutex>
#include <thread>
#include "Slave/SlaveProcessingWorker.h"
#include "Base/Error.h"
#include "Ocr/Tesseract/TesseractFactory.h"
#include "Http/HttpServer.h"
using std::string;

#include <boost/program_options.hpp>

#define __USE_GNU

namespace po = boost::program_options;
using boost::program_options::value;

#include "Master/MasterProcessingWorker.h"


extern Docapost::IA::Tesseract::MasterProcessingWorker* workerM;
extern Docapost::IA::Tesseract::SlaveProcessingWorker* workerS;

extern boost::thread* th;
extern std::mutex g_console_mutex;
extern HttpServer* http;


void Master(char** argv, po::variables_map& vm);
void Slave(char** argv, po::variables_map& vm);

Docapost::IA::Tesseract::OcrFactory* CreateOcrFactory(po::variables_map& vm);
int _main(int argc, char* argv[]);