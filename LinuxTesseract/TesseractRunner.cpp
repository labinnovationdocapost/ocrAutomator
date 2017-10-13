#include "TesseractRunner.h"

#include <leptonica/allheaders.h>
#include <future>
#include <exiv2/exiv2.hpp>

std::atomic_int Docapost::IA::Tesseract::TesseractRunner::next_id{ 0 };

Docapost::IA::Tesseract::TesseractRunner::TesseractRunner(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang, Docapost::IA::Tesseract::TesseractOutputFlags types) : outputTypes(types), total(0), skip(0), psm(psm), oem(oem), lang(lang)
{
	setMsgSeverity(L_SEVERITY_NONE);
}

bool Docapost::IA::Tesseract::TesseractRunner::FileExist(fs::path path) const
{
	fs::path r_path;
	if (output.empty())
	{
		r_path = fs::change_extension(path, ".txt");
	}
	else
	{
		r_path = fs::absolute(fs::relative(path, input), output);
		r_path = r_path.parent_path() / (fs::change_extension(r_path.filename(), ".txt"));
	}

	if (fs::exists(r_path))
	{
		return true;
	}
	return false;
}

bool Docapost::IA::Tesseract::TesseractRunner::ExifExist(fs::path path) const
{
	fs::path r_path;
	if (output.empty())
	{
		r_path = fs::change_extension(path, ".txt");
	}
	else
	{
		r_path = fs::absolute(fs::relative(path, input), output);
		r_path = r_path.parent_path() / r_path.filename();
	}

	if (!fs::exists(r_path))
	{
		return false;
	}

	Exiv2::Image::AutoPtr o_image = Exiv2::ImageFactory::open(r_path.string());
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
							if(outputTypes & TesseractOutputFlags::Text)
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
						files.push(new FileStatus(path.string()));
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
	if (!output.empty())
	{
		fs::create_directories(output);
	}
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

	if(threads.size() > threadToStop +1)
		threadToStop++;
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

	FileStatus* file;
	try
	{
		while ((file = GetFile()) != nullptr)
		{

			file->thread = id;
			onStartProcessFile(file);
			file->start = boost::posix_time::microsec_clock::local_time();

			Pix *image = pixRead(file->name.c_str());
			api->SetImage(image);
			char *outText = api->GetUTF8Text();

			fs::path r_path;
			if(outputTypes & TesseractOutputFlags::Text)
			{
				if (output.empty())
				{
					r_path = fs::change_extension(file->name, ".txt");
				}
				else
				{
					r_path = fs::absolute(fs::relative(file->name, input), output);
					r_path = r_path.parent_path() / (fs::change_extension(r_path.filename(), ".txt"));
					fs::create_directories(r_path.parent_path());
				}

				std::ofstream output;
				output.open(r_path.string(), std::ofstream::trunc | std::ofstream::out);
				output << outText;
				output.close();

			}

			if (outputTypes & TesseractOutputFlags::Exif)
			{
				auto relative = fs::relative(file->name, input);
				if (output.empty())
				{
					r_path = file->name;
				}
				else
				{
					r_path = fs::absolute(relative, output);
					r_path = r_path.parent_path() / r_path.filename();
					fs::create_directories(r_path.parent_path());
					if(fs::exists(r_path))
					{
						fs::remove(r_path);
					}
					fs::copy_file(file->name, r_path);
				}
				Exiv2::ExifData exifData;
				auto v = Exiv2::Value::create(Exiv2::asciiString);
				v->read(outText);
				Exiv2::ExifKey key("Exif.Image.ImageDescription");
				exifData.add(key, v.get());
				exifData["Exif.Image.ImageID"] = relative.string();
				exifData["Exif.Image.ProcessingSoftware"] = softName;

				auto o_image = Exiv2::ImageFactory::open(r_path.string());
				o_image->setExifData(exifData);
				o_image->writeMetadata();
			}

			delete[] outText;
			pixDestroy(&image);

			file->end = boost::posix_time::microsec_clock::local_time();
			file->ellapsed = file->end - file->start;
			file->output = r_path.string();


			boost::lock_guard<std::mutex> lock(g_thread_mutex);
			if(threadToStop > 0)
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
		end = boost::posix_time::second_clock::local_time();
		onProcessEnd();
	}
}

void Docapost::IA::Tesseract::TesseractRunner::Wait()
{
	for (auto& th : threads)
		th.second->join();
}

void Docapost::IA::Tesseract::TesseractRunner::SetOutput(std::string folder)
{
	this->output = folder;
}
void Docapost::IA::Tesseract::TesseractRunner::SetOutput(fs::path folder)
{
	this->output = folder;
}

void Docapost::IA::Tesseract::TesseractRunner::DisplayFiles() const
{
	std::stack<FileStatus*> t = files;

	std::cout << "Files : \n";
	while (!t.empty())
	{
		std::cout << "\t" << t.top();
		t.pop();
		std::cout << "\n";
	}
	std::cout << "\n";
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

		auto f = files.top();

		files.pop();

		g_stack_mutex.unlock();
		return f;
	}
}
