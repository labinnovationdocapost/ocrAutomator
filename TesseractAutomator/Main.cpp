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
#include <boost/unordered_map.hpp>
#include <execinfo.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define __USE_GNU
#include <ucontext.h>

namespace po = boost::program_options;
using boost::program_options::value;

#include "TesseractRunner.h"

#if DISPLAY
#include "Display.h"

void resizeHandler(int sig);

Display* display;
#endif

std::mutex g_console_mutex;

#define VERSION "1.2.4"

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

void segfault_action(int sig, siginfo_t *info, void *secret)
{
	/*void *array[10];
	auto size = backtrace(array, 10);

	fprintf(stderr, "Error: signal %d:\n", signal);
	backtrace_symbols_fd(array, size, STDERR_FILENO);

	exit(0);*/  
	void *trace[16];
	char **messages = (char **)NULL;
	int i, trace_size = 0;
	ucontext_t *uc = (ucontext_t *)secret;

	auto file = fopen("TesseractAutomatorLog.log", "w");
	/* Do something useful with siginfo_t */
	if (sig == SIGSEGV)
	fprintf(file, "Got signal %d, faulty address is %p, "
		"from %p\n", sig, info->si_addr,
		uc->uc_mcontext.gregs[REG_RIP]);
	else
	fprintf(file, "Got signal %d\n", sig);

	trace_size = backtrace(trace, 16);
	/* overwrite sigaction with caller's address */
	trace[1] = (void *)uc->uc_mcontext.gregs[REG_RIP];

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	fprintf(file, "[bt] Execution path:\n");
	for (i = 1; i<trace_size; ++i)
	{
		fprintf(file, "[bt] %s\n", messages[i]);

		/* find first occurence of '(' or ' ' in message[i] and assume
		* everything before that is the file name. (Don't go beyond 0 though
		* (string terminator)*/
		size_t p = 0;
		while (messages[i][p] != '(' && messages[i][p] != ' '
			&& messages[i][p] != 0)
			++p;

		char syscom[512];
		sprintf(syscom, "addr2line %p -e %.*s", trace[i], p, messages[i]);
		//last parameter is the filename of the symbol
		FILE* ptr;
		char buf[BUFSIZ];

		fprintf(file, "%s\n", syscom);
		if ((ptr = popen(syscom, "r")) != NULL) {
			while (fgets(buf, BUFSIZ, ptr) != NULL)
			{

				fprintf(file, "%s", buf);
			}
			pclose(ptr);
		}
	}

	delete display;
	exit(0);
}

int main(int argc, char* argv[])
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = segfault_action;
	sa.sa_flags = SA_SIGINFO | SA_RESTART;

	sigaction(SIGSEGV, &sa, nullptr);

	// oblige le buffer desortie a etre thread safe
	std::ios_base::sync_with_stdio(true);
	int nb_process = 2;

	boost::program_options::positional_options_description pd;
	pd.add("input", -1);

	po::options_description desc;
	desc.add_options()
		("psm", value<int>()->default_value(3)->value_name("NUM"), "Page Segmentation Mode")
		("oem", value<int>()->default_value(3)->value_name("NUM"), "Ocr Engine Mode")
		("lang,l", value <std::string>()->default_value("fra")->value_name("LANG"), "Langue utilise pour l'OCR")
		("help,h", "")
		("parallel,p", value<int>()->value_name("NUM"), "Nombre de threads en parrallele")
		("output,o", value<std::string>()->value_name("DOSSIER")->default_value(boost::filesystem::current_path().string()), "Dossier de sortie (defaut: dossier actuel)")
		("continue,c", "Ne pas ecraser les fichiers existant")
		("silent,s", "Ne pas afficher l'interface")
		("exif,e", value<std::string>()->value_name("DOSSIER")->default_value("")->implicit_value(""), "Copier l'image dans le fichier de sortie et écrire le resulat dans les Exif")
		("text,t", value<std::string>()->value_name("DOSSIER")->default_value("")->implicit_value(""), "Ecrire le resultat dans un fichier texte (.txt) dans le dossier de sortie")
		("prefixe,f", "Ajout le chemin relatif a [input] en prefixe du fichier")
		("version,v", "Affiche le numero de version de l'application")
		("input,i", value<std::string>()->value_name("DOSSIER"), "");


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

	if (!vm.count("input"))
	{
		std::cout << "\nVeuiller indiquer un dossier a traiter\n\n";

		std::cout << "Tapez :\n";
		std::cout << "  " << argv[0] << " -h\n";
		std::cout << "Pour afficher l'aide\n\n";
		return 0;
	}


	if (!fs::is_directory(vm["input"].as<std::string>()))
	{
		std::cout << "Le chemin " << vm["input"].as<std::string>() << " n'est pas un dossier valide\n";
		return 0;
	}

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

#if DISPLAY
	std::thread* th = nullptr;
	if (!vm.count("silent"))
	{
		th = new std::thread([&]()
		{
			display = new Display(tessR);

			signal(SIGWINCH, resizeHandler);
			display->Run();

			delete display;
		});
	}
#endif

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
