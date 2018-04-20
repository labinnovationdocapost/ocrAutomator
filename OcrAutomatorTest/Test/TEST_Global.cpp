//#include <BoostTestTargetConfig.h>
#include <boost/thread/thread.hpp>
#include "TEST_Global.h"
#include <sstream>
#include <codecvt>
#define BOOST_TEST_MODULE TesseractAutomator
#include <boost/test/unit_test.hpp>
#include <curl/curl.h>
#include <archive_entry.h>

//#include <boost/test/output_test_stream.hpp>
/*struct cout_redirect {
	cout_redirect( std::streambuf * new_buffer )
		: old( std::cout.rdbuf( new_buffer ) )
	{ }

	~cout_redirect( ) {
		std::cout.rdbuf( old );
	}

private:
	std::streambuf * old;
};*/

int Fixature::port = 14000;

TEST_OCRAUTOMATOR_SLAVE::TEST_OCRAUTOMATOR_SLAVE(Docapost::IA::Tesseract::TesseractFactory* factory, int port) : factory(factory)
{
	files.clear();
	BOOST_TEST_MESSAGE("Start : TEST_OCRAUTOMATOR_SLAVE");

	auto ocr = factory->CreateNew();
	BOOST_CHECK(ocr != nullptr);

	worker = new Docapost::IA::Tesseract::SlaveProcessingWorker(*factory, port);
	worker->onEndProcessFile.connect(boost::bind(file_received_slave, _1));
	worker->onStartProcessFile.connect(boost::bind(file_start_slave, _1));
}
TEST_OCRAUTOMATOR::TEST_OCRAUTOMATOR(Docapost::IA::Tesseract::TesseractFactory* factory, Docapost::IA::Tesseract::OutputFlags flags, int port) : factory(factory)
{
	files.clear();
	BOOST_TEST_MESSAGE("Start : TEST_OCRAUTOMATOR");

	auto ocr = factory->CreateNew();
	BOOST_CHECK(ocr != nullptr);

	worker = new Docapost::IA::Tesseract::MasterProcessingWorker(*factory, flags, port);
	worker->onEndProcessFile.connect(boost::bind(file_received, _1));
	worker->onStartProcessFile.connect(boost::bind(file_start, _1));
}

std::mutex mtx;
std::vector<MasterFileStatus*> files;
std::vector<SlaveFileStatus*> filesSlave;
bool isOutputSet = false;


#ifdef __linux__
auto output_test_path = fs::system_complete("./Test_Data_Output/");
#endif
#ifdef _WIN32
auto output_test_path = fs::system_complete("../Test_Data_Output/");
#endif
#ifdef __linux__
auto input_test_path = fs::system_complete("./Test_Data/");
#endif
#ifdef _WIN32
auto input_test_path = fs::system_complete("../Test_Data/");
#endif

extern const char* kPathSeparator;

void file_received(MasterFileStatus* file)
{
	BOOST_TEST_MESSAGE("End : " << file->name);
	std::lock_guard<std::mutex> lock(mtx);
	files.push_back(file);
}

void file_received_slave(SlaveFileStatus* file)
{
	BOOST_TEST_MESSAGE("End : " << file->uuid);
	std::lock_guard<std::mutex> lock(mtx);
	filesSlave.push_back(file);
}

void file_start(MasterFileStatus* file)
{
	auto txt = boost::filesystem::change_extension(file->name, ".txt");
	if (boost::filesystem::exists(txt))
	{
		boost::filesystem::remove(txt);
		BOOST_TEST_MESSAGE("Remove : " << txt);
	}
	if (isOutputSet)
	{
		if (boost::filesystem::exists(file->name))
		{
			boost::filesystem::remove(file->name);
			BOOST_TEST_MESSAGE("Remove : " << file->name);
		}
	}
	BOOST_TEST_MESSAGE("Start : " << file->name);
}

void file_start_slave(SlaveFileStatus* file)
{
	BOOST_TEST_MESSAGE("Start : " << file->uuid);
}

