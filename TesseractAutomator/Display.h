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
	WINDOW * win;
	WINDOW * top;
	WINDOW * header;
	WINDOW * bottom;
	WINDOW * ctrl;
	int h, w;
	std::vector<FileStatus*> files{};

	int currentView = 0;
	int totalView = 2;

	std::mutex g_thread_mutex;

	Docapost::IA::Tesseract::TesseractRunner& tessR;

	boost::posix_time::ptime timeEnd;
	bool isEnd = false;
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

	void Run();
};

