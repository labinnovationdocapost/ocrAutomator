#include "MasterProcessingWorker.h"

#include <iostream>
#include <fstream>
#include <future>
#include <exiv2/exiv2.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/format.hpp>
#include "Base/Error.h"
#include "Ocr/Tesseract/TesseractFactory.h"
#include "Ocr/UninitializedOcrException.h"
#include <boost/algorithm/string.hpp>
#include "PDF/MuPDF.h"
#include "Buffer/ExivMemoryFileBuffer.h"
#include <zlib.h>
#include "MasterLocalFileStatus.h"
#include "MasterMemoryFileStatus.h"

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#define TXMP_STRING_TYPE std::string
#define XMP_INCLUDE_XMPFILES 1 
#include "XMP.incl_cpp"
#include "XMP.hpp"
#include "XMP_IO.hpp"
#include "XMP/MemoryXMPIO.h"
#include "Base/Version.h"
#include "Buffer/VectorMemoryFileBuffer.h"

const XMP_StringPtr kXMP_NS_OCRAUTOMATOR = "http://leia.io/meta/1.0/Xmp/";
const XMP_StringPtr kXMP_NS_OCRAUTOMATOR_OCR = "http://leia.io/meta/1.0/Xmp/Ocr/";

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

const std::vector<char*> Docapost::IA::Tesseract::MasterProcessingWorker::EXIF_FIELD = { (char*)"Exif.Image.DNGPrivateData", (char*)"Exif.Image.CFAPlaneColor", (char*)"Exif.Image.Copyright" };
const char* EXTENSION_EXTRA = ".extra";

Docapost::IA::Tesseract::MasterProcessingWorker::MasterProcessingWorker(OcrFactory& ocr, Docapost::IA::Tesseract::OutputFlags types, int port) :
	BaseProcessingWorker(ocr),
	mOutputTypes(types)
{
	/*PoDoFo::PdfError::EnableDebug(false);
	PoDoFo::PdfError::EnableLogging(false);*/

	SXMPMeta::Initialize();
	XMP_OptionBits options = 0;
#ifdef __linux__
	options |= kXMPFiles_IgnoreLocalText;
#endif
	SXMPFiles::Initialize(options);

	mNetworkThread = new std::thread([this, port]()
	{
		int _port = port;
		while (_port < 65565)
		{
			try
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Try Starting network on port " << _port;
				CatchAllErrorSignals();
				mNetwork = boost::make_shared<Network>(_port);
				mNetwork->onSlaveConnect.connect(boost::bind(&MasterProcessingWorker::OnSlaveConnectHandler, this, _1, _2, _3));

				mNetwork->onSlaveSynchro.connect(boost::bind(&MasterProcessingWorker::OnSlaveSynchroHandler, this, _1, _2, _3, _4));

				mNetwork->onSlaveDisconnect.connect(boost::bind(&MasterProcessingWorker::OnSlaveDisconnectHandler, this, _1, _2));

				mNetwork->InitBroadcastReceiver();
				mNetwork->InitComm();
				mIsNetworkInit = true;
				mNetwork->Start();
				break;
			}
			catch (std::exception &e)
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "/!\\ Unable to start network on port " << _port;
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "/!\\ Error " << e.what();

				if (mNetwork != nullptr)
				{
					mNetwork->Stop();
					mNetwork.reset();
				}
				_port++;
			}
			catch (...)
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "/!\\ Unable to start network on port " << _port;
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "/!\\ Error " << GET_EXCEPTION_NAME;

				if (mNetwork != nullptr)
				{
					mNetwork->Stop();
					mNetwork.reset();
				}
				_port++;
			}
		}
	});
}


const char* kPathSeparator =
#ifdef _WIN32
"\\";
#else
"/";
#endif

fs::path Docapost::IA::Tesseract::MasterProcessingWorker::ConstructNewTextFilePath(fs::path path, std::string ext = ".txt") const
{
	fs::path new_path;

	auto it = mOutputs.find(OutputFlags::Text);
	if (it == mOutputs.end() || it->second == mInput)
	{
		new_path = fs::change_extension(path, ext);
	}
	else
	{
		auto relative_path = fs::relative(path, mInput);

		new_path = fs::absolute(relative_path, it->second);
		if (mOutputTypes & OutputFlags::Flattern)
			new_path = new_path.parent_path() / boost::replace_all_copy(fs::change_extension(relative_path, ext).string(), kPathSeparator, mSeparator);
		else
			new_path = new_path.parent_path() / fs::change_extension(new_path.filename(), ext);

	}
	return new_path;
}

