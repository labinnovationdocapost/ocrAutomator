#pragma once
#include "MasterProcessingWorker.h"
#include "SlaveProcessingWorker.h"

void MasterDisplayUI();
void SlaveDisplayUI();

extern Docapost::IA::Tesseract::MasterProcessingWorker* workerM;
extern Docapost::IA::Tesseract::SlaveProcessingWorker* workerS;

#include "Display.h"
#include "SlaveDisplay.h"

void resizeHandler(int sig);

Display* display;
SlaveDisplay* sdisplay;
