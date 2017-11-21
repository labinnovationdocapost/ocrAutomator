#include <cstdio>

#include <string>
#include <stack>
#include <iostream>
#include <mutex>
#include <thread>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <csignal>
#include "Version.h"
#include "TesseractSlaveRunner.h"
#include <google/protobuf/extension_set.h>
#include <google/protobuf/extension_set.h>
#include "Error.h"
using std::string;

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <boost/program_options.hpp>
#include <boost/date_time.hpp>
#include <boost/unordered_map.hpp>
#include <execinfo.h>
#include <stdlib.h>
#include <unistd.h>

#define __USE_GNU
#include <ucontext.h>

namespace po = boost::program_options;
using boost::program_options::value;

#include "TesseractRunner.h"
#include "Network.h"

#if DISPLAY
#include "Display.h"
#include "SlaveDisplay.h"

void resizeHandler(int sig);

Display* display;
SlaveDisplay* sdisplay;

#endif

std::thread* th = nullptr;
std::mutex g_console_mutex;

//./TesseractAutomator -i "/mnt/e/LinuxHeader/rpa2" -p 3 --PSM 1 --OEM 3 -l fra -e /mnt/e/RPA/OutputE -t /mnt/e/RPA/OutputT -f
//-i /mnt/e/LinuxHeader/rpa2/ -p 1 --PSM 1 --OEM 3 -l fra -o /mnt/e/LinuxHeader/rpa3/ -e -t /root/outputT/ -s

void ShowHelp(char** argv, po::options_description& desc)
{
	std::cout << "Usage:\n";
	std::cout << argv[0] << " --help\n";
	std::cout << argv[0] << " [options...] /folder/of/images\n";
	std::cout << argv[0] << " [options...] --input /folder/of/images [options...]\n";

	std::cout << "\nOptions:\n";
	std::cout << desc << std::endl;

	std::cout << R"V0G0N(
Information sur --exif et --text
Si aucun dossier n'est specifie le dossier utilise sera celui défini par --output. 
Si --output n'est pas definit, le dossier de sortie sera le dossier courrant.
Exemple: 
)V0G0N";
	std::cout << argv[0] << " --input /folder/of/images -et --output /output/folder\n";
	std::cout << argv[0] << " --input /folder/of/images -e --text /text/output/folder --output /output/folder\n";
	std::cout << argv[0] << " --input /folder/of/images --exif /image/output/folder --text /text/output/folder\n";

	std::cout << std::endl;

	std::cout << R"V0G0N(
Page segmentation modes:
  0    Orientation and script detection (OSD) only.
  1    Automatic page segmentation with OSD.
  2    Automatic page segmentation, but no OSD, or OCR.
  3    Fully automatic page segmentation, but no OSD. (Default)
  4    Assume a single column of text of variable sizes.
  5    Assume a single uniform block of vertically aligned text.
  6    Assume a single uniform block of text.
  7    Treat the image as a single text line.
  8    Treat the image as a single word.
  9    Treat the image as a single word in a circle.
 10    Treat the image as a single character.
 11    Sparse text. Find as much text as possible in no particular order.
 12    Sparse text with OSD.
 13    Raw line. Treat the image as a single text line,
                        bypassing hacks that are Tesseract-specific.
OCR Engine modes:
  0    Original Tesseract only.
  1    Neural nets LSTM only.
  2    Tesseract + LSTM.
  3    Default, based on what is available.
)V0G0N";
}


