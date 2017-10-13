#include "Display.h"

int k = 0;
int l = 0;

#define COLOR_GREY 8

#define HTOP 4
#define HHEADER 1
#define HBOTTOM 1
#define HCTRL 1
#define HWIN(h) h - (HTOP + HHEADER + HBOTTOM + HCTRL)

void Display::Init(bool create)
{
	if (!create)
	{
		delwin(top);
		delwin(header);
		delwin(win);
		delwin(bottom);
		delwin(ctrl);
		endwin();
		refresh();
		clear();
		refresh();
	}
	getmaxyx(stdscr, h, w);
	top = newwin(HTOP, w, 0, 0);
	header = newwin(HHEADER, w, HTOP, 0);
	win = newwin(HWIN(h), w, HTOP + HHEADER, 0);
	bottom = newwin(HBOTTOM, w, h - HCTRL - 1, 0);
	ctrl = newwin(HCTRL, w, h - 1, 0);

	scrollok(win, true);
	wbkgd(header, COLOR_PAIR(1));
	wbkgd(ctrl, COLOR_PAIR(4));
	if (isEnd)
		wbkgd(bottom, COLOR_PAIR(3));
	else
		wbkgd(bottom, COLOR_PAIR(2));

	refresh();
}

void Display::OnEnd()
{
	timeEnd = boost::posix_time::second_clock::local_time();
	this->isEnd = true;

	wbkgd(bottom, COLOR_PAIR(3));
	refresh();
}

Display::Display(Docapost::IA::Tesseract::TesseractRunner& tessR) : tessR(tessR)
{
	initscr();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	curs_set(0);

	start_color();
	init_color(COLOR_GREY, 500, 500, 500);
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_WHITE, COLOR_RED);
	init_pair(3, COLOR_WHITE, COLOR_GREEN);
	init_pair(4, COLOR_WHITE, COLOR_GREY);

	Init();
	tessR.onStartProcessFile.connect(boost::bind(&Display::ShowFile, this, _1));
	tessR.onProcessEnd.connect(boost::bind(&Display::OnEnd, this));
}


Display::~Display()
{
	endwin();
}

struct Sum
{
	Sum() : count(0) {};
	void operator()(FileStatus* fs)
	{
		if (!fs->output.empty())
		{
			sum += fs->ellapsed;
			count++;
		}
	}
	boost::posix_time::ptime::time_duration_type sum;
	int count;
};

void Display::Draw()
{
	boost::lock_guard<std::mutex> lock(g_thread_mutex);
	const auto cfiles = files;
	auto files_count = files.size();
	if (tessR.GetOutput().empty())
	{
		mvwprintw(top, 0, 0, "Input : %s | Output: %s\n", tessR.GetInput().c_str(), tessR.GetInput().c_str());
	}
	else
	{
		mvwprintw(top, 0, 0, "Input : %s | Output: %s\n", tessR.GetInput().c_str(), tessR.GetOutput().c_str());
	}
	if (tessR.GetThreadToStop() > 0)
		mvwprintw(top, 1, 0, "Threads: %d (-%d) | Page Segmentation Mode: %d | Ocr Engine Mode: %d\n", tessR.GetNbThread(), tessR.GetThreadToStop(), tessR.GetPSM(), tessR.GetOEM());
	else
		mvwprintw(top, 1, 0, "Threads: %d | Page Segmentation Mode: %d | Ocr Engine Mode: %d\n", tessR.GetNbThread(), tessR.GetPSM(), tessR.GetOEM());

	if (isEnd)
	{
		std::stringstream cstring;
		cstring << "Start: " << tessR.GetStartTime() << " | End: " << tessR.GetEndTime() << " | Ellapsed: " << (tessR.GetEndTime() - tessR.GetStartTime());
		mvwprintw(top, 2, 0, "%s\n", cstring.str().c_str());
	}
	else
	{
		std::stringstream cstring;
		cstring << "Start: " << tessR.GetStartTime() << " | End: In Progress | Ellapsed: " << (boost::posix_time::second_clock::local_time() - tessR.GetStartTime());
		mvwprintw(top, 2, 0, "%s\n", cstring.str().c_str());
	}
	mvwprintw(top, 3, 0, "Files Total: %d | Files Skip: %d\n", tessR.GetNbFiles(), tessR.GetNbSkipFiles());
	wrefresh(top);

	mvwprintw(header, 0, 0, "%-15s %-6s %s -> %s\n", "Ellapsed", "Thread", "Origin", "Output");
	wrefresh(header);

	wmove(win, 0, 0);
	Sum s{};
	for (auto j = std::max(static_cast<int>(files_count) - h, 0); j < files_count; j++)
	{
		if (files[j]->output.empty())
		{
			wprintw(win, "%-15s %-6d %s\n", "", files[j]->thread, files[j]->name.c_str());
		}
		else
		{
			std::stringstream cstring;
			cstring << "" << files[j]->ellapsed;
			wprintw(win, "%-15s %-6d %s -> %s\n", cstring.str().c_str(), files[j]->thread, files[j]->name.c_str(), files[j]->output.c_str());
		}

		s(files[j]);
	}
	wrefresh(win);

	if (s.count > 0)
	{
		std::stringstream cstring;


		if (isEnd)
			cstring << "files: " << files_count << "/" << tessR.GetNbFiles() << "\t Average: " << (tessR.GetEndTime() - tessR.GetStartTime()) / tessR.GetNbFiles() << "/Image" << "\t Ellapsed: " << tessR.GetEndTime() - tessR.GetStartTime() << "\t Finish: " << tessR.GetEndTime();
		else
		{
			auto remaining = (s.sum / s.count / tessR.GetNbThread()) * (tessR.GetNbFiles() - files_count);
			cstring << "files: " << files_count << "/" << tessR.GetNbFiles()
				<< "\t Average: " << s.sum / s.count / tessR.GetNbThread() << "/Image";

			cstring << "\t Remaining: " << remaining
				<< "\t Estimated End: " << boost::posix_time::second_clock::local_time() + remaining;
		}
		cstring << std::endl;

		mvwprintw(bottom, 0, 0, cstring.str().c_str());
	}
	else
	{
		mvwprintw(bottom, 0, 0, "files: %d/%d\t Average: Unknown\t Remaining: Unknown\t Estimated End: Unknown\n", files_count, tessR.GetNbFiles());
	}

	wrefresh(bottom);


	if (!isEnd)
		mvwprintw(ctrl, 0, 0, "[CTRL+C] Abandon | [+]/[-] Increase/Decrease Thread\n");
	else
		mvwprintw(ctrl, 0, 0, "[ENTER] Exit\n");
	wrefresh(ctrl);
}

void Display::Resize()
{
	Init(false);
	Draw();
}


void Display::ShowFile(FileStatus* str)
{
	files.insert(files.end(), str);
}

void Display::Run()
{
	int ch;
	while (true)
	{
		if ((ch = getch()) == ERR) {
			/* user hasn't responded
			...
			*/
			Draw();
		}
		else {
			if (ch == '+')
			{
				tessR.AddThread();
			}
			else if (ch == '-')
			{
				tessR.RemoveThread();
			}
			else if ((ch == KEY_ENTER || ch == '\n') && isEnd)
			{
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}