void OcrMemory(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::MemoryText | Docapost::IA::Tesseract::OutputFlags::MemoryImage);

	auto file = input_test_path / folder / "/TestOcr";
	test_worker->worker->AddFolder(file);

	test_worker->RUN();

	auto ffile = static_cast<MasterLocalFileStatus*>(files[0]);
	BOOST_CHECK(ffile->relative_name == "Image1.jpg");
	auto txt = boost::filesystem::change_extension(ffile->new_name, ".txt");
	BOOST_CHECK(!boost::filesystem::exists(txt));
	BOOST_CHECK(boost::filesystem::exists(ffile->new_name));
}
void OcrMemoryPdf(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::MemoryText | Docapost::IA::Tesseract::OutputFlags::MemoryImage);

	auto file = input_test_path / folder / "TestOcr";
	test_worker->worker->AddFolder(file);

	test_worker->RUN();

	auto ffile = static_cast<MasterLocalFileStatus*>(files[0]);
	auto ffile2 = static_cast<MasterLocalFileStatus*>(files[1]);
	BOOST_CHECK(files.size() == 2);
	BOOST_CHECK(ffile->relative_name == "Pdf1.pdf");
	BOOST_CHECK(ffile2->relative_name == "Pdf1.pdf");
	auto p_txtall = boost::filesystem::change_extension(ffile->new_name, "").string();
	p_txtall.pop_back();
	p_txtall.pop_back();
	auto txt = boost::filesystem::change_extension(ffile->new_name, ".txt");
	auto txt2 = boost::filesystem::change_extension(ffile2->new_name, ".txt");
	auto json = boost::filesystem::change_extension(ffile->new_name, ".json");
	auto json2 = boost::filesystem::change_extension(ffile2->new_name, ".json");
	BOOST_CHECK(!boost::filesystem::exists(txt));
	BOOST_CHECK(!boost::filesystem::exists(txt2));
	BOOST_CHECK(!boost::filesystem::exists(json));
	BOOST_CHECK(!boost::filesystem::exists(json2));
	BOOST_CHECK(!boost::filesystem::exists(p_txtall + ".txt"));

	BOOST_CHECK(!boost::filesystem::exists(ffile->new_name));
	BOOST_CHECK(!boost::filesystem::exists(ffile2->new_name));

	boost::filesystem::remove(txt);
	boost::filesystem::remove(txt2);
	boost::filesystem::remove(json);
	boost::filesystem::remove(json2);
	boost::filesystem::remove(p_txtall + ".txt");

	boost::filesystem::remove(ffile->new_name);
	boost::filesystem::remove(ffile2->new_name);
}
void OcrCorruptPdf(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif);

	auto file = input_test_path / folder / "Corrupt";
	auto output = output_test_path / "Corrupt";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_worker->RUN();

	auto ffile = static_cast<MasterLocalFileStatus*>(files[0]);
	auto ffile2 = static_cast<MasterLocalFileStatus*>(files[1]);
	BOOST_CHECK(files.size() == 2);
	BOOST_CHECK(ffile->relative_name == "Pdf1.pdf");
	BOOST_CHECK(ffile2->relative_name == "Pdf1.pdf");
	auto p_txtall = boost::filesystem::change_extension(ffile->new_name, "").string();
	p_txtall.pop_back();
	p_txtall.pop_back();
	auto txt = boost::filesystem::change_extension(ffile->new_name, ".txt");
	auto txt2 = boost::filesystem::change_extension(ffile2->new_name, ".txt");
	BOOST_CHECK(!boost::filesystem::exists(txt));
	BOOST_CHECK(!boost::filesystem::exists(txt2));
	BOOST_CHECK(!boost::filesystem::exists(p_txtall + ".txt"));

	BOOST_CHECK(!boost::filesystem::exists(ffile->new_name));
	BOOST_CHECK(!boost::filesystem::exists(ffile2->new_name));

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}

