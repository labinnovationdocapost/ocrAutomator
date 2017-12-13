#pragma once

#include <string>
#include <ncurses.h>
#include <vector>
#include "SlaveFileStatus.h"
using std::string;
#include "SlaveProcessingWorker.h"
#include "FileSum.h"

class SlaveDisplay
{
private:
	WINDOW * mMainWindow;
	WINDOW * mTopWindow;
	WINDOW * mHeaderWindow;
	WINDOW * mFooterWindow;
	WINDOW * mControlWindow;
	int mScreenHeight, mScreenWidth;
	std::vector<SlaveFileStatus*> mFiles{};

	int mCurrentView = 0;
	int mTotalView = 1;

	std::mutex mThreadMutex;
	bool mIsTerminated = false;

	Docapost::IA::Tesseract::SlaveProcessingWorker& mTesseractRunner;

	boost::posix_time::ptime mTimeEnd;
	bool mIsEnd = false;
	void Init(bool create = true);
	void OnEnd();
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