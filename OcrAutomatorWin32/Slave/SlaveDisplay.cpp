#include "Slave/SlaveDisplay.h"
#include "Ocr/Tesseract/TesseractFactory.h"
#include <boost/uuid/uuid_io.hpp>

#define COLOR_GREY 8

#define HTOP 4
#define HHEADER 1
#define HBOTTOM 1
#define HCTRL 1
#define HWIN(h) h - (HTOP + HHEADER + HBOTTOM + HCTRL)

void SlaveDisplay::Init(bool create)
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	if (!create)
	{
	}
}

void SlaveDisplay::OnEnd()
{
	mTimeEnd = boost::posix_time::second_clock::local_time();
	this->mIsEnd = true;

}

void SlaveDisplay::OnNewBatch()
{
	if(this->mIsEnd)
	{
		this->mIsEnd = false;

	}
}

SlaveDisplay::SlaveDisplay(Docapost::IA::Tesseract::SlaveProcessingWorker& tessR) : mTesseractRunner(tessR)
{

	Init();
	mStartProcessFileSignalConnection = tessR.onStartProcessFile.connect(boost::bind(&SlaveDisplay::ShowFile, this, _1));
	mProcessEndSignalConnection = tessR.onProcessEnd.connect(boost::bind(&SlaveDisplay::OnEnd, this));
	mNewBatchSignalConnection = tessR.onNewBatch.connect(boost::bind(&SlaveDisplay::OnNewBatch, this));
}


SlaveDisplay::~SlaveDisplay()
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	mStartProcessFileSignalConnection.disconnect();
	mProcessEndSignalConnection.disconnect();
	mNewBatchSignalConnection.disconnect();
}


void SlaveDisplay::DrawHeader() const
{
}

void SlaveDisplay::DrawBody(const std::vector<SlaveFileStatus*> files) const
{
}

void SlaveDisplay::DrawFooter(const std::vector<SlaveFileStatus*> cfiles) const
{
}

void SlaveDisplay::DrawCommand() const
{
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
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}
