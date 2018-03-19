
#include <boost/thread/thread.hpp>
#include "Base/Error.h"
#include "DisplayManager.h"
#include "Master/Display.h"
#include "Slave/SlaveDisplay.h"


#if DISPLAY
void resizeHandler(int sig)
{
	if (display != nullptr)
		display->Resize();
	if (sdisplay != nullptr)
		sdisplay->Resize();
}
#endif

void MasterDisplayUI()
{
	CatchAllErrorSignals();
	CatchAllExceptions();
	display = new Display(*workerM);

	display->Run();

	delete display;
	
}

void SlaveDisplayUI()
{
	CatchAllErrorSignals();
	sdisplay = new SlaveDisplay(*workerS);

	sdisplay->Run();

	delete sdisplay;
}
