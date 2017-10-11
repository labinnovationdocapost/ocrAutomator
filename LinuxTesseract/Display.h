#pragma once

#include <string>
#include <ncurses.h>
#include <vector>
#include "FileStatus.h"
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

	std::mutex g_thread_mutex;

	Docapost::IA::Tesseract::TesseractRunner& tessR;

	boost::posix_time::ptime timeEnd;
	bool isEnd = false;
	void Init(bool create = true);
	void OnEnd();
public:
	explicit Display(Docapost::IA::Tesseract::TesseractRunner &tessR);
	~Display();

	void Draw();

	void Resize();

	void ShowFile(FileStatus* str);

	void Run();
};

