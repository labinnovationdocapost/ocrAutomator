#pragma once

#include <string>
#include <ncurses.h>
#include <vector>
#include "FileStatus.h"
#include "FileSum.h"
struct FileSum;
using std::string;
#include "TesseractRunner.h"

class Display
{
private:
	WINDOW * mMainWindow;
	WINDOW * mTopWindow;
	WINDOW * mHeaderWindow;
	WINDOW * mFooterWindow;
	WINDOW * mControlWindow;
	int mScreenHeight, mScreenWidth;
	std::vector<FileStatus*> mFiles{};

	int mCurrentView = 0;
	int mTotalView = 2;

	std::mutex mThreadMutex;

	Docapost::IA::Tesseract::TesseractRunner& mTesseractRunner;

	boost::posix_time::ptime mTimeEnd;
	bool mIsEnd = false;
	bool mIsTerminated = false;

	void Init(bool create = true);
	void OnEnd();
public:
	explicit Display(Docapost::IA::Tesseract::TesseractRunner &tessR);
	~Display();

	void DrawHeader() const;
	void DrawBody(const std::vector<FileStatus*> files, FileSum& s) const;
	void DrawBodyNetwork(const std::vector<FileStatus*> files, FileSum& s) const;
	void DrawFooter(const std::vector<FileStatus*> cfiles, FileSum s) const;
	void DrawCommand() const;
	void Draw();

	void Resize();

	void ShowFile(FileStatus* str);
	void OnCanceled(FileStatus* str);
	void AddFile(FileStatus* file);

	void terminated(bool t) { mIsTerminated = t; }

	void Run();
};