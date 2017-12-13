#include "SlaveDisplay.h"
#include "TesseractFactory.h"
#include <boost/uuid/uuid_io.hpp>

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
		delwin(mTopWindow);
		delwin(mHeaderWindow);
		delwin(mMainWindow);
		delwin(mFooterWindow);
		delwin(mControlWindow);
		endwin();
		refresh();
		clear();
		refresh();
	}
	getmaxyx(stdscr, mScreenHeight, mScreenWidth);
	mTopWindow = newwin(HTOP, mScreenWidth, 0, 0);
	mHeaderWindow = newwin(HHEADER, mScreenWidth, HTOP, 0);
	mMainWindow = newwin(HWIN(mScreenHeight), mScreenWidth, HTOP + HHEADER, 0);
	mFooterWindow = newwin(HBOTTOM, mScreenWidth, mScreenHeight - HCTRL - 1, 0);
	mControlWindow = newwin(HCTRL, mScreenWidth, mScreenHeight - 1, 0);

	scrollok(mMainWindow, true);
	wbkgd(mHeaderWindow, COLOR_PAIR(1));
	wbkgd(mControlWindow, COLOR_PAIR(4));
	if (mIsEnd)
		wbkgd(mFooterWindow, COLOR_PAIR(3));
	else
		wbkgd(mFooterWindow, COLOR_PAIR(2));

	refresh();
}

void SlaveDisplay::OnEnd()
{
	mTimeEnd = boost::posix_time::second_clock::local_time();
	this->mIsEnd = true;

	wbkgd(mFooterWindow, COLOR_PAIR(3));
	refresh();
}

SlaveDisplay::SlaveDisplay(Docapost::IA::Tesseract::SlaveProcessingWorker& tessR) : mTesseractRunner(tessR)
{
	initscr();
	noecho();
	keypad(stdscr, true);
	nodelay(stdscr, true);
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
	if (mTesseractRunner.remote_isconnected())
	{
		try
		{
			mvwprintw(mTopWindow, 0, 0, "Remote: %s\n", mTesseractRunner.remote_address().c_str());
		}
		catch (std::exception e)
		{
			mvwprintw(mTopWindow, 0, 0, "Remote: Not found");
		}
	}
	else
	{
		mvwprintw(mTopWindow, 0, 0, "Remote: Not found");
	}
	auto& ocrFactory = mTesseractRunner.ocrFactory();
	auto tess = dynamic_cast<Docapost::IA::Tesseract::TesseractFactory*>(&ocrFactory);
	if (tess != nullptr)
	{
		if (mTesseractRunner.NbThreadToStop() > 0)
			mvwprintw(mTopWindow, 1, 0, "Threads: %d (-%d) | Page Segmentation Mode: %d | Ocr Engine Mode: %d | Network: %s (%d)\n", mTesseractRunner.NbThreads(), mTesseractRunner.NbThreadToStop(), tess->Psm(), tess->Oem(), mTesseractRunner.NetworkEnable() ? "On" : "Off", mTesseractRunner.Port());
		else
			mvwprintw(mTopWindow, 1, 0, "Threads: %d | Page Segmentation Mode: %d | Ocr Engine Mode: %d | Network: %s (%d)\n", mTesseractRunner.NbThreads(), tess->Psm(), tess->Oem(), mTesseractRunner.NetworkEnable() ? "On" : "Off", mTesseractRunner.Port());
	}


	mvwprintw(mTopWindow, 3, 0, "Files Total: %d | Files Skip: %d\n", mTesseractRunner.Total(), mTesseractRunner.Skip());
	wrefresh(mTopWindow);
}

void SlaveDisplay::DrawBody(const std::vector<SlaveFileStatus*> files) const
{
	mvwprintw(mHeaderWindow, 0, 0, "%-15s %-6s %s\n", "Ellapsed", "Thread", "Origin");
	wrefresh(mHeaderWindow);

	wmove(mMainWindow, 0, 0);
	for (auto j = std::max(static_cast<int>(files.size()) - mScreenHeight, 0); j < files.size(); j++)
	{
		if (files[j]->isEnd)
		{
			std::stringstream cstring;
			cstring << "" << files[j]->ellapsed;
			//wprintw(win, "%-15s %-6d %s -> %s\n", cstring.str().c_str(), files[j]->thread, files[j]->relative_name.c_str(), boost::algorithm::join(files[j]->relative_output, " | ").c_str());
			wprintw(mMainWindow, "%-15s %-6d %-36s %d\n", cstring.str().c_str(), files[j]->thread, boost::uuids::to_string(files[j]->uuid).c_str(), files[j]->fileSize);
		}
		else
		{
			wprintw(mMainWindow, "%-15s %-6d %-36s %d\n", "", files[j]->thread, boost::uuids::to_string(files[j]->uuid).c_str(), files[j]->fileSize);
		}
	}
	wrefresh(mMainWindow);
}

void SlaveDisplay::DrawFooter(const std::vector<SlaveFileStatus*> cfiles) const
{
	mvwprintw(mFooterWindow, 0, 0, "files: %d/%d\n", mTesseractRunner.Done(), mTesseractRunner.Total());

	wrefresh(mFooterWindow);
}

void SlaveDisplay::DrawCommand() const
{
	if (!mIsEnd)
		mvwprintw(mControlWindow, 0, 0, "[CTRL+C] Abandon | [+]/[-] Increase/Decrease Thread\n");
	else
		mvwprintw(mControlWindow, 0, 0, "[ENTER] Exit\n");
	wrefresh(mControlWindow);
}

void SlaveDisplay::Draw()
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	DrawHeader();

	DrawBody(mFiles);

	DrawFooter(mFiles);

	DrawCommand();
}

void SlaveDisplay::Resize()
{
	Init(false);
	Draw();
}


void SlaveDisplay::ShowFile(SlaveFileStatus* str)
{
	mFiles.insert(mFiles.end(), str);
}

void SlaveDisplay::Run()
{
	int ch;
	while (!mIsTerminated)
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
				mTesseractRunner.AddThread();
			}
			else if (ch == '-')
			{
				mTesseractRunner.RemoveThread();
			}
			/*else if (ch == 'v')
			{
				currentView = (currentView + 1) % totalView;
				werase(win);
				Draw();
			}*/
			else if ((ch == KEY_ENTER || ch == '\n') && mIsEnd)
			{
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}