void OcrFile(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif);

	auto file = input_test_path / folder / "/TestOcr";
	test_worker->worker->AddFolder(file);

	test_worker->RUN();

	auto ffile = static_cast<MasterLocalFileStatus*>(files[0]);
	BOOST_CHECK(ffile->relative_name == "Image1.jpg");
	auto txt = boost::filesystem::change_extension(ffile->new_name, ".txt");
	BOOST_CHECK(boost::filesystem::exists(txt));
	auto json = boost::filesystem::change_extension(ffile->new_name, ".json");
	BOOST_CHECK(boost::filesystem::exists(json));
	boost::filesystem::remove(txt);
	boost::filesystem::remove(json);
}
void OcrFilePdf(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif);

	auto file = input_test_path / folder / "/TestOcr";
	test_worker->worker->AddFolder(file);

	test_worker->RUN();

	auto ffile = static_cast<MasterLocalFileStatus*>(files[0]);
	auto ffile2 = static_cast<MasterLocalFileStatus*>(files[1]);
	BOOST_CHECK(files.size() == 2);
	BOOST_CHECK(ffile->relative_name == "Pdf1.pdf");
	BOOST_CHECK(ffile2->relative_name == "Pdf1.pdf");
	auto p_txtall = boost::filesystem::change_extension(ffile->new_name, "").string();
	p_txtall.pop_back();
	p_txtall.pop_back();
	auto txt = boost::filesystem::change_extension(ffile->new_name, ".txt");
	auto txt2 = boost::filesystem::change_extension(ffile2->new_name, ".txt");
	auto json = boost::filesystem::change_extension(ffile->new_name, ".json");
	auto json2 = boost::filesystem::change_extension(ffile2->new_name, ".json");
	BOOST_CHECK(boost::filesystem::exists(txt));
	BOOST_CHECK(boost::filesystem::exists(txt2));
	BOOST_CHECK(boost::filesystem::exists(json));
	BOOST_CHECK(boost::filesystem::exists(json2));
	BOOST_CHECK(boost::filesystem::exists(p_txtall + ".txt"));

	BOOST_CHECK(boost::filesystem::exists(ffile->new_name));
	BOOST_CHECK(boost::filesystem::exists(ffile2->new_name));

	boost::filesystem::remove(txt);
	boost::filesystem::remove(txt2);
	boost::filesystem::remove(json);
	boost::filesystem::remove(json2);
	boost::filesystem::remove(p_txtall + ".txt");

	boost::filesystem::remove(ffile->new_name);
	boost::filesystem::remove(ffile2->new_name);
}

void OcrFile_DifferentDir(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif);

	auto file = input_test_path / folder / "/TestOcr";
	auto output = output_test_path / "TestOcr";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output }, { Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_worker->RUN();

	auto ffile = static_cast<MasterLocalFileStatus*>(files[0]);
	BOOST_CHECK(ffile->relative_name == "Image1.jpg");

	auto relative_path = fs::relative(ffile->new_name, file);

	auto new_path = fs::absolute(relative_path, output);
	auto txt = boost::filesystem::change_extension(new_path, ".txt");
	auto json = boost::filesystem::change_extension(new_path, ".json");
	BOOST_CHECK(boost::filesystem::exists(txt));
	BOOST_CHECK(boost::filesystem::exists(json));
	BOOST_CHECK(boost::filesystem::exists(ffile->new_name));

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}
void OcrFilePdf_DifferentDir(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif);

	auto file = input_test_path / folder / "/TestOcr";
	auto output = output_test_path / "TestOcr";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_worker->RUN();

	auto ffile = static_cast<MasterLocalFileStatus*>(files[0]);
	auto ffile2 = static_cast<MasterLocalFileStatus*>(files[1]);
	BOOST_CHECK(files.size() == 2);
	BOOST_CHECK(ffile->relative_name == "Pdf1.pdf");
	BOOST_CHECK(ffile2->relative_name == "Pdf1.pdf");

	auto relative_path = fs::relative(ffile->new_name, file);
	auto relative_path2 = fs::relative(ffile2->new_name, file);

	auto new_path = fs::absolute(relative_path, output);
	auto new_path2 = fs::absolute(relative_path2, output);

	auto p_txtall = boost::filesystem::change_extension(new_path, "").string();
	p_txtall.pop_back();
	p_txtall.pop_back();
	
	auto txt = boost::filesystem::change_extension(new_path, ".txt");
	auto txt2 = boost::filesystem::change_extension(new_path2, ".txt");
	auto json = boost::filesystem::change_extension(new_path, ".json");
	auto json2 = boost::filesystem::change_extension(new_path2, ".json");
	BOOST_CHECK(boost::filesystem::exists(txt));
	BOOST_CHECK(boost::filesystem::exists(txt2));
	BOOST_CHECK(boost::filesystem::exists(json));
	BOOST_CHECK(boost::filesystem::exists(json2));
	BOOST_CHECK(boost::filesystem::exists(p_txtall + ".txt"));

	BOOST_CHECK(boost::filesystem::exists(new_path));
	BOOST_CHECK(boost::filesystem::exists(new_path2));

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}

