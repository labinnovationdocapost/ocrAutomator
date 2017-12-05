#include "Display.h"
#include "TesseractFactory.h"

#define COLOR_GREY 8

#define HTOP 4
#define HHEADER 1
#define HBOTTOM 1
#define HCTRL 1
#define HWIN(h) h - (HTOP + HHEADER + HBOTTOM + HCTRL)

void Display::Init(bool create)
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
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

void Display::OnEnd()
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	mTimeEnd = boost::posix_time::second_clock::local_time();
	this->mIsEnd = true;

	wbkgd(mFooterWindow, COLOR_PAIR(3));
	refresh();
}

Display::Display(Docapost::IA::Tesseract::MasterProcessingWorker& tessR) : mTesseractRunner(tessR)
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
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	endwin();
}


void Display::DrawHeader() const
{
	if (mTesseractRunner.Output().empty()) 
	{
		mvwprintw(mTopWindow, 0, 0, "Input : %s | Output: %s\n", mTesseractRunner.Input().c_str(), mTesseractRunner.Input().c_str());
	}
	else
	{
		mvwprintw(mTopWindow, 0, 0, "Input : %s | Exif Output: %s | Text Output : %s\n", mTesseractRunner.Input().c_str(), mTesseractRunner.Output()[Docapost::IA::Tesseract::OutputFlags::Exif].string().c_str(), mTesseractRunner.Output()[Docapost::IA::Tesseract::OutputFlags::Text].string().c_str());
	}
	auto& ocrFactory = mTesseractRunner.ocrFactory();
	auto tess = dynamic_cast<Docapost::IA::Tesseract::TesseractFactory*>(&ocrFactory);
	if(tess != nullptr)
	{
	if (mTesseractRunner.NbThreadToStop() > 0)
		mvwprintw(mTopWindow, 1, 0, "Threads Local/Remote: %d (-%d)/%d | Page Segmentation Mode: %d | Ocr Engine Mode: %d | Network: %s (%d)\n", mTesseractRunner.NbThreads(), mTesseractRunner.NbThreadToStop(), mTesseractRunner.TotalRemoteThreads(), tess->Psm(), tess->Oem(), mTesseractRunner.NetworkEnable() ? "On" : "Off", mTesseractRunner.Port());
	else
		mvwprintw(mTopWindow, 1, 0, "Threads Local/Remote: %d/%d | Page Segmentation Mode: %d | Ocr Engine Mode: %d | Network: %s (%d)\n", mTesseractRunner.NbThreads(), mTesseractRunner.TotalRemoteThreads(), tess->Psm(), tess->Oem(), mTesseractRunner.NetworkEnable() ? "On" : "Off", mTesseractRunner.Port());
	}

	if (mIsEnd)
	{
		std::stringstream cstring;
		cstring << "Start: " << mTesseractRunner.StartTime() << " | End: " << mTesseractRunner.EndTime() << " | Ellapsed: " << (mTesseractRunner.EndTime() - mTesseractRunner.StartTime());
		mvwprintw(mTopWindow, 2, 0, "%s\n", cstring.str().c_str());
	}
	else
	{
		std::stringstream cstring;
		cstring << "Start: " << mTesseractRunner.StartTime() << " | End: In Progress | Ellapsed: " << (boost::posix_time::second_clock::local_time() - mTesseractRunner.StartTime());
		mvwprintw(mTopWindow, 2, 0, "%s\n", cstring.str().c_str());
	}

	mvwprintw(mTopWindow, 3, 0, "Files Total: %d | Files Skip: %d | Mode: %d\n", mTesseractRunner.Total(), mTesseractRunner.Skip(), mTesseractRunner.OutputTypes());
	wrefresh(mTopWindow);
}

void Display::DrawBody(const std::vector<MasterFileStatus*> files, FileSum& s) const
{
	mvwprintw(mHeaderWindow, 0, 0, "%-15s %-6s %s\n", "Ellapsed", "Thread", "Origin");
	wrefresh(mHeaderWindow);

	wmove(mMainWindow, 0, 0);
	auto start = std::max(static_cast<int>(files.size()) - mScreenHeight, 0);
	auto fileToPrint = files.size();
	for (auto j = start; j < fileToPrint; j++)
	{
		if (files[j]->isEnd)
		{
			std::stringstream cstring;
			cstring << "" << files[j]->ellapsed;
			//wprintw(win, "%-15s %-6d %s -> %s\n", cstring.str().c_str(), files[j]->thread, files[j]->relative_name.c_str(), boost::algorithm::join(files[j]->relative_output, " | ").c_str());
			wprintw(mMainWindow, "%-15s %-6d %-15s %2d %s\n", cstring.str().c_str(), files[j]->thread,  files[j]->hostname.c_str(), files[j]->filePosition, files[j]->relative_name.c_str());
		}
		else
		{
			wprintw(mMainWindow, "%-15s %-6d %-15s %2d %s\n", "", files[j]->thread,  files[j]->hostname.c_str(), files[j]->filePosition, files[j]->relative_name.c_str());
		}

		s(files[j]);
	}
	wrefresh(mMainWindow);
}

