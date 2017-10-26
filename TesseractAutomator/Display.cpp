#include "Display.h"

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
	tessR.onFileCanceled.connect(boost::bind(&Display::OnCanceled, this, _1));
}


Display::~Display()
{
	endwin();
}


void Display::DrawHeader() const
{
	if (tessR.GetOutput().empty())
	{
		mvwprintw(top, 0, 0, "Input : %s | Output: %s\n", tessR.GetInput().c_str(), tessR.GetInput().c_str());
	}
	else
	{
		mvwprintw(top, 0, 0, "Input : %s | Exif Output: %s | Text Output : %s\n", tessR.GetInput().c_str(), tessR.GetOutput()[Docapost::IA::Tesseract::TesseractOutputFlags::Exif].string().c_str(), tessR.GetOutput()[Docapost::IA::Tesseract::TesseractOutputFlags::Text].string().c_str());
	}
	if (tessR.GetThreadToStop() > 0)
		mvwprintw(top, 1, 0, "Threads Local/Remote: %d (-%d)/%d | Page Segmentation Mode: %d | Ocr Engine Mode: %d\n", tessR.GetNbThread(), tessR.GetThreadToStop(), tessR.GetRemoteThread(), tessR.GetPSM(), tessR.GetOEM());
	else
		mvwprintw(top, 1, 0, "Threads Local/Remote: %d/%d | Page Segmentation Mode: %d | Ocr Engine Mode: %d\n", tessR.GetNbThread(), tessR.GetRemoteThread(), tessR.GetPSM(), tessR.GetOEM());

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

	mvwprintw(top, 3, 0, "Files Total: %d | Files Skip: %d | Mode: %d\n", tessR.GetNbFiles(), tessR.GetNbSkipFiles(), tessR.GetOutputTypes());
	wrefresh(top);
}

void Display::DrawBody(const std::vector<FileStatus*> files, FileSum& s) const
{
	mvwprintw(header, 0, 0, "%-15s %-6s %s\n", "Ellapsed", "Thread", "Origin");
	wrefresh(header);

	wmove(win, 0, 0);
	for (auto j = std::max(static_cast<int>(files.size()) - h, 0); j < files.size(); j++)
	{
		if (files[j]->isEnd)
		{
			std::stringstream cstring;
			cstring << "" << files[j]->ellapsed;
			//wprintw(win, "%-15s %-6d %s -> %s\n", cstring.str().c_str(), files[j]->thread, files[j]->relative_name.c_str(), boost::algorithm::join(files[j]->relative_output, " | ").c_str());
			wprintw(win, "%-15s %-6d %-15s %s\n", cstring.str().c_str(), files[j]->thread,  files[j]->hostname.c_str(), files[j]->relative_name.c_str());
		}
		else
		{
			wprintw(win, "%-15s %-6d %-15s %s\n", "", files[j]->thread,  files[j]->hostname.c_str(), files[j]->relative_name.c_str());
		}

		s(files[j]);
	}
	wrefresh(win);
}

void Display::DrawBodyNetwork(const std::vector<FileStatus*> files, FileSum& s) const
{
	mvwprintw(header, 0, 0, "%-20s %-6s\n", "Hostname", "Thread");
	wrefresh(top);

	wmove(win, 0, 0);
	for (auto j = std::max(static_cast<int>(files.size()) - h, 0); j < files.size(); j++)
	{
		s(files[j]);
	}
	wprintw(win, "%-20s %-6d\n", "Master", tessR.GetNbThread());
	for(auto& slave : tessR.GetSlaves())
	{
		wprintw(win, "%-20s %-6d\n", slave.first.c_str(), slave.second);
	}
	wrefresh(win);
}

void Display::DrawFooter(const std::vector<FileStatus*> cfiles, FileSum s) const
{
	if (s.count > 0)
	{
		std::stringstream cstring;


		if (isEnd)
		{
			auto average = (tessR.GetEndTime() - tessR.GetStartTime()) / tessR.GetNbFiles();
			cstring << "files: " << tessR.GetDone() << "/" << tessR.GetNbFiles() 
				<< "\t Average: " << std::setw(2) << std::setfill('0') << average.hours() << ":"
				<< std::setw(2) << std::setfill('0') << average.minutes() << ":"
				<< std::setw(2) << std::setfill('0') << average.seconds() << "."
				<< std::setw(3) << std::setfill('0') << average.fractional_seconds() / 1000 << "/Image"
				<< "\t Ellapsed: " << tessR.GetEndTime() - tessR.GetStartTime() << "\t Finish: " << tessR.GetEndTime();
		}
		else
		{
			auto remaining = (s.sum / s.count / (tessR.GetNbThread() + tessR.GetRemoteThread())) * (tessR.GetNbFiles() - cfiles.size());
			auto average = s.sum / s.count / (tessR.GetNbThread() + tessR.GetRemoteThread());
			cstring << "files: " << tessR.GetDone() << "/" << tessR.GetNbFiles()
				<< "\t Average: " << std::setw(2) << std::setfill('0') << average.hours() << ":"
				<< std::setw(2) << std::setfill('0') << average.minutes() << ":"
				<< std::setw(2) << std::setfill('0') << average.seconds() << "."
				<< std::setw(3) << std::setfill('0') << average.fractional_seconds()/1000 << "/Image";

			cstring << "\t Remaining: " << std::setw(2) << std::setfill('0') << remaining.hours() << ":"
				<< std::setw(2) << std::setfill('0') << remaining.minutes() << ":"
				<< std::setw(2) << std::setfill('0') << remaining.seconds()
				<< "\t Estimated End: " << boost::posix_time::second_clock::local_time() + remaining;
		}
		cstring << std::endl;

		mvwprintw(bottom, 0, 0, cstring.str().c_str());
	}
	else
	{
		mvwprintw(bottom, 0, 0, "files: %d/%d\t Average: Unknown\t Remaining: Unknown\t Estimated End: Unknown\n", tessR.GetDone(), tessR.GetNbFiles());
	}

	wrefresh(bottom);
}

void Display::DrawCommand() const
{
	if (!isEnd)
		mvwprintw(ctrl, 0, 0, "[CTRL+C] Abandon | [+]/[-] Increase/Decrease Thread | [v] Change view\n");
	else
		mvwprintw(ctrl, 0, 0, "[ENTER] Exit\n");
	wrefresh(ctrl);
}

void Display::Draw()
{
	boost::lock_guard<std::mutex> lock(g_thread_mutex);
	DrawHeader();

	FileSum s{};

	if(currentView == 0)
		DrawBody(files, s);
	if (currentView == 1)
		DrawBodyNetwork(files, s);

	DrawFooter(files, s);

	DrawCommand();
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
void Display::OnCanceled(FileStatus* str)
{
	files.erase(std::remove(files.begin(), files.end(), str), files.end());
	werase(win);
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
			else if (ch == 'v')
			{
				currentView = (currentView + 1) % totalView;
				werase(win);
				Draw();
			}
			else if ((ch == KEY_ENTER || ch == '\n') && isEnd)
			{
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}
