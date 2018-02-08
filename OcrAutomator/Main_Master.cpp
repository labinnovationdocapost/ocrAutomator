#include "Main.h"
#include <boost/algorithm/string/predicate.hpp>
#include "ExtraOutput.h"

Docapost::IA::Tesseract::MasterProcessingWorker* workerM;


void Master(char** argv, po::variables_map& vm)
{
	/*boost::interprocess::named_mutex ip_process {boost::interprocess::open_or_create, "OCRAutomator_Mutex"};
	if(!ip_process.try_lock())
	{
	std::cout << "Application already running, two application cannot run on the same PC\n";
	return;
	}*/

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


	boost::unordered_map <boost::uuids::uuid, ExtraOutput> extraMap;
	boost::uuids::basic_random_generator<boost::mt19937> mGen = boost::uuids::basic_random_generator<boost::mt19937>();
	if (vm.count("copy"))
	{
		auto copy = vm["copy"].as<std::vector<std::string>>();
		for (auto& c : copy)
		{
			auto del = c.find_first_of(":");
			string first;
			string second;
			if (del == std::string::npos)
			{
				first = c;
			}
			else
			{
				first = c.substr(0, del);
				second = c.substr(del + 1);
			}

			auto id = mGen();

			if (boost::starts_with(first, "x"))
			{
				extraMap[id].Scale = ::atof(first.c_str() + 1);
				std::cout << "Scale: " << extraMap[id].Scale << "\n";
			}
			else if (boost::ends_with(first, "px"))
			{
				extraMap[id].Pixel = ::atoi(first.c_str());
				std::cout << "Pixel: " << extraMap[id].Pixel << "\n";
			}
			else
			{
				continue;
			}
			if (del != std::string::npos)
			{
				extraMap[id].Path = second;
				std::cout << extraMap[id].Path << "\n";
			}
		}
	}

	std::unique_ptr<Docapost::IA::Tesseract::OcrFactory> factory = std::unique_ptr<Docapost::IA::Tesseract::OcrFactory>(CreateOcrFactory(vm));

	if (vm.count("image"))
	{
		if (vm["image"].as<std::string>() == "png")
		{
			factory->ImageFormat(Docapost::IA::Tesseract::ImageFormatEnum::PNG);
		}
		else if (vm["image"].as<std::string>() == "jpg")
		{
			factory->ImageFormat(Docapost::IA::Tesseract::ImageFormatEnum::JPG);
		}
		else
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Option " << vm["image"].as<std::string>() << " unrecognized for field [image], fallback to jpg";
			factory->ImageFormat(Docapost::IA::Tesseract::ImageFormatEnum::JPG);
		}
	}

	workerM = new Docapost::IA::Tesseract::MasterProcessingWorker(*factory, types, vm["port"].as<int>());

	if (vm.count("prefixe"))
	{
		workerM->Separator(vm["prefixe"].as<std::string>());
	}


#if DISPLAY
	if (!vm.count("silent"))
	{
		th = new boost::thread([&]()
		{
			MasterDisplayUI();
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
	workerM->Wait();
#endif

	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cleaning MasterProcessingWorker instance";

	auto processTime = workerM->EndTime() - workerM->StartTime();

	std::cout << "Total Files : " << workerM->Total() << "\n";
	std::cout << "Start       : " << workerM->StartTime() << "\n";
	std::cout << "End         : " << workerM->EndTime() << "\n";
	std::cout << "Duration    : " << processTime << "\n";

	delete workerM;
}
