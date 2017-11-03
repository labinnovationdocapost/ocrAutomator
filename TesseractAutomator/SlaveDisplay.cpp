#include "SlaveDisplay.h"

#define COLOR_GREY 8

#define HTOP 4
#define HHEADER 1
#define HBOTTOM 1
#define HCTRL 1
#define HWIN(h) h - (HTOP + HHEADER + HBOTTOM + HCTRL)

void SlaveDisplay::Init(bool create)
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

void SlaveDisplay::OnEnd()
{
	timeEnd = boost::posix_time::second_clock::local_time();
	this->isEnd = true;

	wbkgd(bottom, COLOR_PAIR(3));
	refresh();
}

SlaveDisplay::SlaveDisplay(Docapost::IA::Tesseract::TesseractSlaveRunner& tessR) : tessR(tessR)
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
	tessR.onStartProcessFile.connect(boost::bind(&SlaveDisplay::ShowFile, this, _1));
	tessR.onProcessEnd.connect(boost::bind(&SlaveDisplay::OnEnd, this));
}


SlaveDisplay::~SlaveDisplay()
{
	endwin();
}


void SlaveDisplay::DrawHeader() const
{
	if (tessR.remote_isconnected())
	{
		try
		{
			mvwprintw(top, 0, 0, "Remote: %s\n", tessR.remote_address().c_str());
		}
		catch (std::exception e)
		{
			mvwprintw(top, 0, 0, "Remote: Not found");
		}
	}
	else
	{
		mvwprintw(top, 0, 0, "Remote: Not found");
	}
	if (tessR.NbThreadToStop() > 0)
		mvwprintw(top, 1, 0, "Threads: %d (-%d) | Page Segmentation Mode: %d | Ocr Engine Mode: %d\n", tessR.NbThreads(), tessR.NbThreadToStop(), tessR.Psm(), tessR.Oem());
	else
		mvwprintw(top, 1, 0, "Threads: %d | Page Segmentation Mode: %d | Ocr Engine Mode: %d\n", tessR.NbThreads(), tessR.Psm(), tessR.Oem());


	mvwprintw(top, 3, 0, "Files Total: %d | Files Skip: %d\n", tessR.Total(), tessR.Skip());
	wrefresh(top);
}

void SlaveDisplay::DrawBody(const std::vector<SlaveFileStatus*> files) const
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
			wprintw(win, "%-15s %-6d %-36s %d\n", cstring.str().c_str(), files[j]->thread, files[j]->uuid.c_str(), files[j]->fileSize);
		}
		else
		{
			wprintw(win, "%-15s %-6d %-36s %d\n", "", files[j]->thread, files[j]->uuid.c_str(), files[j]->fileSize);
		}
	}
	wrefresh(win);
}

void SlaveDisplay::DrawFooter(const std::vector<SlaveFileStatus*> cfiles) const
{
	mvwprintw(bottom, 0, 0, "files: %d/%d\n", tessR.Done(), tessR.Total());

	wrefresh(bottom);
}

void SlaveDisplay::DrawCommand() const
{
	if (!isEnd)
		mvwprintw(ctrl, 0, 0, "[CTRL+C] Abandon | [+]/[-] Increase/Decrease Thread\n");
	else
		mvwprintw(ctrl, 0, 0, "[ENTER] Exit\n");
	wrefresh(ctrl);
}

void SlaveDisplay::Draw()
{
	boost::lock_guard<std::mutex> lock(g_thread_mutex);
	DrawHeader();

	DrawBody(files);

	DrawFooter(files);

	DrawCommand();
}

void SlaveDisplay::Resize()
{
	Init(false);
	Draw();
}


void SlaveDisplay::ShowFile(SlaveFileStatus* str)
{
	files.insert(files.end(), str);
}

void SlaveDisplay::Run()
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
			/*else if (ch == 'v')
			{
				currentView = (currentView + 1) % totalView;
				werase(win);
				Draw();
			}*/
			else if ((ch == KEY_ENTER || ch == '\n') && isEnd)
			{
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}