bool Docapost::IA::Tesseract::MasterProcessingWorker::MergeResult(MasterFileStatus* mfile)
{
	// IF we do not require text writing, just do not merge
	if (!(mOutputTypes & OutputFlags::Text))
		return true;

	MasterLocalFileStatus* file = dynamic_cast<MasterLocalFileStatus*>(mfile);
	if (file == nullptr)
		return true;

	if (file->filePosition < 0)
	{
		file->isCompleted = true;
		return true;
	}

	std::lock_guard<std::mutex>(*file->mutex_siblings);
	file->isCompleted = true;
	bool completed = true;
	for (int i = 0; i < file->siblings->size(); i++)
	{
		auto item = (*file->siblings)[i];
		if (item != nullptr)
		{
			if (!item->isCompleted)
			{
				completed = false;
				break;
			}
		}
	}

	if (completed)
	{
		auto new_path = ConstructNewTextFilePath(file->name);
		std::ofstream stream{ new_path.string(), std::ios::out | std::ios::trunc };

		for (int i = 0; i < file->siblings->size(); i++)
		{
			auto item = (*file->siblings)[i];
			if (item != nullptr)
			{
				stream << *(item->result->begin()) << std::endl;
			}
			else
			{
				auto pdf_path = CreatePdfOutputPath(file->name, i);
				auto output_file = ConstructNewTextFilePath(pdf_path);
				if (!fs::exists(output_file))
				{
					stream.close();
					fs::remove(new_path);
					break;
				}
				std::ifstream ifile(output_file.string(), std::ios::in);
				stream << ifile.rdbuf() << std::endl;
			}
		}
	}
	return completed;
}


fs::path Docapost::IA::Tesseract::MasterProcessingWorker::ConstructNewExifFilePath(fs::path path) const
{
	fs::path new_path;

	auto relative_path = fs::relative(path, mInput);
	auto it = mOutputs.find(OutputFlags::Metadata);
	if (it == mOutputs.end() || it->second == mInput)
	{
		return path;
	}
	else
	{
		new_path = fs::absolute(relative_path, it->second);

		if (mOutputTypes & OutputFlags::Flattern)
			new_path = new_path.parent_path() / boost::replace_all_copy(relative_path.string(), kPathSeparator, mSeparator);
		else
			new_path = new_path.parent_path() / new_path.filename();

	}
	return new_path;
}

bool Docapost::IA::Tesseract::MasterProcessingWorker::FileExist(fs::path path) const
{
	auto new_path = ConstructNewTextFilePath(path);

	return fs::exists(new_path);
}

bool Docapost::IA::Tesseract::MasterProcessingWorker::MetadataExist(fs::path path, bool fast) const
{
	auto new_path = ConstructNewExifFilePath(path);

	if (!fs::exists(new_path))
	{
		return false;
	}

	if (fast)
	{
		return fs::exists(new_path);
	}

	try
	{
		SXMPMeta meta;
		SXMPFiles xmpfile;
		auto res1 = xmpfile.OpenFile(new_path.string(), kXMP_UnknownFile, kXMPFiles_OpenForUpdate | kXMPFiles_OpenUseSmartHandler);
		auto res = xmpfile.GetXMP(&meta);


		std::string ns;
		SXMPMeta::RegisterNamespace(kXMP_NS_OCRAUTOMATOR, "OcrAutomator", &ns);
		SXMPMeta::RegisterNamespace(kXMP_NS_OCRAUTOMATOR_OCR, "Ocr", &ns);

		auto ocrName = mOcrFactory.Name();
		if (!meta.DoesPropertyExist(kXMP_NS_OCRAUTOMATOR, "Sections"))
		{
			if (meta.DoesPropertyExist(kXMP_NS_OCRAUTOMATOR, ocrName.c_str()))
			{
				// If the file does not possese a Section, correct it by creating one
				meta.SetProperty(kXMP_NS_OCRAUTOMATOR, "Sections", 0, kXMP_PropValueIsArray);
				meta.AppendArrayItem(kXMP_NS_OCRAUTOMATOR, "Sections", 0, ocrName.c_str(), 0);
				xmpfile.PutXMP(meta);
				xmpfile.CloseFile();
				return true;
			}
			return false;
		}

		if (!meta.DoesPropertyExist(kXMP_NS_OCRAUTOMATOR, ocrName.c_str()))
		{
			return false;
		}

		int nbSections = meta.CountArrayItems(kXMP_NS_OCRAUTOMATOR, "Sections");
		bool sectionExist = false;
		for (int i = 0; i < nbSections; i++)
		{
			std::string section;
			meta.GetArrayItem(kXMP_NS_OCRAUTOMATOR, "Sections", i + 1, &section, 0);
			if (section == ocrName)
			{
				sectionExist = true;
				break;
			}
		}

		return sectionExist;
	}
	catch (XMP_Error& wmp_err)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "cannot Write XMP : " << wmp_err.GetErrMsg();
	}
	return false;
}

