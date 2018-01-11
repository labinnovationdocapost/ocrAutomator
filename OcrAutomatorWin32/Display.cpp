#include "Display.h"
#include "TesseractFactory.h"
#include <math.h>

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
	}
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

void Display::DrawBody(const std::list<MasterFileStatus*> files, FileSum& s) const
{
}

void Display::DrawBodyNetwork(const std::list<MasterFileStatus*> files, FileSum& s) const
{
}

void Display::DrawFooter(const std::list<MasterFileStatus*> cfiles, FileSum s) const
{
}

void Display::DrawCommand() const
{
}

void Display::Draw()
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
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
	mFiles.remove(str);
}

void Display::AddFile(MasterFileStatus* file)
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	mFiles.push_back(file);
}

void Display::Run()
{
	int ch;
	while (!mIsTerminated)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}
