#include "TesseractRunner.h"

#include <leptonica/allheaders.h>
#include <future>

std::atomic_int Docapost::IA::Tesseract::TesseractRunner::next_id{ 0 };

Docapost::IA::Tesseract::TesseractRunner::TesseractRunner(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang) : total(0), skip(0), psm(psm), oem(oem), lang(lang)
{
	//std::cout << "Initialisation : \n  PSM=" << this->psm << "\n  OEM=" << this->oem << "\n  Lang=" << this->lang << "\n";
	setMsgSeverity(L_SEVERITY_NONE);
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
	//total = static_cast<int>(files.size());
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
		//threads.push_back(ptr);
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
	tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	api->SetVariable("debug_file", (std::string("tesseract") + std::to_string(id) + std::string(".log")).c_str());
	api->SetVariable("out", "quiet");
	int res;

	if ((res = api->Init(NULL, lang.c_str(), oem))) {
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
