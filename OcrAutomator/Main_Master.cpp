#include "Main.h"
#include <boost/algorithm/string/predicate.hpp>
#include "Base/ExtraOutput.h"
#include "Http/HttpServer.h"
#include "DisplayManager.h"
#include <boost/algorithm/string/join.hpp>

Docapost::IA::Tesseract::MasterProcessingWorker* workerM;
Display* display;
HttpServer* http;

void Master(int argc, char** argv, po::variables_map& vm)
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

	if (!vm.count("input") && !vm.count("http"))
	{
		std::cout << "\nVeuiller indiquer un dossier a traiter\n\n";

		std::cout << "Tapez :\n";
		std::cout << "  " << argv[0] << " -h\n";
		std::cout << "Pour afficher l'aide\n\n";
		return;
	}

	if (vm.count("input"))
	{
		if (!fs::is_directory(vm["input"].as<std::string>()))
		{
			std::cout << "Le chemin " << vm["input"].as<std::string>() << " n'est pas un dossier valide\n";
			return;
		}
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

	if (vm.count("http"))
	{
		types |= Docapost::IA::Tesseract::OutputFlags::MemoryImage;
		types |= Docapost::IA::Tesseract::OutputFlags::MemoryText;
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

	std::unique_ptr<Docapost::IA::Tesseract::OcrFactory> factory;
	if(vm.count("ocr"))
	{
		if(Docapost::IA::Tesseract::OcrFactory::OcrExist(vm["ocr"].as<std::string>()))
		{
			factory = std::unique_ptr<Docapost::IA::Tesseract::OcrFactory>(Docapost::IA::Tesseract::OcrFactory::CreateNew(vm["ocr"].as<std::string>()));
			auto props = factory->GetOcrParametersDefinition();
			for(auto& prop : props)
			{
				if(vm.count(prop->name))
				{
					factory->SetFactoryProperty(prop->name, vm[prop->name].as<std::string>());
				}
			}
		}
	}

	if(!factory)
		factory = std::unique_ptr<Docapost::IA::Tesseract::OcrFactory>(CreateOcrFactory(vm));

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
	workerM->ExternalXmp()["CommandLine"] = boost::algorithm::join(std::vector<string>(argv, argv + argc), " ");
	if (vm.count("prefixe"))
	{
		auto p = vm["prefixe"].as<std::string>();
		workerM->Separator(p);
	}

	try
	{
		http = new HttpServer(*workerM, L"0.0.0.0", 8888);
	}
	catch(std::exception& e)
	{
		
	}
	catch (...)
	{

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

	if (vm.count("input"))
	{
		workerM->AddFolder(vm["input"].as<std::string>(), resume);
	}

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

	delete http;

	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cleaning MasterProcessingWorker instance";

	auto processTime = workerM->EndTime() - workerM->StartTime();

	std::cout << "Total Files : " << workerM->Total() << "\n";
	std::cout << "Start       : " << workerM->StartTime() << "\n";
	std::cout << "End         : " << workerM->EndTime() << "\n";
	std::cout << "Duration    : " << processTime << "\n";

	delete workerM;
}