void Master(char** argv, po::variables_map& vm)
{
	int nb_process = 1;
	if (vm.count("parallel"))
	{
		try
		{
			nb_process = vm["parallel"].as<int>();
		}
		catch (const std::exception &e)
		{

		}
	}

	if (!vm.count("input"))
	{
		std::cout << "\nVeuiller indiquer un dossier a traiter\n\n";

		std::cout << "Tapez :\n";
		std::cout << "  " << argv[0] << " -h\n";
		std::cout << "Pour afficher l'aide\n\n";
		return;
	}


	if (!fs::is_directory(vm["input"].as<std::string>()))
	{
		std::cout << "Le chemin " << vm["input"].as<std::string>() << " n'est pas un dossier valide\n";
		return;
	}
	bool resume = false;

	if (vm.count("continue"))
	{
		resume = true;
	}

	boost::unordered_map <Docapost::IA::Tesseract::TesseractOutputFlags, fs::path> map;
	Docapost::IA::Tesseract::TesseractOutputFlags types = Docapost::IA::Tesseract::TesseractOutputFlags::None;
	if (vm.count("exif"))
	{
		types |= Docapost::IA::Tesseract::TesseractOutputFlags::Exif;
		if (vm["exif"].as<std::string>().empty())
		{
			map[Docapost::IA::Tesseract::TesseractOutputFlags::Exif] = vm["output"].as<std::string>();
		}
		else
		{
			map[Docapost::IA::Tesseract::TesseractOutputFlags::Exif] = vm["exif"].as<std::string>();
		}
	}

	if (vm.count("text"))
	{
		types |= Docapost::IA::Tesseract::TesseractOutputFlags::Text;
		if (vm["text"].as<std::string>().empty())
		{
			map[Docapost::IA::Tesseract::TesseractOutputFlags::Text] = vm["output"].as<std::string>();
		}
		else
		{
			map[Docapost::IA::Tesseract::TesseractOutputFlags::Text] = vm["text"].as<std::string>();
		}
	}

	if (types == Docapost::IA::Tesseract::TesseractOutputFlags::None)
	{
		types |= Docapost::IA::Tesseract::TesseractOutputFlags::Text;
		map[Docapost::IA::Tesseract::TesseractOutputFlags::Text] = vm["output"].as<std::string>();
	}

	if (vm.count("prefixe"))
	{
		types |= Docapost::IA::Tesseract::TesseractOutputFlags::Flattern;
	}


	Docapost::IA::Tesseract::TesseractRunner tessR(
		static_cast<tesseract::PageSegMode>(vm["psm"].as<int>()),
		static_cast<tesseract::OcrEngineMode>(vm["oem"].as<int>()),
		vm["lang"].as<std::string>(), types);

	if (vm.count("prefixe"))
	{
		tessR.Separator(vm["prefixe"].as<std::string>());
	}

#if DISPLAY
	std::thread* th = nullptr;
	if (!vm.count("silent"))
	{
		th = new std::thread([&]()
		{
			CatchAllErrorSignals();
			display = new Display(tessR);

			signal(SIGWINCH, resizeHandler);
			display->Run();

			delete display;
		});
	}
#endif


	/*Network nt{ 12000 };
	nt.InitBroadcastReceiver();
	nt.InitComm();
	nt.Start();*/

	tessR.SetOutput(map);

	tessR.AddFolder(vm["input"].as<std::string>(), resume);


	auto startProcess = boost::posix_time::second_clock::local_time();
	tessR.Run(nb_process);

#if DISPLAY
	if (vm.count("silent"))
	{
		tessR.Wait();
	}
	else
	{
		if (th != nullptr)
		{
			th->join();
			delete th;
		}
	}
#else
	mTesseractRunner.Wait();
#endif

	auto processTime = tessR.EndTime() - tessR.StartTime();

	std::cout << "Total Files : " << tessR.Total() << "\n";
	std::cout << "Start       : " << tessR.StartTime() << "\n";
	std::cout << "End         : " << tessR.EndTime() << "\n";
	std::cout << "Duration    : " << processTime << "\n";
}

void Slave(char** argv, po::variables_map& vm)
{
	int nb_process = 1;
	if (vm.count("parallel"))
	{
		try
		{
			nb_process = vm["parallel"].as<int>();
		}
		catch (const std::exception &e)
		{

		}
	}

	Docapost::IA::Tesseract::TesseractSlaveRunner tessSR{};
#if DISPLAY
	if (!vm.count("silent"))
	{
		th = new std::thread([&]()
		{
			CatchAllErrorSignals();
			sdisplay = new SlaveDisplay(tessSR);

			signal(SIGWINCH, resizeHandler);
			sdisplay->Run();

			delete sdisplay;
		});
	}
#endif	

	auto pth = tessSR.Run(nb_process);

#if DISPLAY
	if (vm.count("silent"))
	{
		pth->join();
	}
	else
	{
		if (th != nullptr)
		{
			th->join();
			delete th;
		}
	}
#else

	tessSR.Wait();.Wait();
#endif
}

