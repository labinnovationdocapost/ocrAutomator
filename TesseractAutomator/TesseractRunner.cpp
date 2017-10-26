#include "TesseractRunner.h"

#include <leptonica/allheaders.h>
#include <future>
#include <exiv2/exiv2.hpp>
#include <boost/uuid/uuid_io.hpp>

std::atomic_int Docapost::IA::Tesseract::TesseractRunner::next_id{ 0 };


Docapost::IA::Tesseract::TesseractRunner::TesseractRunner(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang, Docapost::IA::Tesseract::TesseractOutputFlags types) :
	outputTypes(types),
	total(0),
	skip(0),
	done(0),
	psm(psm),
	oem(oem),
	lang(lang),
	network(boost::make_shared<Network>(12000))
{
	setMsgSeverity(L_SEVERITY_NONE);

	network->onSlaveConnect.connect(boost::bind(&TesseractRunner::OnSlaveConnectHandler, this, _1, _2, _3));

	network->onSlaveSynchro.connect(boost::bind(&TesseractRunner::OnSlaveSynchroHandler, this, _1, _2, _3, _4));

	network->onSlaveDisconnect.connect(boost::bind(&TesseractRunner::OnSlaveDisconnectHandler, this, _1, _2));

	network->InitBroadcastReceiver();
	network->InitComm();
	networkThread =new std::thread([this]()
	{
		network->Start();
	});
}

void Docapost::IA::Tesseract::TesseractRunner::OnSlaveConnectHandler(NetworkSession* ns, int thread, std::string hostname)
{
	slaves[ns->GetHostname()] = thread;
	ns->SendStatus(this->done, this->skip, this->total, this->psm, this->oem, this->lang);
}

void Docapost::IA::Tesseract::TesseractRunner::OnSlaveSynchroHandler(NetworkSession* ns, int thread, int required, std::vector<std::tuple<std::string, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::string>>& results)
{
	slaves[ns->GetHostname()] = thread;
	std::lock_guard<std::mutex> lock(g_network_mutex);
	try
	{
		for (auto& res : results)
		{
			std::string result, uuid;
			boost::posix_time::ptime start, end;
			boost::posix_time::time_duration ellapsed;
			int threadId;
			auto file = fileSend[std::get<0>(res)];
			std::tie(uuid, file->thread, file->start, file->end, file->ellapsed, result) = res;
			file->isEnd = true;
			file->hostname = ns->GetHostname();
			CreateOutput(file, result);
			done++;
			fileSend.erase(std::get<0>(res));
		}
		boost::unordered_map<std::string, std::vector<unsigned char>*> fileToSend;
		for (int i = 0; i < required; i++)
		{
			FileStatus*  file;
			if ((file = GetFile()) != nullptr)
			{
				auto id = boost::uuids::to_string(gen());

				file->uuid = id;
				fileSend[id] = file;
				fileToSend[id] = OpenFileForLeptonica(fileSend[id]->name);
				file->fileSize = fileToSend[id]->size();
				onStartProcessFile(file);
			}
		}
		ns->SendSynchro(threads.size(), done, fileToSend);
	}
	catch (std::exception& e)
	{
		std::cout << "ERROR !!! : " << e.what() << std::endl;
	}
}

void Docapost::IA::Tesseract::TesseractRunner::OnSlaveDisconnectHandler(NetworkSession* ns, boost::unordered_map<std::string, bool>& noUsed)
{
	slaves.erase(ns->GetHostname());
	std::lock_guard<std::mutex> lock(g_network_mutex);
	try
	{
		for (auto& res : noUsed)
		{
			if (!res.second)
			{
				AddFile(fileSend[res.first]);
				onFileCanceled(fileSend[res.first]);
				fileSend.erase(res.first);
			}
		}

	}
	catch (std::exception& e)
	{
		std::cout << "ERROR !!! : " << e.what() << std::endl;
	}
}


fs::path Docapost::IA::Tesseract::TesseractRunner::ConstructNewTextFilePath(fs::path path) const
{
	fs::path new_path;

	auto it = outputs.find(TesseractOutputFlags::Text);
	if (it == outputs.end() || it->second == input)
	{
		new_path = fs::change_extension(path, ".txt");
	}
	else
	{
		auto relative_path = fs::relative(path, input);

		new_path = fs::absolute(relative_path, it->second);
		if (outputTypes & TesseractOutputFlags::Flattern)
			new_path = new_path.parent_path() / boost::replace_all_copy(fs::change_extension(relative_path, ".txt").string(), "/", separator);
		else
			new_path = new_path.parent_path() / fs::change_extension(new_path.filename(), ".txt");

	}
	return new_path;
}


