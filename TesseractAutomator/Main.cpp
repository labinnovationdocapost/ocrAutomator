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
#include "SlaveProcessingWorker.h"
#include <google/protobuf/extension_set.h>
#include "Error.h"
#include "TesseractFactory.h"
#include "ImageFormatEnum.h"
using std::string;

#include <tesseract/baseapi.h>
#include <dirent.h>
#include <sys/types.h>
#include <boost/program_options.hpp>
#include <boost/date_time.hpp>
#include <boost/unordered_map.hpp>
#include <stdlib.h>
#include <unistd.h>

#define __USE_GNU

namespace po = boost::program_options;
using boost::program_options::value;

#include "MasterProcessingWorker.h"
#include "Network.h"

#if DISPLAY
#include "Display.h"
#include "SlaveDisplay.h"

void resizeHandler(int sig);

Display* display;
SlaveDisplay* sdisplay;

#endif

Docapost::IA::Tesseract::MasterProcessingWorker* workerM;
Docapost::IA::Tesseract::SlaveProcessingWorker* workerS;

boost::thread* th = nullptr;
std::mutex g_console_mutex;

//./TesseractAutomator -i "/mnt/e/LinuxHeader/rpa2" -p 3 --PSM 1 --OEM 3 -l fra -e /mnt/e/RPA/OutputE -t /mnt/e/RPA/OutputT -f
//-i /mnt/e/LinuxHeader/rpa2/ -p 1 --PSM 1 --OEM 3 -l fra -o /mnt/e/LinuxHeader/rpa3/ -e -t /root/outputT/ -s

void ShowHelp(char** argv, std::vector<std::pair<string, po::options_description&>>& descs)
{
	std::cout << "Usage:\n";
	std::cout << argv[0] << " --help\n";
	std::cout << argv[0] << " [options...] /folder/of/images\n";
	std::cout << argv[0] << " [options...] --input /folder/of/images [options...]\n";

	for (auto desc : descs)
	{
		std::cout << "\n"<< desc.first <<":\n";
		std::cout << desc.second << std::endl;
	}

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

Image type:
  - png
  - jpg (default)
)V0G0N";
}


/*void SetCapability()
{
	cap_value_t cap_values[] = { CAP_NET_BROADCAST };
	cap_t caps;
	int res = 0;
	caps = cap_get_proc();
	res = cap_set_flag(caps, CAP_PERMITTED, 2, cap_values, CAP_SET);
	res = cap_set_proc(caps);
	prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
	cap_free(caps);

	caps = cap_get_proc();
	res = cap_set_flag(caps, CAP_EFFECTIVE, 2, cap_values, CAP_SET);
	res = cap_set_proc(caps);
	cap_free(caps);
}*/

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

	boost::unordered_map <Docapost::IA::Tesseract::OutputFlags, fs::path> map;
	Docapost::IA::Tesseract::OutputFlags types = Docapost::IA::Tesseract::OutputFlags::None;
	if (vm.count("exif"))
	{
		types |= Docapost::IA::Tesseract::OutputFlags::Exif;
		if (vm["exif"].as<std::string>().empty())
		{
			map[Docapost::IA::Tesseract::OutputFlags::Exif] = vm["output"].as<std::string>();
		}
		else
		{
			map[Docapost::IA::Tesseract::OutputFlags::Exif] = vm["exif"].as<std::string>();
		}
	}

	if (vm.count("text"))
	{
		types |= Docapost::IA::Tesseract::OutputFlags::Text;
		if (vm["text"].as<std::string>().empty())
		{
			map[Docapost::IA::Tesseract::OutputFlags::Text] = vm["output"].as<std::string>();
		}
		else
		{
			map[Docapost::IA::Tesseract::OutputFlags::Text] = vm["text"].as<std::string>();
		}
	}

	if (types == Docapost::IA::Tesseract::OutputFlags::None)
	{
		types |= Docapost::IA::Tesseract::OutputFlags::Text;
		map[Docapost::IA::Tesseract::OutputFlags::Text] = vm["output"].as<std::string>();
	}

	if (vm.count("prefixe"))
	{
		types |= Docapost::IA::Tesseract::OutputFlags::Flattern;
	}

	Docapost::IA::Tesseract::TesseractFactory factory{};
	factory.Lang(vm["lang"].as<std::string>());
	factory.Oem(static_cast<tesseract::OcrEngineMode>(vm["oem"].as<int>()));
	factory.Psm(static_cast<tesseract::PageSegMode>(vm["psm"].as<int>()));

	if (vm.count("image"))
	{
		if (vm["image"].as<std::string>() == "png")
		{
			factory.ImageFormat(Docapost::IA::Tesseract::ImageFormatEnum::PNG);
		}
		else if (vm["image"].as<std::string>() == "jpg")
		{
			factory.ImageFormat(Docapost::IA::Tesseract::ImageFormatEnum::JPG);
		}
		else
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Option " << vm["image"].as<std::string>() << " unrecognized for field [image], fallback to jpg";
			factory.ImageFormat(Docapost::IA::Tesseract::ImageFormatEnum::JPG);
		}
	}

	workerM = new Docapost::IA::Tesseract::MasterProcessingWorker(factory, types, vm["port"].as<int>());

	if (vm.count("prefixe"))
	{
		workerM->Separator(vm["prefixe"].as<std::string>());
	}