string Docapost::IA::Tesseract::MasterProcessingWorker::CreatePdfOutputPath(fs::path path, int i)
{
	return (boost::format("%s%s%s-%d%d") % path.parent_path().string() % kPathSeparator % fs::change_extension(path.filename(), "").string() % i % mOcrFactory.GetExtension()).str();
}

void Docapost::IA::Tesseract::MasterProcessingWorker::InitPdfMasterFileStatus(MasterFileStatus* file, std::mutex* mutex_siblings, std::vector<MasterFileStatus*>* siblings, int i)
{
	mTotal++;
	file->filePosition = i;
	(*siblings)[i] = file;
	file->siblings = siblings;
	file->mutex_siblings = mutex_siblings;
}

int Docapost::IA::Tesseract::MasterProcessingWorker::AddPdfFile(bool resume, fs::path path)
{
	int nbPages = 0;
	try
	{
		MuPDF::MuPDF pdf;
		nbPages = pdf.GetNbPage(path.string());
	}
	catch (std::runtime_error& err)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "cannot read pdf: " << path << "\nReason: " << err.what();
	}

	std::mutex* mutex_siblings = new std::mutex();

	std::vector<MasterFileStatus*>* siblings = new std::vector<MasterFileStatus*>(nbPages);


	bool added = false;
	for (int i = 0; i < nbPages; i++)
	{
		bool toProcess = true;
		std::string output_file = CreatePdfOutputPath(path, i);

		if (resume)
		{
			toProcess = false;
			if (mOutputTypes & OutputFlags::Text)
			{
				toProcess = toProcess || !FileExist(output_file);
			}

			if (mOutputTypes & OutputFlags::Metadata)
			{
				toProcess = toProcess || !MetadataExist(output_file, mOutputTypes & OutputFlags::FastScan);
			}

			if (!toProcess)
			{
				mSkip++;
			}
		}

		if (toProcess)
		{
			MasterFileStatus* file = new MasterLocalFileStatus(path.string(), fs::relative(path, mInput).string(), output_file);
			InitPdfMasterFileStatus(file, mutex_siblings, siblings, i);
			if (!added)
			{
				added = true;
				AddFileBack(file);
			}
		}
	}
	if (!added)
	{
		delete mutex_siblings;
		delete siblings;
	}
	return nbPages;
}

int Docapost::IA::Tesseract::MasterProcessingWorker::AddPdfFile(std::string id, char* pdf, int len, std::vector<boost::uuids::uuid>& uids)
{
	MuPDF::MuPDF mupdf;
	auto nbPages = mupdf.GetNbPage(pdf, len);

	std::mutex* mutex_siblings = new std::mutex();
	std::vector<MasterFileStatus*>* siblings = new std::vector<MasterFileStatus*>(nbPages);
	bool added = false;
	for (int i = 0; i < nbPages; i++)
	{
		MasterFileStatus* file = new MasterMemoryFileStatus(id, pdf, len);
		uids.push_back(file->uid);
		InitPdfMasterFileStatus(file, mutex_siblings, siblings, i);
		if (added == false)
		{
			added = true;
			AddFileBack(file);
		}
	}
	return nbPages;
}

void Docapost::IA::Tesseract::MasterProcessingWorker::AddImageFile(bool resume, fs::path path)
{
	bool toProcess = true;
	if (resume)
	{
		toProcess = false;
		if (mOutputTypes & OutputFlags::Text)
		{
			toProcess = toProcess || !FileExist(path);
		}

		if (mOutputTypes & OutputFlags::Metadata)
		{
			toProcess = toProcess || !MetadataExist(path, mOutputTypes & OutputFlags::FastScan);
		}

		if (!toProcess)
		{
			mSkip++;
		}
	}
	if (toProcess)
	{
		mTotal++;
		AddFileBack(new MasterLocalFileStatus(path.string(), fs::relative(path, mInput).string()));
	}
}

