#include "Display.h"
#include "TesseractFactory.h"
#include <math.h>
#include "MasterLocalFileStatus.h"

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
		CloseHandle(mConsoleHandler);
	}
	mConsoleHandler = GetStdHandle(STD_OUTPUT_HANDLE);
	Clear();
	//auto Status = SetConsoleActiveScreenBuffer(mConsoleHandler);
}

void Display::OnEnd()
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	if (mIsEnd == true || mIsTerminated == true)
		return;
	mTimeEnd = boost::posix_time::second_clock::local_time();
	this->mIsEnd = true;

}

Display::Display(Docapost::IA::Tesseract::MasterProcessingWorker& tessR) : mTesseractRunner(tessR)
{

	Init();
	mStartProcessFileSignalConnection = tessR.onStartProcessFile.connect(boost::bind(&Display::ShowFile, this, _1));
	mProcessEndSignalConnection = tessR.onProcessEnd.connect(boost::bind(&Display::OnEnd, this));
	mFileCanceledSignalConnection = tessR.onFileCanceled.connect(boost::bind(&Display::OnCanceled, this, _1));
}


Display::~Display()
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	mStartProcessFileSignalConnection.disconnect();
	mProcessEndSignalConnection.disconnect();
	mFileCanceledSignalConnection.disconnect();
}


void Display::DrawHeader() const
{
}

int Display::GetAverageSize() const
{
	return std::floor(pow(log(mTesseractRunner.NbThreads() + mTesseractRunner.TotalRemoteThreads()), 3.5)) + 10;
}

void Display::Clear() {
	COORD topLeft = { 0, 0 };
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	SetConsoleCursorPosition(console, topLeft);
}

void Display::DrawBody(const std::unordered_set<MasterFileStatus*> files, FileSum& s)
{
	int i = 0;
	for (auto pfile = files.cbegin(); pfile != files.cend(); ++pfile)
	{
		if (*pfile == nullptr)
			break;

		auto plfile = dynamic_cast<MasterLocalFileStatus*>(*pfile);
		if ((*pfile)->isEnd)
		{
			if ((boost::posix_time::microsec_clock::local_time() - (*pfile)->end).total_seconds() > 5)
			{
				boost::lock_guard<std::mutex> lock(mThreadMutex);
				mFiles.erase(*pfile);
				mFilesCompleted.push_back(*pfile);
				continue;
			}

			std::stringstream cstring;
			cstring << "" << (*pfile)->ellapsed;
			//wprintw(win, "%-15s %-6d %s -> %s\n", cstring.str().c_str(), files[j]->thread, files[j]->relative_name.c_str(), boost::algorithm::join(files[j]->relative_output, " | ").c_str());
			//wprintw(mMainWindow, "%-15s %-6d %-15s %2d %s\n", cstring.str().c_str(), (*j)->thread, (*j)->hostname.c_str(), (*j)->filePosition, (*j)->relative_name.c_str());

			char text[1024];
			if (plfile != nullptr)
				sprintf(text, "%-15s %-6d %-15s %2d %s", cstring.str().c_str(), (*pfile)->thread, (*pfile)->hostname.c_str(), (*pfile)->filePosition, plfile->relative_name.c_str());
			else
				sprintf(text, "%-15s %-6d %-15s %2d %s", cstring.str().c_str(), (*pfile)->thread, (*pfile)->hostname.c_str(), (*pfile)->filePosition, (*pfile)->name.c_str());
			DWORD dwBytesWritten = 0;
			WriteConsoleOutputCharacter(mConsoleHandler, text, strlen(text), { 0, (SHORT)i++ }, &dwBytesWritten);
		}
		else
		{
			//wprintw(mMainWindow, "%-15s %-6d %-15s %2d %s\n", "", (*j)->thread, (*j)->hostname.c_str(), (*j)->filePosition, (*j)->relative_name.c_str());
			char text[1024];
			if (plfile != nullptr)
				sprintf(text, "%-15s %-6d %-15s %2d %s", "", (*pfile)->thread, (*pfile)->hostname.c_str(), (*pfile)->filePosition, plfile->relative_name.c_str());
			else
				sprintf(text, "%-15s %-6d %-15s %2d %s", "", (*pfile)->thread, (*pfile)->hostname.c_str(), (*pfile)->filePosition, (*pfile)->name.c_str());
			DWORD dwBytesWritten = 0;
			WriteConsoleOutputCharacter(mConsoleHandler, text, strlen(text), { 0, (SHORT)i++ }, &dwBytesWritten);
		}

	}

	auto max = GetAverageSize();
	for (auto j = mFilesCompleted.rbegin(); j != mFilesCompleted.rend() && s.count < max; ++j)
	{
		if ((*j)->isEnd)
		{
			s(*j);
		}
	}
}

void Display::DrawBodyNetwork(const std::unordered_set<MasterFileStatus*> files, FileSum& s)
{
}

void Display::DrawFooter(const std::unordered_set<MasterFileStatus*> cfiles, FileSum s)
{
}

void Display::DrawCommand() const
{
}

void Display::Draw()
{
	boost::lock_guard<std::mutex> lockLoop(mLoopMutex);
	DrawHeader();

	FileSum s{};

	if (mCurrentView == 0)
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
	mFiles.erase(str);
}

void Display::AddFile(MasterFileStatus* file)
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	mFiles.insert(file);
}


void Display::Run()
{
	//Clear();
	int ch;
	while (!mIsTerminated)
	{
		Draw();
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}