void OcrFile_DifferentDir_Flatern(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif | Docapost::IA::Tesseract::OutputFlags::Flattern);

	auto file = input_test_path / folder / "/TestFlatern";
	auto output = output_test_path / "TestFlatern";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_worker->RUN();

	auto ffile = static_cast<MasterLocalFileStatus*>(files[0]);
	BOOST_CHECK(ffile->relative_name == std::string("TestFolder") + kPathSeparator + "Image1.jpg");

	auto relative_path = fs::relative(ffile->new_name, file);

	auto new_path = fs::absolute(relative_path, output);
	new_path = new_path.parent_path() / boost::replace_all_copy(relative_path.string(), kPathSeparator, "__");
	auto txt = boost::filesystem::change_extension(new_path, ".txt");
	BOOST_CHECK(boost::filesystem::exists(txt));
	BOOST_CHECK(boost::filesystem::exists(ffile->new_name));
	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}

void OcrFilePdf_DifferentDir_Flatern(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif | Docapost::IA::Tesseract::OutputFlags::Flattern);

	auto file = input_test_path / folder / "/TestFlatern";
	auto output = output_test_path / "TestFlatern";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_worker->RUN();


	auto ffile = static_cast<MasterLocalFileStatus*>(files[0]);
	auto ffile2 = static_cast<MasterLocalFileStatus*>(files[1]);
	BOOST_CHECK(files.size() == 2);
	BOOST_CHECK(ffile->relative_name == std::string("TestFolder") + kPathSeparator + "Pdf1.pdf");
	BOOST_CHECK(ffile2->relative_name == std::string("TestFolder") + kPathSeparator + "Pdf1.pdf");

	auto relative_path = fs::relative(ffile->new_name, file);
	auto relative_path2 = fs::relative(ffile2->new_name, file);

	auto new_path = fs::absolute(relative_path, output);
	new_path = new_path.parent_path() / boost::replace_all_copy(relative_path.string(), kPathSeparator, "__");
	auto new_path2 = fs::absolute(relative_path2, output);
	new_path2 = new_path2.parent_path() / boost::replace_all_copy(relative_path2.string(), kPathSeparator, "__");

	auto p_txtall = boost::filesystem::change_extension(new_path, "").string();
	p_txtall.pop_back();
	p_txtall.pop_back();

	auto txt = boost::filesystem::change_extension(new_path, ".txt");
	auto txt2 = boost::filesystem::change_extension(new_path2, ".txt");
	BOOST_CHECK(boost::filesystem::exists(txt));
	BOOST_CHECK(boost::filesystem::exists(txt2));
	BOOST_CHECK(boost::filesystem::exists(p_txtall + ".txt"));

	BOOST_CHECK(boost::filesystem::exists(new_path));
	BOOST_CHECK(boost::filesystem::exists(new_path2));

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}

void OcrFile_DifferentDir_MultiFile_SingleThread(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif);

	auto file = input_test_path / folder / "/TestMulti";
	auto output = output_test_path / "TestMulti";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_worker->RUN();

	for (auto& fi : files)
	{
		auto ffile = static_cast<MasterLocalFileStatus*>(fi);

		auto relative_path = fs::relative(ffile->new_name, file);

		auto new_path = fs::absolute(relative_path, output);
		auto txt = boost::filesystem::change_extension(new_path, ".txt");
		BOOST_CHECK(boost::filesystem::exists(txt));
		BOOST_CHECK(boost::filesystem::exists(ffile->new_name));
	}
	BOOST_CHECK(files.size() == 8);

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}

void OcrFilePdf_DifferentDir_MultiFile_SingleThread(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif);

	auto file = input_test_path / folder / "/TestMulti";
	auto output = output_test_path / "TestMulti";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_worker->RUN();

	BOOST_CHECK(files.size() == 16);
	for (auto& fi : files)
	{
		auto ffile = static_cast<MasterLocalFileStatus*>(fi);

		auto relative_path = fs::relative(ffile->new_name, file);

		auto new_path = fs::absolute(relative_path, output);

		auto p_txtall = boost::filesystem::change_extension(new_path, "").string();
		p_txtall.pop_back();
		p_txtall.pop_back();

		auto txt = boost::filesystem::change_extension(new_path, ".txt");
		BOOST_CHECK(boost::filesystem::exists(txt));
		BOOST_CHECK(boost::filesystem::exists(p_txtall + ".txt"));

		BOOST_CHECK(boost::filesystem::exists(new_path));
	}

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}

