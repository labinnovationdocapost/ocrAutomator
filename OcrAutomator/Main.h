#pragma once
#include <cstdio>

#include <string>
#include <stack>
#include <iostream>
#include <mutex>
#include <thread>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <csignal>
#include "Base/Version.h"
#include "Slave/SlaveProcessingWorker.h"
#include <google/protobuf/extension_set.h>
#include "Base/Error.h"
#include "Ocr/Tesseract/TesseractFactory.h"
#include "Base/ImageFormatEnum.h"
using std::string;

#include <tesseract/baseapi.h>
#include <sys/types.h>
#include <boost/program_options.hpp>
#include <boost/date_time.hpp>
#include <boost/unordered_map.hpp>
#include <stdlib.h>
#include <boost/interprocess/sync/named_mutex.hpp>

#define __USE_GNU

namespace po = boost::program_options;
using boost::program_options::value;

#include "Master/MasterProcessingWorker.h"
#include "Master/Network/Network.h"
#include "Base/BaseDisplay.h"


extern Docapost::IA::Tesseract::MasterProcessingWorker* workerM;
extern Docapost::IA::Tesseract::SlaveProcessingWorker* workerS;

extern boost::thread* th;
extern std::mutex g_console_mutex;


void Master(char** argv, po::variables_map& vm);
void Slave(char** argv, po::variables_map& vm);

Docapost::IA::Tesseract::OcrFactory* CreateOcrFactory(po::variables_map& vm);