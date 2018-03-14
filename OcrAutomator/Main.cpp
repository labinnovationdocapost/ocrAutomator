#include "Main.h"
#include <boost/assign/list_of.hpp>
#include <rttr/registration.h>

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
		std::cout << "\n" << desc.first << ":\n";
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
	
copy parameter's format:
  - xN:FOLDER : N is a floating point number witch represent the new size of the image (>1 : greater size, <1 : lower size). FOLDER is the folder where the new image is write, if it's empty the default folder is use
	Example: x0.25 for an image 4 times less
  - Npx:FOLDER : N is the maximum width and height allow for the new image
	Example: 150px:/usr/example/myfolder/thumbnail/ for an image of a maximum of 150px width and weight (if the image is 300x200 pixel, the new image is 150x100 pixel)
)V0G0N";
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
	/*using namespace rttr;
	std::vector<type> classs_type = type::get_by_name("OcrFactory").get_derived_classes();
	for(type class_type: classs_type)
	{
		auto props = class_type.get_properties();
		auto ctor = class_type.get_constructor();
		auto obj = class_type.create();
		auto prop = class_type.get_property("PSM");
		prop.set_value(obj, tesseract::PageSegMode::PSM_SINGLE_CHAR);
		auto val = prop.get_value(obj);
		auto res = val;
	}*/
	/*if(class_type)
	{
		auto props = class_type.get_properties();
	}*/


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
		("exif,e", value<std::string>()->value_name("DOSSIER"), "Copier l'image dans le fichier de sortie et écrire le resulat dans les Exif. Si non sépcifié le paramètre --output est utilisé")
		("text,t", value<std::string>()->value_name("DOSSIER"), "Ecrire le resultat dans un fichier texte (.txt) dans le dossier de sortie. Si non sépcifié le paramètre --output est utilisé")
		("prefixe,f", value<std::string>()->value_name("SEPARATOR")->implicit_value("__"), "Ajout le chemin relatif a [input] en prefixe du fichier.Defaut: __")
		("input,i", value<std::string>()->value_name("DOSSIER"), "Dossier d'entree a partir de laquelle seront listee les fichiers a traiter")
		("image", value<std::string>()->value_name("FORMAT")->default_value("jpg"), "Format de l'image de sortie")
		("copy", value<std::vector<std::string>>()->value_name("FORMAT"), "Copie de l'image dans different scaling, le paramètre peut être specifié plusieur fois pour avoir plusieur sorties")
		("http", "Lance le server HTTP pour envoyer des images/PDF a l'application");

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
#ifdef __linux__
		freopen("/var/log/TesseractAutomatorStdErr_Slave.log", "w", stderr);
#endif
#ifdef _WIN32
		freopen("TesseractAutomatorStdErr_Master.log", "w", stderr);
#endif
		Slave(argv, vm);
	}
	else
	{
#ifdef __linux__
		freopen("/var/log/TesseractAutomatorStdErr_Master.log", "w", stderr);
#endif
#ifdef _WIN32
		freopen("TesseractAutomatorStdErr_Master.log", "w", stderr);
#endif
		Master(argv, vm);
}


	return 0;
}