void Docapost::IA::Tesseract::MasterProcessingWorker::AddImageFile(std::string id, char* image, int len, boost::uuids::uuid& uuid)
{
	mTotal++;
	MasterMemoryFileStatus* mmfs;
	AddFileBack(mmfs = new MasterMemoryFileStatus(id, image, len));
	uuid = mmfs->uid;
}

void Docapost::IA::Tesseract::MasterProcessingWorker::_AddFolder(fs::path folder, bool resume)
{
	if (!fs::is_directory(folder) || mIsTerminated)
	{
		return;
	}

	for (auto& entry : boost::make_iterator_range(fs::directory_iterator(folder), {}))
	{
		fs::path path = entry.path();
		if (path.filename_is_dot() || path.filename_is_dot_dot())
		{
			continue;
		}
		if (fs::is_directory(path))
		{
			this->_AddFolder(path, resume);
			continue;
		}
		auto ext = path.extension().string();
		if (!extensions.count(ext))
		{
			continue;
		}
		if (ext == ".pdf")
		{
			try
			{

				/*PoDoFo::PdfMemDocument document;
				document.Load(path.string().c_str());
				//PoDoFo::PdfStreamedDocument document(path.string().c_str());
				auto nbPages = document.GetPageCount();*/

				AddPdfFile(resume, path);
			}
			catch (...)
			{
				auto n = GET_EXCEPTION_NAME;
				//BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Impossible de decoder le fichier, le fichier ne sera pas inclue pour traitement " << path.string() << " | " << n;
				std::cerr << "Impossible de decoder le fichier, le fichier ne sera pas inclue pour traitement " << path.string() << " | " << n << std::endl;
			}
		}
		else
		{
			AddImageFile(resume, path);
		}
	}
}
void Docapost::IA::Tesseract::MasterProcessingWorker::AddFolder(fs::path folder, bool resume)
{
	mListingThread = new std::thread([this, folder, resume]()
	{
		CatchAllErrorSignals();
		if (mFiles.size() > 0)
			return;

		mInput = folder;
		_AddFolder(folder, resume);
		mIsEnd = true;

		if (!mIsTerminated)
		{
			mListingThread->detach();
			delete mListingThread;
		}
	});
}

std::thread* Docapost::IA::Tesseract::MasterProcessingWorker::Run(int nbThread)
{
	mStart = boost::posix_time::second_clock::local_time();

	for (const auto& kv : mOutputs) {
		fs::create_directories(kv.second);
	};
	for (int i = 0; i < nbThread; i++)
	{
		AddThread();
	}
	return nullptr;
}
std::string Docapost::IA::Tesseract::MasterProcessingWorker::Compress(std::string& str,
	int compressionlevel = Z_BEST_COMPRESSION)
{
	z_stream zs;                        // z_stream is zlib's control structure
	memset(&zs, 0, sizeof(zs));

	if (deflateInit(&zs, compressionlevel) != Z_OK)
		throw(std::runtime_error("deflateInit failed while compressing."));

	zs.next_in = (Bytef*)str.data();
	zs.avail_in = str.size();           // set the z_stream's input

	int ret;
	char outbuffer[32768];
	std::string outstring;

	// retrieve the compressed bytes blockwise
	do {
		zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
		zs.avail_out = sizeof(outbuffer);

		ret = deflate(&zs, Z_FINISH);

		if (outstring.size() < zs.total_out) {
			// append the block to the output string
			outstring.append(outbuffer,
				zs.total_out - outstring.size());
		}
	} while (ret == Z_OK);

	deflateEnd(&zs);

	if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
		std::ostringstream oss;
		oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
		throw(std::runtime_error(oss.str()));
	}

	return outstring;
}

