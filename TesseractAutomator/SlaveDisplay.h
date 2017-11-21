#pragma once

#include <string>
#include <ncurses.h>
#include <vector>
#include "FileStatus.h"
using std::string;
#include "TesseractSlaveRunner.h"
#include "FileSum.h"

class SlaveDisplay
{
private:
	WINDOW * win;
	WINDOW * top;
	WINDOW * header;
	WINDOW * bottom;
	WINDOW * ctrl;
	int h, w;
	std::vector<SlaveFileStatus*> files{};

	int currentView = 0;
	int totalView = 1;

	std::mutex g_thread_mutex;

	Docapost::IA::Tesseract::TesseractSlaveRunner& tessR;

	boost::posix_time::ptime timeEnd;
	bool isEnd = false;
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