void OcrFile_DifferentDir_MultiFile_MultiThread(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif);

	auto file = input_test_path / folder / "/TestMulti";
	auto output = output_test_path / "TestMulti";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_worker->RUN(4);

	for (auto& fi : files)
	{
		auto ffile = static_cast<MasterLocalFileStatus*>(fi);

		auto relative_path = fs::relative(ffile->new_name, file);

		auto new_path = fs::absolute(relative_path, output);
		auto txt = boost::filesystem::change_extension(new_path, ".txt");
		BOOST_CHECK(boost::filesystem::exists(txt));
		BOOST_CHECK(boost::filesystem::exists(ffile->new_name));
	}
	BOOST_CHECK(files.size() == 8);

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);

}

void OcrFilePdf_DifferentDir_MultiFile_MultiThread(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif);

	auto file = input_test_path / folder / "/TestMulti";
	auto output = output_test_path / "TestMulti";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_worker->RUN(4);


	BOOST_CHECK(files.size() == 16);
	for (auto& fi : files)
	{
		auto ffile = static_cast<MasterLocalFileStatus*>(fi);

		auto relative_path = fs::relative(ffile->new_name, file);

		auto new_path = fs::absolute(relative_path, output);

		auto p_txtall = boost::filesystem::change_extension(new_path, "").string();
		p_txtall.pop_back();
		p_txtall.pop_back();

		auto txt = boost::filesystem::change_extension(new_path, ".txt");
		BOOST_CHECK(boost::filesystem::exists(txt));
		BOOST_CHECK(boost::filesystem::exists(p_txtall + ".txt"));

		BOOST_CHECK(boost::filesystem::exists(new_path));
	}

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);

}

void OcrFile_Utf8(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif);

	auto file = input_test_path / folder / "/TestUtf8";
	auto output = output_test_path / "TestUtf8";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_worker->RUN();

	auto ffile = static_cast<MasterLocalFileStatus*>(files[0]);
	BOOST_CHECK(ffile->relative_name == "Image1.jpg");


	auto relative_path = fs::relative(ffile->new_name, file);

	auto new_path = fs::absolute(relative_path, output);
	auto txt = boost::filesystem::change_extension(new_path, ".txt");
	BOOST_CHECK(boost::filesystem::exists(txt));
	BOOST_CHECK(boost::filesystem::exists(ffile->new_name));


	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;
	BOOST_CHECK_NO_THROW(auto new_str = converter.from_bytes((*ffile->result)[0]));

	std::ifstream input((output / "Image1.txt").string());
	std::string codepage_str = static_cast<std::stringstream const&>(std::stringstream() << input.rdbuf()).str();
	BOOST_CHECK_NO_THROW(auto new_str = converter.from_bytes(codepage_str));
	input.close();


	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}
void OcrFilePdf_Utf8(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif);

	auto file = input_test_path / folder / "/TestUtf8";
	auto output = output_test_path / "TestUtf8";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_worker->RUN();

	auto ffile = static_cast<MasterLocalFileStatus*>(files[0]);
	auto ffile2 = static_cast<MasterLocalFileStatus*>(files[1]);
	BOOST_CHECK(ffile->relative_name == "Pdf1.pdf");
	BOOST_CHECK(ffile2->relative_name == "Pdf1.pdf");

	auto relative_path = fs::relative(ffile->new_name, file);
	auto relative_path2 = fs::relative(ffile2->new_name, file);

	auto new_path = fs::absolute(relative_path, output);
	auto new_path2 = fs::absolute(relative_path2, output);

	auto p_txtall = boost::filesystem::change_extension(new_path, "").string();
	p_txtall.pop_back();
	p_txtall.pop_back();

	auto txt = boost::filesystem::change_extension(new_path, ".txt");
	auto txt2 = boost::filesystem::change_extension(new_path2, ".txt");


	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;

	BOOST_CHECK_NO_THROW(auto new_str = converter.from_bytes((*ffile->result)[0]));
	BOOST_CHECK_NO_THROW(auto new_str2 = converter.from_bytes((*ffile2->result)[0]));

	std::ifstream input((output / "Pdf1.txt").string());
	std::string codepage_str = static_cast<std::stringstream const&>(std::stringstream() << input.rdbuf()).str();
	BOOST_CHECK_NO_THROW(auto new_str = converter.from_bytes(codepage_str));
	input.close();

	std::ifstream input2((output / "Pdf1.txt").string());
	std::string codepage_str2 = static_cast<std::stringstream const&>(std::stringstream() << input2.rdbuf()).str();
	BOOST_CHECK_NO_THROW(auto new_str2 = converter.from_bytes(codepage_str2));
	input2.close();


	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}