void Display::DrawBodyNetwork(const std::vector<MasterFileStatus*> files, FileSum& s) const
{
	mvwprintw(mHeaderWindow, 0, 0, "%-20s %-6s\n", "Hostname", "Thread");
	wrefresh(mTopWindow);

	wmove(mMainWindow, 0, 0);
	for (auto j = std::max(static_cast<int>(files.size()) - mScreenHeight, 0); j < files.size(); j++)
	{
		s(files[j]);
	}
	wprintw(mMainWindow, "%-20s %-6d\n", "Master", mTesseractRunner.NbThreads());
	for(auto& slave : mTesseractRunner.Slaves())
	{
		wprintw(mMainWindow, "%-20s %-6d\n", slave.second->Name.c_str(), slave.second->NbThread);
	}
	wrefresh(mMainWindow);
}

void Display::DrawFooter(const std::vector<MasterFileStatus*> cfiles, FileSum s) const
{
	if (s.count > 0 && (mTesseractRunner.TotalRemoteThreads() + mTesseractRunner.NbThreads() > 0 || mIsEnd))
	{
		std::stringstream cstring;


		if (mIsEnd)
		{
			auto average = (mTesseractRunner.EndTime() - mTesseractRunner.StartTime()) / mTesseractRunner.Total();
			cstring << "files: " << mTesseractRunner.Done() << "/" << mTesseractRunner.Total() 
				<< "\t Average: " << std::setw(2) << std::setfill('0') << average.hours() << ":"
				<< std::setw(2) << std::setfill('0') << average.minutes() << ":"
				<< std::setw(2) << std::setfill('0') << average.seconds() << "."
				<< std::setw(3) << std::setfill('0') << average.fractional_seconds() / 1000 << "/Image"
				<< "\t Ellapsed: " << mTesseractRunner.EndTime() - mTesseractRunner.StartTime() << "\t Finish: " << mTesseractRunner.EndTime();
		}
		else
		{
			auto remaining = (s.sum / s.count / (mTesseractRunner.NbThreads() + mTesseractRunner.TotalRemoteThreads())) * (mTesseractRunner.Total() - cfiles.size());
			auto average = s.sum / s.count / (mTesseractRunner.NbThreads() + mTesseractRunner.TotalRemoteThreads());
			cstring << "files: " << mTesseractRunner.Done() << "/" << mTesseractRunner.Total()
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

		mvwprintw(mFooterWindow, 0, 0, cstring.str().c_str());
	}
	else
	{
		mvwprintw(mFooterWindow, 0, 0, "files: %d/%d\t Average: Unknown\t Remaining: Unknown\t Estimated End: Unknown\n", mTesseractRunner.Done(), mTesseractRunner.Total());
	}

	wrefresh(mFooterWindow);
}

void Display::DrawCommand() const
{
	if (!mIsEnd)
		mvwprintw(mControlWindow, 0, 0, "[CTRL+C] Abandon | [+]/[-] Increase/Decrease Thread | [v] Change view\n");
	else
		mvwprintw(mControlWindow, 0, 0, "[ENTER] Exit\n");
	wrefresh(mControlWindow);
}

void Display::Draw()
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	DrawHeader();

	FileSum s{};

	if(mCurrentView == 0)
		DrawBody(mFiles, s);
	if (mCurrentView == 1)
		DrawBodyNetwork(mFiles, s);

	DrawFooter(mFiles, s);

	DrawCommand();
}

void Display::Resize()
{
	Init(false);
	Draw();
}


void Display::ShowFile(MasterFileStatus* file)
{
	AddFile(file);
}
void Display::OnCanceled(MasterFileStatus* str)
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	mFiles.erase(std::remove(mFiles.begin(), mFiles.end(), str), mFiles.end());
	werase(mMainWindow);
}

void Display::AddFile(MasterFileStatus* file)
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	mFiles.insert(mFiles.end(), file);
}

void Display::Run()
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
			else if (ch == 'v')
			{
				mCurrentView = (mCurrentView + 1) % mTotalView;
				werase(mMainWindow);
				Draw();
			}
			else if ((ch == KEY_ENTER || ch == '\n') && mIsEnd)
			{
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}
