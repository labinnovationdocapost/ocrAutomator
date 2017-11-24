#pragma once

#include <string>
#include <ncurses.h>
#include <vector>
#include "SlaveFileStatus.h"
using std::string;
#include "TesseractSlaveRunner.h"
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

	Docapost::IA::Tesseract::TesseractSlaveRunner& mTesseractRunner;

	boost::posix_time::ptime mTimeEnd;
	bool mIsEnd = false;
	void Init(bool create = true);
	void OnEnd();
public:
	explicit SlaveDisplay(Docapost::IA::Tesseract::TesseractSlaveRunner &tessR);
	~SlaveDisplay();

	void DrawHeader() const;
	void DrawBody(const std::vector<SlaveFileStatus*> files) const;
	void DrawFooter(const std::vector<SlaveFileStatus*> cfiles) const;
	void DrawCommand() const;
	void Draw();

	void Resize();

	void ShowFile(SlaveFileStatus* str);

	void Run();
};