#include "TesseractRunner.h"

int Docapost::IA::Tesseract::TesseractRunner::next_id = 0;

Docapost::IA::Tesseract::TesseractRunner::TesseractRunner(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang) : total(0), current(0), psm(psm), oem(oem), lang(lang)
{
	std::cout << "Initialisation : \n  PSM=" << this->psm << "\n  OEM=" << this->oem << "\n  Lang=" << this->lang << "\n";
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
								break;
							}
						}

						files.push(path.string());
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
	total = files.size();
}

void Docapost::IA::Tesseract::TesseractRunner::Run(int nbThread)
{
	if (!output.empty())
	{
		fs::create_directories(output);
	}
	for (int i = 0; i < nbThread; i++)
	{
		boost::shared_ptr<std::thread> ptr(new std::thread(&TesseractRunner::ThreadLoop, this));
		threads.push_back(ptr);
	}
}

void Docapost::IA::Tesseract::TesseractRunner::ThreadLoop()
{
	auto id = next_id++;
	tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	api->SetVariable("debug_file", (std::string("tesseract") + std::to_string(id) + std::string(".log")).c_str());
	api->SetVariable("out", "quiet");
	int res;
	api->SetPageSegMode(psm);
	if (res = api->Init(NULL, lang.c_str(), oem)) {
		std::cerr << "Thread " << id << " - " << "Could not initialize tesseract\n";
		return;
	}
	std::cout << "Thread " << id << " - Start\n";

	std::string file;
	struct stat sb;
	try
	{
		boost::posix_time::ptime start, end;
		int curr = 0;
		while (!(file = GetFile()).empty())
		{
			curr = current = current + 1;
			std::cout << "Thread " << id << " - " << current << "/" << total << " - " << file << "\n";

			start = boost::posix_time::microsec_clock::local_time();

			Pix *image = pixRead(file.c_str());
			api->SetImage(image);
			char *outText = api->GetUTF8Text();

			fs::path r_path;
			if (output.empty())
			{
				r_path = fs::change_extension(file, ".txt");
			}
			else
			{
				r_path = fs::absolute(fs::relative(file, input), output);
				r_path = r_path.parent_path() / (fs::change_extension(r_path.filename(), ".txt"));
				fs::create_directories(r_path.parent_path());
			}

			std::ofstream output;
			output.open(r_path.string(), std::ofstream::trunc | std::ofstream::out);
			output << outText;
			output.close();

			delete[] outText;
			pixDestroy(&image);

			end = boost::posix_time::microsec_clock::local_time();

			std::cout << "\033[32mThread " << id << " - " << curr << "/" << total << " - " << r_path << " [" << (end - start) << "]\033[0m" << std::endl;
		}


		std::cout << "Thread " << id << " - End\n";

		api->End();
	}
	catch (const std::exception& e)
	{
		std::cout << "Thread - " << id << " " << e.what();
	}
}

void Docapost::IA::Tesseract::TesseractRunner::Wait()
{
	for (auto& th : threads)
		th->join();
}

void Docapost::IA::Tesseract::TesseractRunner::SetOutput(std::string folder)
{
	this->output = folder;
}

void Docapost::IA::Tesseract::TesseractRunner::DisplayFiles() const
{
	std::stack<std::string> t = files;

	std::cout << "Files : \n";
	while (!t.empty())
	{
		std::cout << "\t" << t.top();
		t.pop();
		std::cout << "\n";
	}
	std::cout << "\n";
}

std::string Docapost::IA::Tesseract::TesseractRunner::GetFile()
{
	while (true)
	{
		g_stack_mutex.lock();
		if (files.empty())
		{
			g_stack_mutex.unlock();
			return std::string();
		}

		std::string f = files.top();

		files.pop();

		g_stack_mutex.unlock();
		return f;
	}
}