void OcrFile_Slave(Docapost::IA::Tesseract::TesseractFactory* factory, Docapost::IA::Tesseract::TesseractFactory* factory2, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	int port = f.Port();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif, port);
	while ((port = test_worker->worker->Port()) == 0) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
	auto test_slave = std::make_unique<TEST_OCRAUTOMATOR_SLAVE>(factory2, port);

	auto file = input_test_path / folder / "/TestMulti";
	auto output = output_test_path / "TestSlave";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_slave->RUN(5);
	test_worker->RUN();
	test_slave.reset();
	test_worker.reset();


	for (auto& fi : files)
	{
		auto ffile = static_cast<MasterLocalFileStatus*>(fi);

		auto relative_path = fs::relative(ffile->new_name, file);

		auto new_path = fs::absolute(relative_path, output);
		auto txt = boost::filesystem::change_extension(new_path, ".txt");
		BOOST_CHECK(boost::filesystem::exists(txt));
		BOOST_CHECK(boost::filesystem::exists(ffile->new_name));
	}

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}
void OcrFilePdf_Slave(Docapost::IA::Tesseract::TesseractFactory* factory, Docapost::IA::Tesseract::TesseractFactory* factory2, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	int port = f.Port();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif, port);
	while ((port = test_worker->worker->Port()) == 0) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
	auto test_slave = std::make_unique<TEST_OCRAUTOMATOR_SLAVE>(factory2, port);

	auto file = input_test_path / folder / "/TestMulti";
	auto output = output_test_path / "TestSlave";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_slave->RUN(5);
	test_worker->RUN();
	test_slave.reset();
	test_worker.reset();


	for (auto& fi : files)
	{
		auto ffile = static_cast<MasterLocalFileStatus*>(fi);

		auto relative_path = fs::relative(ffile->new_name, file);

		auto new_path = fs::absolute(relative_path, output);

		auto p_txtall = boost::filesystem::change_extension(new_path, "").string();
		p_txtall.pop_back();
		p_txtall.pop_back();

		auto txt = boost::filesystem::change_extension(new_path, ".txt");
		BOOST_CHECK(boost::filesystem::exists(txt));
		BOOST_CHECK(boost::filesystem::exists(p_txtall + ".txt"));

		BOOST_CHECK(boost::filesystem::exists(new_path));
	}

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}

void OcrFile_Slave_Inverse(Docapost::IA::Tesseract::TesseractFactory* factory, Docapost::IA::Tesseract::TesseractFactory* factory2, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	int port = f.Port();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif, port);
	while ((port = test_worker->worker->Port()) == 0) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
	auto test_slave = std::make_unique<TEST_OCRAUTOMATOR_SLAVE>(factory2, port);

	auto file = input_test_path / folder / "/TestMulti";
	auto output = output_test_path / "TestSlave";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_slave->RUN(5);
	test_worker->RUN();
	test_worker.reset();
	test_slave.reset();


	for (auto& fi : files)
	{
		auto ffile = static_cast<MasterLocalFileStatus*>(fi);

		auto relative_path = fs::relative(ffile->new_name, file);

		auto new_path = fs::absolute(relative_path, output);
		auto txt = boost::filesystem::change_extension(new_path, ".txt");
		BOOST_CHECK(boost::filesystem::exists(txt));
		BOOST_CHECK(boost::filesystem::exists(ffile->new_name));
	}

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}