fs::path Docapost::IA::Tesseract::TesseractRunner::ConstructNewExifFilePath(fs::path path) const
{
	fs::path new_path;

	auto relative_path = fs::relative(path, input);
	auto it = outputs.find(TesseractOutputFlags::Exif);
	if (it == outputs.end() || it->second == input)
	{
		return path;
	}
	else
	{
		new_path = fs::absolute(relative_path, it->second);

		if (outputTypes & TesseractOutputFlags::Flattern)
			new_path = new_path.parent_path() / boost::replace_all_copy(relative_path.string(), "/", separator);
		else
			new_path = new_path.parent_path() / new_path.filename();

	}
	return new_path;
}

bool Docapost::IA::Tesseract::TesseractRunner::FileExist(fs::path path) const
{
	auto new_path = ConstructNewTextFilePath(path);

	if (fs::exists(new_path))
	{
		return true;
	}
	return false;
}

bool Docapost::IA::Tesseract::TesseractRunner::ExifExist(fs::path path) const
{
	auto new_path = ConstructNewExifFilePath(path);

	if (!fs::exists(new_path))
	{
		return false;
	}

	Exiv2::Image::AutoPtr o_image = Exiv2::ImageFactory::open(new_path.string());
	o_image->readMetadata();
	Exiv2::ExifData &exifData = o_image->exifData();

	Exiv2::ExifKey key("Exif.Image.ProcessingSoftware");
	Exiv2::ExifData::iterator pos = exifData.findKey(key);
	if (pos != exifData.end() && pos->value().toString() == softName)
	{
		return true;
	}
	return false;
}

void Docapost::IA::Tesseract::TesseractRunner::_AddFolder(fs::path folder, bool resume)
{
	if (!fs::is_directory(folder))
	{
		return;
	}

	for (auto& entry : boost::make_iterator_range(fs::directory_iterator(folder), {}))
	{
		fs::path path = entry.path();
		if (!path.filename_is_dot() && !path.filename_is_dot_dot())
		{
			if (fs::is_directory(path))
			{
				this->_AddFolder(path, resume);
			}
			else
			{
				for (auto& extension : extensions)
				{
					if (boost::iequals<string, string>(extension, path.extension().string()))
					{
						if (resume)
						{
							bool toProcess = false;
							if (outputTypes & TesseractOutputFlags::Text)
							{
								toProcess = toProcess || !FileExist(path);
							}

							if (outputTypes & TesseractOutputFlags::Exif)
							{
								toProcess = toProcess || !ExifExist(path);
							}

							if (!toProcess)
							{
								skip++;
								break;
							}
						}

						total++;
						files.push(new FileStatus(path.string(), fs::relative(path, input).string()));
						break;
					}
				}
			}
		}
	}
}
void Docapost::IA::Tesseract::TesseractRunner::AddFolder(fs::path folder, bool resume)
{
	if (files.size() > 0)
		return;
	input = folder;
	_AddFolder(folder, resume);
}

void Docapost::IA::Tesseract::TesseractRunner::Run(int nbThread)
{
	start = boost::posix_time::second_clock::local_time();

	for (const auto& kv : outputs) {
		fs::create_directories(kv.second);
	};
	for (int i = 0; i < nbThread; i++)
	{
		int id = next_id++;
		threads[id] = new std::thread(&TesseractRunner::ThreadLoop, this, id);
	}
}

void Docapost::IA::Tesseract::TesseractRunner::AddThread()
{
	int id = next_id++;
	threads[id] = new std::thread(&TesseractRunner::ThreadLoop, this, id);
}

void Docapost::IA::Tesseract::TesseractRunner::RemoveThread()
{
	boost::lock_guard<std::mutex> lock(g_thread_mutex);

	if (threads.size() > threadToStop + 1)
		threadToStop++;
}

