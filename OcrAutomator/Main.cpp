#include "Main.h"

/*
 * Main d�di� au lancement de l'application en mode autonome, exclu des test
 */

int main(int argc, char** argv)
{
	Log::InitLogger();
	_main(argc, argv);
}