void OcrFilePdf_Slave_Inverse(Docapost::IA::Tesseract::TesseractFactory* factory, Docapost::IA::Tesseract::TesseractFactory* factory2, std::string folder)
{
	Fixature f; // Setup et deleter
	files.clear();
	int port = f.Port();
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif, port);
	while ((port = test_worker->worker->Port()) == 0) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
	auto test_slave = std::make_unique<TEST_OCRAUTOMATOR_SLAVE>(factory2, port);

	auto file = input_test_path / folder / "/TestMulti";
	auto output = output_test_path / "TestSlave";
	test_worker->worker->AddFolder(file);
	test_worker->worker->SetOutput({ { Docapost::IA::Tesseract::OutputFlags::Exif, output },{ Docapost::IA::Tesseract::OutputFlags::Text, output } });

	test_slave->RUN(5);
	test_worker->RUN();
	test_worker.reset();
	test_slave.reset();


	for (auto& fi : files)
	{
		auto ffile = static_cast<MasterLocalFileStatus*>(fi);

		auto relative_path = fs::relative(ffile->new_name, file);

		auto new_path = fs::absolute(relative_path, output);

		auto p_txtall = boost::filesystem::change_extension(new_path, "").string();
		p_txtall.pop_back();
		p_txtall.pop_back();

		auto txt = boost::filesystem::change_extension(new_path, ".txt");
		BOOST_CHECK(boost::filesystem::exists(txt));
		BOOST_CHECK(boost::filesystem::exists(p_txtall + ".txt"));

		BOOST_CHECK(boost::filesystem::exists(new_path));
	}

	if (boost::filesystem::exists(output))
		boost::filesystem::remove_all(output);
}

BOOST_AUTO_TEST_CASE(TEST_Global)
{
	auto output_t = output_test_path / "Global" / "Text";
	auto output_e = output_test_path / "Global" / "Exif";
	auto file = input_test_path / "Pdf" / "TestMulti";
	auto s_file = file.string();
	auto s_output_t = output_t.string();
	auto s_output_e = output_e.string();
	std::vector<char*> arg = { "Test", "-i", const_cast<char*>(s_file.c_str()), "-e",const_cast<char*>(s_output_e.c_str()), "-t",const_cast<char*>(s_output_t.c_str()),  "-s" };

	_main(arg.size(), &arg[0]);

	int file_nb = std::count_if(
		fs::directory_iterator(output_t),
		fs::directory_iterator(),
		static_cast<bool(*)(const fs::path&)>(fs::is_regular_file));

	int folder_nb = std::count_if(
		fs::directory_iterator(output_t),
		fs::directory_iterator(),
		static_cast<bool(*)(const fs::path&)>(fs::is_directory));


	int file_folder1_nb = std::count_if(
		fs::directory_iterator(output_t / "TestFolder"),
		fs::directory_iterator(),
		static_cast<bool(*)(const fs::path&)>(fs::is_regular_file));

	int file_folder2_nb = std::count_if(
		fs::directory_iterator(output_t / "TestFolder2"),
		fs::directory_iterator(),
		static_cast<bool(*)(const fs::path&)>(fs::is_regular_file));


	BOOST_CHECK(file_nb == 20);
	BOOST_CHECK(file_folder1_nb == 10);
	BOOST_CHECK(file_folder2_nb == 10);
	BOOST_CHECK(folder_nb == 2);

	if (boost::filesystem::exists(output_test_path / "Global"))
		boost::filesystem::remove_all(output_test_path / "Global");
}

size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s)
{
	size_t newLength = size * nmemb;
	size_t oldLength = s->size();
	try
	{
		s->resize(oldLength + newLength);
	}
	catch (std::bad_alloc &e)
	{
		//handle memory problem
		return 0;
	}

	std::copy((char*)contents, (char*)contents + newLength, s->begin() + oldLength);
	return size * nmemb;
}
void OcrFile_Http_Get(Docapost::IA::Tesseract::TesseractFactory* factory)
{
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif | Docapost::IA::Tesseract::OutputFlags::MemoryImage | Docapost::IA::Tesseract::OutputFlags::MemoryText);
	http = new HttpServer(*test_worker->worker, L"0.0.0.0", 8888);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	test_worker->worker->Run(1);

	CURL *curl = nullptr;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	
	std::string s;
	curl_easy_setopt(curl, CURLOPT_URL, "localhost:8888");
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	auto res = curl_easy_perform(curl);
	BOOST_CHECK(res == CURLE_OK);
	BOOST_TEST_MESSAGE(s);
	delete http;
}

