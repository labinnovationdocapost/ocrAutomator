#include "TesseractRunner.h"
#include <boost/date_time/posix_time/posix_time_types.hpp>

int Docapost::IA::Tesseract::TesseractRunner::next_id = 0;

Docapost::IA::Tesseract::TesseractRunner::TesseractRunner(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang) : g_console_mutex(new std::mutex()), total(0), current(0), psm(psm), oem(oem), lang(lang)
{
	std::cout << "Initialisation : \n  PSM=" << this->psm << "\n  OEM=" << this->oem << "\n  Lang=" << this->lang << "\n";
}

void Docapost::IA::Tesseract::TesseractRunner::AddFolder(std::string folder)
{
	DIR *dir;
	struct dirent *ent;
	struct stat sb;
	if ((dir = opendir(folder.c_str())) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			{
				// do nothing (straight logic)
			}
			else
			{
				std::string str = folder + std::string(ent->d_name);
				int ret = lstat(str.c_str(), &sb);
				if (S_ISDIR(sb.st_mode))
				{
					str += "/";
					this->AddFolder(str);
				}
				else
				{
					std::string file = std::string(ent->d_name);
					std::string ext = file.substr(file.find_last_of("."));

					for (auto& extension : extensions)
					{
						if (boost::iequals(extension, ext))
						{
							files.push(str);
							break;
						}
					}
				}
			}
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		perror("");
	}
	total = files.size();
}

void Docapost::IA::Tesseract::TesseractRunner::Run(int nbThread)
{
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


			std::string output_file = file.substr(0, file.find_last_of(".")) + ".txt";
			std::ofstream output;
			output.open(output_file, std::ofstream::trunc | std::ofstream::out);
			output << outText;
			output.close();

			delete[] outText;
			pixDestroy(&image);

			end = boost::posix_time::microsec_clock::local_time();

			std::cout << "\033[32mThread " << id << " - " << curr << "/" << total << " - " << file << " [" << (end - start) << "]\033[0m" << std::endl;
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

void Docapost::IA::Tesseract::TesseractRunner::SetConsoleMutex(std::mutex* mutex)
{
	delete g_console_mutex;
	g_console_mutex = mutex;
}

void Docapost::IA::Tesseract::TesseractRunner::DisplayFiles() const
{
	g_console_mutex->lock();
	std::stack<std::string> t = files;

	std::cout << "Files : \n";
	while (!t.empty())
	{
		std::cout << "\t" << t.top();
		t.pop();
		std::cout << "\n";
	}
	std::cout << "\n";
	g_console_mutex->unlock();
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
