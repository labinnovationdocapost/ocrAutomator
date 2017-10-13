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
using std::string;

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <boost/program_options.hpp>
#include <boost/date_time.hpp>
namespace po = boost::program_options;
using boost::program_options::value;

#include "TesseractRunner.h"

#if DISPLAY
#include "Display.h"

void resizeHandler(int sig);

Display* display;
#endif

std::mutex g_console_mutex;

void ShowHelp(char** argv, po::options_description& desc)
{
	std::cout << "Usage:\n";
	std::cout << argv[0] << " --help\n";
	std::cout << argv[0] << " [options...] /folder/of/images\n";
	std::cout << argv[0] << " [options...] --folder /folder/of/images [options...]\n";

	std::cout << "\nOptions:\n";
	std::cout << desc << "\n";

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

int main(int argc, char* argv[])
{
	// oblige le buffer desortie a etre thread safe
	std::ios_base::sync_with_stdio(true);
	int nb_process = 2;

	boost::program_options::positional_options_description pd;
	pd.add("folder", -1);

	po::options_description desc;
	desc.add_options()
		("psm", value<int>()->default_value(3)->value_name("NUM"), "Page Segmentation Mode")
		("oem", value<int>()->default_value(3)->value_name("NUM"), "Ocr Engine Mode")
		("lang,l", value <std::string>()->default_value("fra")->value_name("LANG"), "Langue utilisé pour l'OCR")
		("help,h", "")
		("parallel,p", value<int>()->value_name("NUM"), "Nombre de threads en parralèle")
		("output,o", value<std::string>()->value_name("DOSSIER"), "Dossier de sortie (défaut: dossier actuel)")
		("continue,c", "Ne pas ecraser les fichiers existant")
		("silent,s", "Ne pas afficher l'interface")
		("exif,e", "Copier l'image dans le fichier de sortie et écrire le résulat dans les Exif")
		("text,t", "Ecrire le résultat dans un fichier texte (.txt) dans le dossier de sortie")
		("folder,f", value<std::string>()->value_name("DOSSIER"), "");

	
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
			.options(desc).positional(pd).run(), vm);
		po::notify(vm);
	}
	catch(std::exception& e)
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

	if(!vm.count("folder"))
	{
		std::cout << "\nVeuiller indiquer un dossier a traiter\n\n";

		std::cout << "Tapez :\n";
		std::cout << "  " << argv[0] << " -h\n";
		std::cout << "Pour afficher l'aide\n\n";
		return 0;
	}


	if (!fs::is_directory(vm["folder"].as<std::string>()))
	{
		std::cout << "Le chemin "<< vm["folder"].as<std::string>() << " n'est pas un dossier valide\n";
		return 0;
	}

	if(vm.count("thread"))
	{
		try
		{
			nb_process = vm["thread"].as<int>();
		}
		catch(const std::exception &e)
		{
			
		}
	}

	bool resume = false;

	if (vm.count("continue"))
	{
		resume = true;
	}

	Docapost::IA::Tesseract::TesseractOutputFlags types = Docapost::IA::Tesseract::TesseractOutputFlags::None;
	if (vm.count("exif"))
	{
		types |= Docapost::IA::Tesseract::TesseractOutputFlags::Exif;
	}

	if (vm.count("text"))
	{
		types |= Docapost::IA::Tesseract::TesseractOutputFlags::Text;
	}
	if(types == Docapost::IA::Tesseract::TesseractOutputFlags::None)
	{
		types |= Docapost::IA::Tesseract::TesseractOutputFlags::Text;
	}

	Docapost::IA::Tesseract::TesseractRunner tessR(
		static_cast<tesseract::PageSegMode>(vm["psm"].as<int>()), 
		static_cast<tesseract::OcrEngineMode>(vm["oem"].as<int>()),
		vm["lang"].as<std::string>(), types);



	if (vm.count("output"))
	{
		tessR.SetOutput(vm["output"].as<std::string>());
	}
	else
	{
		tessR.SetOutput(boost::filesystem::current_path());
	}

	tessR.AddFolder(vm["folder"].as<std::string>(), resume);


	auto startProcess = boost::posix_time::second_clock::local_time();
	tessR.Run(nb_process);

#if DISPLAY
	if(vm.count("silent"))
	{
		tessR.Wait();
	}
	else
	{
		display = new Display(tessR);

		signal(SIGWINCH, resizeHandler);
		display->Run();

		delete display;
	}
#else
	tessR.Wait();
#endif



	auto processTime = tessR.GetEndTime() - tessR.GetStartTime();

	std::cout << "Total Files : " << tessR.GetNbFiles() << "\n";
	std::cout << "Start       : " << tessR.GetStartTime() << "\n";
	std::cout << "End         : " << tessR.GetEndTime() << "\n";
	std::cout << "Duration    : " << processTime << "\n";

	return 0;
}

#if DISPLAY
void resizeHandler(int sig)
{
	display->Resize();
}
#endif