void Docapost::IA::Tesseract::MasterProcessingWorker::CreateOutput(MasterFileStatus* _file)
{
	if (_file == nullptr)
		return;

	if (_file->result->size() == 0)
		return;

	if (mOutputTypes & OutputFlags::Text && std::find(_file->capability.begin(), _file->capability.end(), OutputFlags::Text) != _file->capability.end())
	{
		// We write the file if we are in file configuration, else the result is already in memory
		if (MasterLocalFileStatus* file = dynamic_cast<MasterLocalFileStatus*>(_file))
		{
			auto ext = mOcrFactory.GetTextExtension();

			for (int i = 0; i < file->result->size(); i++)
			{
				fs::path new_path = ConstructNewTextFilePath(file->new_name, ext[i]);

				fs::create_directories(new_path.parent_path());

				std::ofstream output;
				output.open(new_path.string(), std::ofstream::trunc | std::ofstream::out);
				output << (*file->result)[i];
				output.close();

				//if (i == 0)
				//{
				file->output.push_back(new_path.string());
				file->relative_output.push_back(file->relative_name);
				//}
			}
		}
	}

	if (mOutputTypes & OutputFlags::MemoryImage || mOutputTypes & OutputFlags::Metadata)
	{
		Exiv2::ExifData exifData;

		try
		{
			SXMPMeta meta;
			auto io = new MemoryXMPIO(_file->buffer);
			SXMPFiles xmpfile;
			xmpfile.OpenFile((XMP_IO*)io, kXMP_UnknownFile, kXMPFiles_OpenForUpdate | kXMPFiles_OpenUseSmartHandler);
			auto res = xmpfile.GetXMP(&meta);

			std::string ns;
			SXMPMeta::RegisterNamespace(kXMP_NS_OCRAUTOMATOR, "OcrAutomator", &ns);
			SXMPMeta::RegisterNamespace(kXMP_NS_OCRAUTOMATOR_OCR, "Ocr", &ns);

			auto ocrName = mOcrFactory.Name();

			meta.SetStructField(kXMP_NS_OCRAUTOMATOR, ocrName.c_str(), kXMP_NS_OCRAUTOMATOR_OCR, "Outputs", 0, kXMP_PropValueIsArray | kXMP_PropArrayIsOrdered);
			std::string path;
			SXMPUtils::ComposeStructFieldPath(kXMP_NS_OCRAUTOMATOR, ocrName.c_str(), kXMP_NS_OCRAUTOMATOR_OCR, "Outputs", &path);


			if (!meta.DoesPropertyExist(kXMP_NS_OCRAUTOMATOR, "Sections"))
			{
				meta.SetProperty(kXMP_NS_OCRAUTOMATOR, "Sections", 0, kXMP_PropValueIsArray);
			}

			if (!meta.DoesPropertyExist(kXMP_NS_OCRAUTOMATOR, ocrName.c_str()))
			{
				meta.AppendArrayItem(kXMP_NS_OCRAUTOMATOR, "Sections", 0, ocrName.c_str(), 0);
			}
			else
			{
				int nbSections = meta.CountArrayItems(kXMP_NS_OCRAUTOMATOR, "Sections");
				bool sectionExist = false;
				for (int i = 0; i < nbSections; i++)
				{
					std::string section;
					meta.GetArrayItem(kXMP_NS_OCRAUTOMATOR, "Sections", i + 1, &section, 0);
					if (section == ocrName)
					{
						sectionExist = true;
						break;
					}
				}
				if (!sectionExist)
				{
					meta.AppendArrayItem(kXMP_NS_OCRAUTOMATOR, "Sections", 0, ocrName.c_str(), 0);
				}
				// if the file already has this OCR output, delete, en rewrite it
				meta.DeleteProperty(kXMP_NS_OCRAUTOMATOR, path.c_str());
				meta.SetStructField(kXMP_NS_OCRAUTOMATOR, ocrName.c_str(), kXMP_NS_OCRAUTOMATOR_OCR, "Outputs", 0, kXMP_PropValueIsArray | kXMP_PropArrayIsOrdered);
			}

			for (int i = 0; i < _file->result->size(); i++)
			{
				meta.AppendArrayItem(kXMP_NS_OCRAUTOMATOR, path.c_str(), 0, (*_file->result)[i], 0);
			}
			meta.SetStructField(kXMP_NS_OCRAUTOMATOR, ocrName.c_str(), kXMP_NS_OCRAUTOMATOR_OCR, "EngineName", mOcrFactory.Name());
			meta.SetStructField(kXMP_NS_OCRAUTOMATOR, ocrName.c_str(), kXMP_NS_OCRAUTOMATOR_OCR, "EngineVersion", mOcrFactory.Version());
			meta.SetStructField(kXMP_NS_OCRAUTOMATOR, ocrName.c_str(), kXMP_NS_OCRAUTOMATOR_OCR, "Version", VERSION);

			for (auto& externalProp : mExternalXmp)
			{
				meta.SetStructField(kXMP_NS_OCRAUTOMATOR, ocrName.c_str(), kXMP_NS_OCRAUTOMATOR_OCR, externalProp.first.c_str(), externalProp.second);
			}

			auto t = std::time(nullptr);
			auto tm = *std::localtime(&t);
			std::ostringstream oss;
			oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S%z");
			auto str = oss.str();

			meta.SetStructField(kXMP_NS_OCRAUTOMATOR, ocrName.c_str(), kXMP_NS_OCRAUTOMATOR_OCR, "Date", str.c_str());
			xmpfile.PutXMP(meta);
			xmpfile.CloseFile();
			_file->buffer = io->Buffer();
			delete io;
		}
		catch (XMP_Error& wmp_err)
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "cannot Write XMP : " << wmp_err.GetErrMsg();
		}
	}

	if (mOutputTypes & OutputFlags::Metadata && std::find(_file->capability.begin(), _file->capability.end(), OutputFlags::Metadata) != _file->capability.end())
	{
		if (MasterLocalFileStatus* file = dynamic_cast<MasterLocalFileStatus*>(_file))
		{
			fs::path new_path = ConstructNewExifFilePath(file->new_name);
			auto it = mOutputs.find(OutputFlags::Metadata);
			if (it != mOutputs.end() && it->second != mInput)
			{
				fs::create_directories(new_path.parent_path());
				if (fs::exists(new_path))
				{
					fs::remove(new_path);
				}
			}
			std::ofstream stream(new_path.string(), std::ios_base::out | std::ios_base::binary);
			stream.write((char*)file->buffer->data(), file->buffer->len());

			file->output.push_back(new_path.string());
			file->relative_output.push_back(file->relative_name);
		}
	}
}

