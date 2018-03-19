#pragma once

#include <string>
#include <vector>
#include "Master/MasterFileStatus.h"
#include "Base/FileSum.h"
#include <unordered_set>
struct FileSum;
using std::string;
#include "Master/MasterProcessingWorker.h"
#include <Windows.h>

class Display
{
private:
	int mScreenHeight, mScreenWidth;
	std::unordered_set<MasterFileStatus*> mFiles;
	std::list<MasterFileStatus*> mFilesCompleted;

	HANDLE mConsoleHandler;

	int mCurrentView = 0;
	int mTotalView = 2;

	std::mutex mThreadMutex;
	std::mutex mLoopMutex;

	HANDLE mNewScreenBuffer;

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
	void Clear();
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