#if DISPLAY
	if (!vm.count("silent"))
	{
		th = new boost::thread([&]()
		{
			CatchAllErrorSignals();
			CatchAllExceptions();
			display = new Display(*workerM);

			signal(SIGWINCH, resizeHandler);
			display->Run();

			delete display;
		});
	}
#endif


	workerM->SetOutput(map);

	workerM->AddFolder(vm["input"].as<std::string>(), resume);

	workerM->Run(nb_process);

#if DISPLAY
	if (vm.count("silent"))
	{
		workerM->Wait();
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

	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cleaning MasterProcessingWorker instance";

	auto processTime = workerM->EndTime() - workerM->StartTime();

	std::cout << "Total Files : " << workerM->Total() << "\n";
	std::cout << "Start       : " << workerM->StartTime() << "\n";
	std::cout << "End         : " << workerM->EndTime() << "\n";
	std::cout << "Duration    : " << processTime << "\n";

	delete workerM;
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

	Docapost::IA::Tesseract::TesseractFactory factory{};
	workerS = new Docapost::IA::Tesseract::SlaveProcessingWorker{ factory, vm["port"].as<int>(), vm["ip"].as<std::string>() };
#if DISPLAY
	if (!vm.count("silent"))
	{
		th = new boost::thread([&]()
		{
			CatchAllErrorSignals();
			sdisplay = new SlaveDisplay(*workerS);

			signal(SIGWINCH, resizeHandler);
			sdisplay->Run();

			delete sdisplay;
		});
	}
#endif	

	auto pth = workerS->Run(nb_process);

#if DISPLAY
	if (vm.count("silent"))
	{
		pth->join();
	}
	else
	{
		//pth->detach();
		if (th != nullptr)
		{ 
			th->join();
			delete th;
		}
	}
#else

	tessSR.Wait(); .Wait();
#endif


	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cleaning SlaveProcessingWorker instance";
}

int main(int argc, char* argv[])
{
#if DISPLAY
	CatchAllErrorSignals();
	CatchAllExceptions();
#endif
	Log::InitLogger();

	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Starting Logger";
	//SetCapability();


	// oblige le buffer desortie a etre thread safe
	std::ios_base::sync_with_stdio(true);

	boost::program_options::positional_options_description pd;
	pd.add("input", -1);

	po::options_description globalDesc;
	globalDesc.add_options()
		("version,v", "Affiche le numero de version de l'application")
		("help,h", "Affiche l'aide");

	po::options_description shareDesc;
	shareDesc.add_options()
		("parallel,p", value<int>()->value_name("NUM")->default_value(2), "Nombre de threads en parrallele")
		("silent,s", "Ne pas afficher l'interface")
		("port", value<int>()->value_name("PORT")->default_value(12000), "Utiliser le port reseau definit pour toute communication");

	po::options_description desc;
	desc.add_options()
		("psm", value<int>()->default_value(3)->value_name("NUM"), "Page Segmentation Mode")
		("oem", value<int>()->default_value(3)->value_name("NUM"), "Ocr Engine Mode")
		("lang,l", value <std::string>()->default_value("fra")->value_name("LANG"), "Langue utilise pour l'OCR")
		("output,o", value<std::string>()->value_name("DOSSIER")->default_value(boost::filesystem::current_path().string()), "Dossier de sortie (defaut: dossier actuel)")
		("continue,c", "le fichier (ou la page pour le PDF) n'est pas traite si le fichier text et/ou l'exif existe deja")
		("exif,e", value<std::string>()->value_name("DOSSIER")->default_value("")->implicit_value(""), "Copier l'image dans le fichier de sortie et écrire le resulat dans les Exif. Si non sépcifié le paramètre --output est utilisé")
		("text,t", value<std::string>()->value_name("DOSSIER")->default_value("")->implicit_value(""), "Ecrire le resultat dans un fichier texte (.txt) dans le dossier de sortie. Si non sépcifié le paramètre --output est utilisé")
		("prefixe,f", value<std::string>()->value_name("SEPARATOR")->default_value("__")->implicit_value("__"), "Ajout le chemin relatif a [input] en prefixe du fichier.Defaut: __")
		("input,i", value<std::string>()->value_name("DOSSIER"), "Dossier d'entree a partir de laquelle seront listee les fichiers a traiter")
		("image", value<std::string>()->value_name("FORMAT")->default_value("jpg"), "Format de l'image de sortie");

	po::options_description slaveDesc;
	slaveDesc.add_options()
		("slave,a", "Le programme agira comme un noeud de calcul et cherchera a ce connecter a un noeud maitre disponible pour récupérer des images a traiter")
		("ip", value<std::string>()->value_name("IP")->default_value(""), "Ip specifique à laquelle ce connecter");;

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
		std::vector<std::pair<string, po::options_description&>>  opts = { {"Options",globalDesc}, { "Communes", shareDesc}, { "Master", desc },{ "Slave", slaveDesc} };
		ShowHelp(argv, opts);
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
		freopen("/var/log/TesseractAutomatorStdErr_Slave.log", "w", stderr);
		Slave(argv, vm);
	}
	else
	{
		freopen("/var/log/TesseractAutomatorStdErr_Master.log", "w", stderr);
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
