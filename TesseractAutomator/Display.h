#pragma once

#include <string>
#include <ncurses.h>
#include <vector>
#include "MasterFileStatus.h"
#include "FileSum.h"
struct FileSum;
using std::string;
#include "MasterProcessingWorker.h"

class Display
{
private:
	WINDOW * mMainWindow;
	WINDOW * mTopWindow;
	WINDOW * mHeaderWindow;
	WINDOW * mFooterWindow;
	WINDOW * mControlWindow;
	int mScreenHeight, mScreenWidth;
	std::vector<MasterFileStatus*> mFiles{};

	int mCurrentView = 0;
	int mTotalView = 2;

	std::mutex mThreadMutex;

	Docapost::IA::Tesseract::MasterProcessingWorker& mTesseractRunner;

	boost::posix_time::ptime mTimeEnd;
	bool mIsEnd = false;
	bool mIsTerminated = false;

	void Init(bool create = true);
	void OnEnd();
public:
	explicit Display(Docapost::IA::Tesseract::MasterProcessingWorker &tessR);
	~Display();

	void DrawHeader() const;
	void DrawBody(const std::vector<MasterFileStatus*> files, FileSum& s) const;
	void DrawBodyNetwork(const std::vector<MasterFileStatus*> files, FileSum& s) const;
	void DrawFooter(const std::vector<MasterFileStatus*> cfiles, FileSum s) const;
	void DrawCommand() const;
	void Draw();

	void Resize();

	void ShowFile(MasterFileStatus* str);
	void OnCanceled(MasterFileStatus* str);
	void AddFile(MasterFileStatus* file);

	void terminated(bool t) { mIsTerminated = t; }

	void Run();
};