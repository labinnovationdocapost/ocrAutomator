#pragma once

#include <string>
#include <ncurses.h>
#include <vector>
#include "Master/MasterFileStatus.h"
#include "Base/FileSum.h"
#include <unordered_set>
struct FileSum;
using std::string;
#include "Master/MasterProcessingWorker.h"

class Display
{
private:
	WINDOW * mMainWindow;
	WINDOW * mTopWindow;
	WINDOW * mHeaderWindow;
	WINDOW * mFooterWindow;
	WINDOW * mControlWindow;
	int mScreenHeight, mScreenWidth;
	std::unordered_set<MasterFileStatus*> mFiles;
	std::list<MasterFileStatus*> mFilesCompleted;

	int mCurrentView = 0;
	int mTotalView = 2;

	std::mutex mThreadMutex;
	std::mutex mLoopMutex;

	Docapost::IA::Tesseract::MasterProcessingWorker& mTesseractRunner;

	boost::posix_time::ptime mTimeEnd;
	bool mIsEnd = false;
	bool mIsTerminated = false;

	boost::signals2::connection mStartProcessFileSignalConnection;
	boost::signals2::connection mProcessEndSignalConnection;
	boost::signals2::connection mFileCanceledSignalConnection;

	int GetAverageSize() const;
	void Init(bool create = true);
	void OnEnd();
public:
	explicit Display(Docapost::IA::Tesseract::MasterProcessingWorker &tessR);
	~Display();

	void DrawHeader() const;
	void DrawBody(const std::unordered_set<MasterFileStatus*> files, FileSum& s);
	void DrawBodyNetwork(const std::unordered_set<MasterFileStatus*> files, FileSum& s);
	void DrawFooter(const std::unordered_set<MasterFileStatus*> files, FileSum s);
	void DrawCommand() const;
	void Draw();

	void Resize();

	void ShowFile(MasterFileStatus* str);
	void OnCanceled(MasterFileStatus* str);
	void AddFile(MasterFileStatus* file);

	void terminated(bool t) { mIsTerminated = t; }

	void Run();
};