bool Docapost::IA::Tesseract::TesseractRunner::GetTextFromTesseract(tesseract::TessBaseAPI* api, std::vector<l_uint8>* imgData, std::string& text)
{
	Pix *image = pixReadMem(imgData->data(), imgData->size());
	if (image == nullptr)
	{
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

std::vector<l_uint8> * Docapost::IA::Tesseract::TesseractRunner::OpenFileForLeptonica(std::string uri)
{
	FILE* f = fopen(uri.c_str(), "r");

	// Determine file size
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);

	std::vector<l_uint8>* data = new std::vector<l_uint8>(size);
	//l_uint8* data = new l_uint8[size];

	rewind(f);
	fread(data->data(), sizeof(l_uint8), size, f);

	return data;
}

void Docapost::IA::Tesseract::TesseractRunner::CreateOutput(FileStatus* file, std::string outText)
{
	fs::path new_path;
	if (outputTypes & TesseractOutputFlags::Text)
	{
		new_path = ConstructNewTextFilePath(file->name);

		fs::create_directories(new_path.parent_path());

		std::ofstream output;
		output.open(new_path.string(), std::ofstream::trunc | std::ofstream::out);
		output << outText;
		output.close();

		file->output.push_back(new_path.string());
		file->relative_output.push_back(fs::relative(new_path, outputs[TesseractOutputFlags::Text]).string());
	}

	if (outputTypes & TesseractOutputFlags::Exif)
	{
		new_path = ConstructNewExifFilePath(file->name);

		auto it = outputs.find(TesseractOutputFlags::Exif);
		if (it != outputs.end() && it->second != input)
		{
			fs::create_directories(new_path.parent_path());
			if (fs::exists(new_path))
			{
				fs::remove(new_path);
			}
			fs::copy_file(file->name, new_path);
		}

		Exiv2::ExifData exifData;
		auto v = Exiv2::Value::create(Exiv2::asciiString);
		v->read(outText);
		Exiv2::ExifKey key("Exif.Image.ImageDescription");
		exifData.add(key, v.get());
		exifData["Exif.Image.ImageID"] = fs::relative(file->name, input).string();
		exifData["Exif.Image.ProcessingSoftware"] = softName;

		auto o_image = Exiv2::ImageFactory::open(new_path.string());
		o_image->setExifData(exifData);
		o_image->writeMetadata();

		file->output.push_back(new_path.string());
		file->relative_output.push_back(fs::relative(new_path, outputs[TesseractOutputFlags::Exif]).string());
	}
}

void Docapost::IA::Tesseract::TesseractRunner::ThreadLoop(int id)
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
		while (true)
		{
			FileStatus * file = GetFile();
			if(file == nullptr)
			{
				if(fileSend.size() == 0)
				{
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(300));
				continue;
			}
			file->uuid = "";
			file->thread = id;
			onStartProcessFile(file);
			file->start = boost::posix_time::microsec_clock::local_time();

			auto data = OpenFileForLeptonica(file->name);
			file->fileSize = data->size();

			std::string outText;
			if (!GetTextFromTesseract(api, data, outText))
			{
				AddFile(file);
				continue;
			}

			CreateOutput(file, outText);

			done++;

			file->end = boost::posix_time::microsec_clock::local_time();
			file->ellapsed = file->end - file->start;
			file->isEnd = true;


			boost::lock_guard<std::mutex> lock(g_thread_mutex);
			if (threadToStop > 0)
			{
				--threadToStop;
				threads.erase(id);

				break;
			}
		}

		api->End();
		delete api;
	}
	catch (const std::exception& e)
	{
		std::cout << "Thread - " << id << " " << e.what();
	}

	threads.erase(id);
	boost::lock_guard<std::mutex> lock(g_thread_mutex);
	if (threads.size() == 0)
	{
		while (fileSend.size() > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
		}
		end = boost::posix_time::second_clock::local_time();
		network->Stop();
		networkThread->join();
		onProcessEnd();
	}
}

void Docapost::IA::Tesseract::TesseractRunner::Wait()
{
	for (auto& th : threads)
		th.second->join();
}

void Docapost::IA::Tesseract::TesseractRunner::SetOutput(boost::unordered_map<TesseractOutputFlags, fs::path> folders)
{
	this->outputs = folders;
}

void Docapost::IA::Tesseract::TesseractRunner::DisplayFiles() const
{
	std::queue<FileStatus*> t = files;

	std::cout << "Files : \n";
	while (!t.empty())
	{
		std::cout << "\t" << t.front();
		t.pop();
		std::cout << "\n";
	}
	std::cout << "\n";
}

Docapost::IA::Tesseract::TesseractRunner::~TesseractRunner()
{
	network.reset();
}

FileStatus* Docapost::IA::Tesseract::TesseractRunner::GetFile()
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

void Docapost::IA::Tesseract::TesseractRunner::AddFile(FileStatus* file)
{
	g_stack_mutex.lock();
	files.push(file);

	g_stack_mutex.unlock();
}