void OcrFile_Http_Post(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif | Docapost::IA::Tesseract::OutputFlags::MemoryImage | Docapost::IA::Tesseract::OutputFlags::MemoryText);
	http = new HttpServer(*test_worker->worker, L"0.0.0.0", 8888);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	test_worker->worker->Run(1);

	CURL *curl = nullptr;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	struct curl_httppost* post = NULL;
	struct curl_httppost* last = NULL;

	curl_formadd(&post, &last, CURLFORM_COPYNAME, "image",
		CURLFORM_FILE, (input_test_path / folder / "TestOcr" / "Image1.jpg").string().c_str(),
		CURLFORM_CONTENTTYPE, "image/jpeg", CURLFORM_END);

	std::string s;
	curl_easy_setopt(curl, CURLOPT_URL, "localhost:8888");
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

	auto res = curl_easy_perform(curl);
	BOOST_CHECK(res == CURLE_OK);

	struct archive_entry *entry;
	int r = 0;

	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	archive_read_open_memory(a, (void*)s.data(), s.length());
	BOOST_CHECK(r == ARCHIVE_OK);


	std::set<std::string> result = { "Image1/Image1.jpg", "Image1/Image1.txt", "Image1/Image1.json" };
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		BOOST_TEST_MESSAGE(archive_entry_pathname(entry));
		BOOST_CHECK(result.count(archive_entry_pathname(entry)));
		result.erase(archive_entry_pathname(entry));
	}

	archive_read_close(a);
	delete http;
}
void OcrFilePdf_Http_Post(Docapost::IA::Tesseract::TesseractFactory* factory, std::string folder)
{
	auto test_worker = std::make_unique<TEST_OCRAUTOMATOR>(factory, Docapost::IA::Tesseract::OutputFlags::Text | Docapost::IA::Tesseract::OutputFlags::Exif | Docapost::IA::Tesseract::OutputFlags::MemoryImage | Docapost::IA::Tesseract::OutputFlags::MemoryText);
	http = new HttpServer(*test_worker->worker, L"0.0.0.0", 8888);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	test_worker->worker->Run(1);

	CURL *curl = nullptr;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	struct curl_httppost* post = NULL;
	struct curl_httppost* last = NULL;

	curl_formadd(&post, &last, CURLFORM_COPYNAME, "image",
		CURLFORM_FILE, (input_test_path / folder / "TestOcr" / "Pdf1.pdf").string().c_str(),
		CURLFORM_CONTENTTYPE, "application/pdf", CURLFORM_END);

	std::string s;
	curl_easy_setopt(curl, CURLOPT_URL, "localhost:8888");
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

	auto res = curl_easy_perform(curl);
	BOOST_CHECK(res == CURLE_OK);

	struct archive_entry *entry;
	int r = 0;

	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	archive_read_open_memory(a, (void*)s.data(), s.length());
	BOOST_CHECK(r == ARCHIVE_OK);

	std::set<std::string> result = { "Pdf1/Pdf1-0.jpg", "Pdf1/Pdf1-0.txt", "Pdf1/Pdf1-0.json", "Pdf1/Pdf1-1.jpg", "Pdf1/Pdf1-1.txt", "Pdf1/Pdf1-1.json" };
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		BOOST_TEST_MESSAGE(archive_entry_pathname(entry));
		BOOST_CHECK(result.count(archive_entry_pathname(entry)));
		result.erase(archive_entry_pathname(entry));
	}

	archive_read_close(a);
	delete http;
}

void Api_Test(std::string name)
{
	for(auto& ocr : Docapost::IA::Tesseract::OcrFactory::GetOcrs())
	{
		BOOST_TEST_MESSAGE(ocr);
	}
	BOOST_CHECK(Docapost::IA::Tesseract::OcrFactory::OcrExist("Tesseract"));

	auto params1 = Docapost::IA::Tesseract::OcrFactory::GetOcrParametersDefinition("Tesseract");
	auto ocr = Docapost::IA::Tesseract::OcrFactory::CreateNew("Tesseract");
	auto params2 = ocr->GetOcrParametersDefinition();

}