void Docapost::IA::Tesseract::MasterProcessingWorker::FreeBuffers(MasterFileStatus* file, bool memoryImage, bool memoryText, bool canDeleteText)
{
	if (file->buffer == nullptr)
		return;

	if (!memoryImage)
	{
		delete file->buffer;
		file->buffer = nullptr;
	}

	if (!memoryText && canDeleteText)
	{
		if (file->filePosition >= 0)
		{
			for (int i = 0; i < file->siblings->size(); i++)
			{
				auto item = (*file->siblings)[i];
				if (item != nullptr)
				{
					if (!item->isCompleted)
					{
						item->result.reset();
						break;
					}
				}
			}
		}
		else
		{
			file->result.reset();
		}
	}
}

void Docapost::IA::Tesseract::MasterProcessingWorker::ThreadLoop(int id)
{
	CatchAllErrorSignals();

	Ocr* ocr = nullptr;
	try
	{
		ocr = mOcrFactory.CreateNew();
		ocr->InitEngine();
	}
	catch (UninitializedOcrException& e)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::error) << "Thread " << id << " - " << e.message();
		return;
	}

	while (ocr != nullptr && !mIsTerminated)
	{
		try
		{
			MasterFileStatus * file = GetFile();
			if (file == nullptr)
			{
				{
					boost::lock_guard<std::mutex> lock(mThreadMutex);
					if (mNbThreadToStop > 0)
					{
						--mNbThreadToStop;
						//mThreads.erase(id);

						break;
					}
				}
				if (mDone == mTotal && mFileSend.empty() && mIsEnd)
				{
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(300));
				continue;
			}

			file->uuid = boost::uuids::uuid();
			file->thread = id;
			file->start = boost::posix_time::microsec_clock::local_time();

			if (file->abandoned)
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Exclude file " << file->name << ":" << file->filePosition;
				Log::WriteFileToExclude(file);
				mDone++;
				file->end = boost::posix_time::microsec_clock::local_time();
				file->ellapsed = file->end - file->start;
				file->isEnd = true;
				continue;
			}

			try
			{
				// If we got null, it meen that the processing is in progress, just wait, otherwise the file is usable, just use it
				if (nullptr == ocr->LoadFile(file, boost::bind(&MasterProcessingWorker::AddFile, this, _1)))
				{
					BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "LoadFile null waiting 1000ms";
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					continue;
				}
			}
			catch (std::runtime_error& e)
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cannot load file";
				if (file->retry < MAX_RETRY)
				{
					file->retry++;
					AddFileBack(file);
				}
				else
				{
					Log::WriteFileToExclude(file);
				}
				continue;
			}
			onStartProcessFile(file);

			try
			{
				file->result = ocr->ProcessThroughOcr(file->buffer);
				if (file->result->empty())
				{
					if (file->retry < MAX_RETRY)
					{
						file->retry++;
						AddFileBack(file);
					}
					else
					{
						Log::WriteFileToExclude(file);
					}
					continue;
				}
			}
			catch (std::runtime_error& error)
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Error: " << error.what();
				onFileCanceled(file);
				if (file->retry < MAX_RETRY)
				{
					file->retry++;
					AddFileBack(file);
				}
				else
				{
					Log::WriteFileToExclude(file);
				}
				continue;
			}

			try
			{
				CreateOutput(file);
			}
			catch (std::exception& e)
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cannot write output: " << e.what();
				onFileCanceled(file);
				if (file->retry < MAX_RETRY)
				{
					Log::WriteFileToExclude(file);
					file->retry++;
					AddFileBack(file);
				}
				continue;
			}

			auto isMerge = MergeResult(file);

			FreeBuffers(file, mOutputTypes & OutputFlags::MemoryImage, (mOutputTypes & OutputFlags::MemoryText), isMerge);

			mDone++;

			file->end = boost::posix_time::microsec_clock::local_time();
			file->ellapsed = file->end - file->start;
			file->isEnd = true;

			onEndProcessFile(file);

			boost::lock_guard<std::mutex> lock(mThreadMutex);
			if (mNbThreadToStop > 0)
			{
				--mNbThreadToStop;
				//mThreads.erase(id);

				break;
			}
		}
		catch (const std::exception& e)
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Thread - " << id << " " << e.what();
		}
		catch (...)
		{
			auto n = GET_EXCEPTION_NAME;
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Thread - " << id << " | " << n;
		}
	}

	try
	{
		// Destroy Ocr class
		std::cerr << "Thread end api " << id << std::endl;
		delete ocr;
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Thread - " << id << " " << e.what();
	}
	catch (...)
	{
		auto n = GET_EXCEPTION_NAME;
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Thread - " << id << " | " << n;
	}

	TerminateThread(id);
}