int main(int argc, char* argv[])
{
#if DISPLAY
	CatchAllErrorSignals();
	CatchAllExceptions();
#endif

	// oblige le buffer desortie a etre thread safe
	std::ios_base::sync_with_stdio(true);
	int nb_process;

	boost::program_options::positional_options_description pd;
	pd.add("input", -1);

	po::options_description globalDesc;
	globalDesc.add_options()
		("version,v", "Affiche le numero de version de l'application")
		("help,h", "");

	po::options_description shareDesc;
	shareDesc.add_options()
		("parallel,p", value<int>()->value_name("NUM")->default_value(2), "Nombre de threads en parrallele")
		("silent,s", "Ne pas afficher l'interface"); 

	po::options_description desc;
	desc.add_options()
		("psm", value<int>()->default_value(3)->value_name("NUM"), "Page Segmentation Mode")
		("oem", value<int>()->default_value(3)->value_name("NUM"), "Ocr Engine Mode")
		("lang,l", value <std::string>()->default_value("fra")->value_name("LANG"), "Langue utilise pour l'OCR")
		("output,o", value<std::string>()->value_name("DOSSIER")->default_value(boost::filesystem::current_path().string()), "Dossier de sortie (defaut: dossier actuel)")
		("continue,c", "Ne pas ecraser les fichiers existant")
		("exif,e", value<std::string>()->value_name("DOSSIER")->default_value("")->implicit_value(""), "Copier l'image dans le fichier de sortie et écrire le resulat dans les Exif. Si non sépcifié le paramètre --output est utilisé")
		("text,t", value<std::string>()->value_name("DOSSIER")->default_value("")->implicit_value(""), "Ecrire le resultat dans un fichier texte (.txt) dans le dossier de sortie. Si non sépcifié le paramètre --output est utilisé")
		("prefixe,f", value<std::string>()->value_name("SEPARATOR")->default_value("__")->implicit_value("__"), "Ajout le chemin relatif a [input] en prefixe du fichier.Defaut: __")
		("input,i", value<std::string>()->value_name("DOSSIER"), "");

	po::options_description slaveDesc;
	slaveDesc.add_options()
		("slave,a", "Le programme agira comme un noeu de calcul et cherchera a ce connecter a un noeud maitre disponible pour récupérer des images a traiter");

	po::options_description cmdline_options;
	cmdline_options.add(globalDesc).add(desc).add(slaveDesc).add(shareDesc);

	po::variables_map vm;
	try
	{
		po::store(po::command_line_parser(argc, argv).style(
			boost::program_options::command_line_style::allow_short |
			boost::program_options::command_line_style::short_allow_next |
			boost::program_options::command_line_style::allow_dash_for_short |
			boost::program_options::command_line_style::allow_sticky |
			boost::program_options::command_line_style::case_insensitive |
			boost::program_options::command_line_style::unix_style)
			.options(cmdline_options).positional(pd).run(), vm);
		po::notify(vm);
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << "\n\n";
		std::cout << "Tapez :\n";
		std::cout << "  " << argv[0] << " -h\n";
		std::cout << "Pour afficher l'aide\n\n";
		return 0;
	}


	if (vm.count("help")) {
		ShowHelp(argv, desc);
		return 0;
	}

	if (vm.count("version"))
	{
		std::cout << "Tesseract Automator" << std::endl;
		std::cout << "Version: " << VERSION;
#if NDEBUG
		std::cout << " RELEASE";
#else
		std::cout << " DEBUG";
#endif
		std::cout << std::endl;

		std::cout << "Date: " << __DATE__ << " " << __TIME__ << std::endl;
		return 0;
	}


	if (vm.count("slave"))
	{
		Slave(argv, vm);
	}
	else
	{
		Master(argv, vm);
	}


	return 0;
}

#if DISPLAY
void resizeHandler(int sig)
{
	if (display != nullptr)
		display->Resize();
	if (sdisplay != nullptr)
		sdisplay->Resize();
}
#endif
