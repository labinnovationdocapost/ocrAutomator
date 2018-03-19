#pragma once

#include <string>
#include <vector>
#include "Slave/SlaveFileStatus.h"
using std::string;
#include "Slave/SlaveProcessingWorker.h"
#include "Base/FileSum.h"

class SlaveDisplay
{
private:
	int mScreenHeight, mScreenWidth;
	std::vector<SlaveFileStatus*> mFiles{};

	int mCurrentView = 0;
	int mTotalView = 1;

	std::mutex mThreadMutex;
	bool mIsTerminated = false;

	Docapost::IA::Tesseract::SlaveProcessingWorker& mTesseractRunner;

	boost::signals2::connection mStartProcessFileSignalConnection;
	boost::signals2::connection mProcessEndSignalConnection;
	boost::signals2::connection mNewBatchSignalConnection;

	boost::posix_time::ptime mTimeEnd;
	bool mIsEnd = false;
	void Init(bool create = true);
	void OnEnd();
	void OnNewBatch();
public:
	explicit SlaveDisplay(Docapost::IA::Tesseract::SlaveProcessingWorker &tessR);
	~SlaveDisplay();

	void DrawHeader() const;
	void DrawBody(const std::vector<SlaveFileStatus*> files) const;
	void DrawFooter(const std::vector<SlaveFileStatus*> cfiles) const;
	void DrawCommand() const;
	void Draw();

	void Resize();

	void ShowFile(SlaveFileStatus* str);
	void terminated(bool t) { mIsTerminated = t; }

	void Run();
};