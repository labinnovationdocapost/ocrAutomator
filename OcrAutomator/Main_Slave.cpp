#include "Main.h"
#include "DisplayManager.h"

Docapost::IA::Tesseract::SlaveProcessingWorker* workerS;
SlaveDisplay* sdisplay;

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
			SlaveDisplayUI();
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

	workerS->Wait();
#endif

	delete workerS;
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cleaning SlaveProcessingWorker instance";
}