void Docapost::IA::Tesseract::MasterProcessingWorker::TerminateThread(int id)
{
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Thread " << id << " begin terminate procedure";

	{
		std::unique_lock<std::mutex> lockManagement(mThreadManagementMutex, std::try_to_lock);
		if (lockManagement.owns_lock())
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Thread " << id << " owned, removed from thread list";
			mThreads[id]->detach();
			delete mThreads[id];
		}
		else
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Thread " << id << " not owned, removed and delete delegate to caller";
		}
		mThreads.erase(id);
	}

	boost::lock_guard<std::mutex> lock(mThreadMutex);
	if (mThreads.size() == 0)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Last thread running stopped";
		while (mFileSend.size() > 0)
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Wait for " << mFileSend.size() << " file to be retrieved, retry in 300ms";
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
		}
		mEnd = boost::posix_time::second_clock::local_time();
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Stoping network";
		onProcessEnd();

		std::unique_lock<std::mutex> lk(mIsWorkDoneMutex);
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Notify everyone of the shutdown";
		mIsWorkDone.notify_all();
	}


	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Thread " << id << " terminate";
}

void Docapost::IA::Tesseract::MasterProcessingWorker::SetOutput(boost::unordered_map<OutputFlags, fs::path> folders)
{
	this->mOutputs = folders;
}

Docapost::IA::Tesseract::MasterProcessingWorker::~MasterProcessingWorker()
{
	mIsTerminated = true;
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Destroying MasterProcessingWorker";

	boost::lock_guard<std::mutex> lock(mThreadManagementMutex);
	const auto ths = mThreads;
	for (auto th : ths)
	{
		if (th.second->joinable())
			th.second->join();

		delete th.second;
	}
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Stoping network";
	mNetwork->Stop();
	mNetwork.reset();
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "network Stop";
	if (mNetworkThread != nullptr)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Waiting for network Thread to complete";
		mNetworkThread->join();
		delete mNetworkThread;
	}
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "MasterProcessingWorker Destroyed";
}