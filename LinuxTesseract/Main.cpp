#include <cstdio>

#include <string>
#include <stack>
#include <iostream>
#include <mutex>
#include <thread>
#include <fstream>
#include <chrono>
using std::string;

#include <baseapi.h>
#include <allheaders.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h> 
#include <libgen.h>

void list_file_in_directory(std::stack<string> &stack, string folder);
void afficher(const std::stack<string>& p);
string get_stack_top(std::stack<string>& p);
void run_tesseract(std::stack<string> *files, std::size_t* total, int* current);

std::mutex g_stack_mutex;
std::mutex g_thread_mutex;

int main(int argc, char* argv[])
{
	int nb_process = 2;
	std::stack<string> files;

	struct stat sb;
	if(argc < 2 || lstat(argv[1], &sb) != 0 || !S_ISDIR(sb.st_mode))
	{
		std::cout << "Pleaser pass a folder in the first parameter\n";
		return 0;
	}

	if(argc > 2)
	{
		try
		{
			nb_process = atoi(argv[2]);
		}
		catch(const std::exception &e)
		{
			
		}
	}
	std::cout << "Number of thread : " << nb_process << "\n";

	list_file_in_directory(files, argv[1]);

	afficher(files);
	
	auto total = files.size();
	auto current = 0;

	std::vector<std::thread> threads;
	for(int i = 0; i < nb_process; i++)
	{
		threads.push_back(std::thread(run_tesseract, &files, &total, &current));
	}

	for (auto& th : threads)
		th.join();

	return 0;
}

int id = 0;
void run_tesseract(std::stack<string> *files, std::size_t* total, int* current)
{
	int t_id = id++;
	tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	api->SetVariable("debug_file", (std::string("tesseract") + std::to_string(t_id) + string(".log")).c_str());
	int res;
	api->SetPageSegMode(tesseract::PSM_AUTO_OSD);
	if (res = api->Init(NULL, "fra")) {
		fprintf(stderr, "Could not initialize tesseract.\n");
		exit(1);
	}

	string file;
	struct stat sb;
	try
	{
		while (!(file = get_stack_top(*files)).empty())
		{
			g_thread_mutex.lock();
			auto c = *current = *current+1;
			std::cout << "Thread " << t_id << " - " << c << "/" << *total << " - " << file << "\n";
			g_thread_mutex.unlock();

			Pix *image = pixRead(file.c_str());
			api->SetImage(image);
			char *outText = api->GetUTF8Text();

			string output_file = file.substr(0, file.find_last_of(".")) + ".txt";
			std::ofstream output;
			output.open(output_file, std::ofstream::trunc | std::ofstream::out);
			output << outText;
			output.close();

			delete[] outText;
			pixDestroy(&image);
		}


		g_thread_mutex.lock();
		std::cout << "Thread " << t_id << " End\n";
		g_thread_mutex.unlock();

		api->End();
	}
	catch (const std::exception& e)
	{
		std::cout << "Thread " << t_id << " " << e.what();
	}
}

string get_stack_top(std::stack<string>& p)
{
	while (true)
	{
		g_stack_mutex.lock();
		if (p.empty())
		{
			g_stack_mutex.unlock();
			return string();
		}

		string f = p.top();

		p.pop();

		g_stack_mutex.unlock();
		return f;
	}
}

void afficher(const std::stack<string>& p)
{
	std::stack<string> t = p;

	std::cout << "Files : \n";
	while (!t.empty())
	{
		std::cout << "\t" << t.top();
		t.pop();
		std::cout << "\n";
	}
	std::cout << "\n";
}

void list_file_in_directory(std::stack<string> &stack, string folder)
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
				string str = folder + string(ent->d_name);
				int ret = lstat(str.c_str(), &sb);
				if (S_ISDIR(sb.st_mode))
				{
					str += "/";
					list_file_in_directory(stack, str);
				}
				else
				{
					string file = string(ent->d_name);
					string ext = file.substr(file.find_last_of("."));
					if(ext == ".jpg" || ext == ".tif" || ext == ".png" || ext == ".jpeg")
						stack.push(str);
				}
			}
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		perror("");
	}
}
