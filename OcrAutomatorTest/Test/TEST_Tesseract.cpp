
#include <boost/test/unit_test.hpp>
#include "TEST_Global.h"


Docapost::IA::Tesseract::TesseractFactory* CreateTesseractFactory()
{
	auto factory = new Docapost::IA::Tesseract::TesseractFactory();
	factory->Lang("fra");
	factory->Oem(tesseract::OcrEngineMode::OEM_DEFAULT);
	factory->Psm(tesseract::PageSegMode::PSM_AUTO_OSD);
	return factory;
}

BOOST_AUTO_TEST_SUITE(TEST_Tesseract)
BOOST_AUTO_TEST_CASE(TEST_TesseractFactory)
{
	auto factory = new Docapost::IA::Tesseract::TesseractFactory();
	factory->Lang("tst");
	BOOST_CHECK(factory->Lang() == "tst");
	factory->Lang("fra");
	BOOST_CHECK(factory->Lang() == "fra");

	factory->Oem(tesseract::OcrEngineMode::OEM_LSTM_ONLY);
	BOOST_CHECK(factory->Oem() == tesseract::OcrEngineMode::OEM_LSTM_ONLY);
	factory->Oem(tesseract::OcrEngineMode::OEM_DEFAULT);
	BOOST_CHECK(factory->Oem() == tesseract::OcrEngineMode::OEM_DEFAULT);

	factory->Psm(tesseract::PageSegMode::PSM_RAW_LINE);
	BOOST_CHECK(factory->Psm() == tesseract::PageSegMode::PSM_RAW_LINE);
	factory->Psm(tesseract::PageSegMode::PSM_AUTO_OSD);
	BOOST_CHECK(factory->Psm() == tesseract::PageSegMode::PSM_AUTO_OSD);

	auto ocr = factory->CreateNew();
	BOOST_CHECK(ocr != nullptr);
}

BOOST_AUTO_TEST_CASE(TEST_TesseractMemory)
{
	OcrMemory(CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractMemoryPdf)
{
	OcrMemoryPdf(CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractCorruptPdf)
{
	OcrCorruptPdf(CreateTesseractFactory());
}


BOOST_AUTO_TEST_CASE(TEST_TesseractFile)
{
	OcrFile(CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractFilePdf)
{
	OcrFilePdf(CreateTesseractFactory());
}


BOOST_AUTO_TEST_CASE(TEST_TesseractFile_DifferentDir)
{
	OcrFile_DifferentDir(CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractFilePdf_DifferentDir)
{
	OcrFilePdf_DifferentDir(CreateTesseractFactory());
}


BOOST_AUTO_TEST_CASE(TEST_TesseractFile_DifferentDir_Flatern)
{
	OcrFile_DifferentDir_Flatern(CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractFilePdf_DifferentDir_Flatern)
{
	OcrFilePdf_DifferentDir_Flatern(CreateTesseractFactory());
}


BOOST_AUTO_TEST_CASE(TEST_TesseractFile_DifferentDir_MultiFile_SingleThread)
{
	OcrFile_DifferentDir_MultiFile_SingleThread(CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractFilePdf_DifferentDir_MultiFile_SingleThread)
{
	OcrFilePdf_DifferentDir_MultiFile_SingleThread(CreateTesseractFactory());
}

BOOST_AUTO_TEST_CASE(TEST_TesseractFile_DifferentDir_MultiFile_MultiThread)
{
	OcrFile_DifferentDir_MultiFile_MultiThread(CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractFilePdf_DifferentDir_MultiFile_MultiThread)
{
	OcrFilePdf_DifferentDir_MultiFile_MultiThread(CreateTesseractFactory());
}

BOOST_AUTO_TEST_CASE(TEST_TesseractFile_Utf8)
{
	OcrFile_Utf8(CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractFilePdf_Utf8)
{
	OcrFilePdf_Utf8(CreateTesseractFactory());
}

BOOST_AUTO_TEST_CASE(TEST_TesseractFile_Slave)
{
	OcrFile_Slave(CreateTesseractFactory(), CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractFilePdf_Slave)
{
	OcrFilePdf_Slave(CreateTesseractFactory(), CreateTesseractFactory());
}

BOOST_AUTO_TEST_CASE(TEST_TesseractFile_Slave_Inverse)
{
	OcrFile_Slave_Inverse(CreateTesseractFactory(), CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractFilePdf_Slave_Inverse)
{
	OcrFilePdf_Slave_Inverse(CreateTesseractFactory(), CreateTesseractFactory());
}


BOOST_AUTO_TEST_CASE(TEST_TesseractFile_Http_Get)
{
	OcrFile_Http_Get(CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractFile_Http_Post)
{
	OcrFile_Http_Post(CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_TesseractFilePdf_Http_Post)
{
	OcrFilePdf_Http_Post(CreateTesseractFactory());
}
BOOST_AUTO_TEST_CASE(TEST_Tesseract_Api)
{
	Api_Test("Tesseract");
}
BOOST_AUTO_TEST_SUITE_END()