#include "A2IAFactory.h"
#include "A2IA.h"

std::string Docapost::IA::Tesseract::A2IAFactory::Version()
{
	std::string buf(A2iARC_ProductVersion);
	buf.append(".");
	buf.append(A2iARC_ProductRelease);
	buf.append(".");
	buf.append(A2iARC_ProductBuild);
	return buf;
}

Docapost::IA::Tesseract::A2IAFactory::A2IAFactory()
{
	mExtension = { ".txt", ".json" };

	if (A2iARC_Identify("INSERT PASSWORD HERE") != A2iARC_Ok)
	{
		std::cout << "A2iARC_Identify: " << A2iARC_GetLastError() << "\n";
	}
	//A2iARC_LoadRecognitionEngine("C:\\Program Files\\A2iA\\Configuration\\CFTest", "test.tbl", 20000, 1, &mEngineId);
	if (A2iARC_Init("C:\\Program Files\\A2iA\\A2iA TextReader V5.0\\Parms\\TRSoft\\Parms") != A2iARC_Ok)
	{
		std::cout << "A2iARC_Init: " << A2iARC_GetLastError() << "\n";
	}
	if (A2iARC_OpenDocumentTable(&mDocumentTable, "C:\\Program Files\\A2iA\\Configuration\\CFTest\\Test.tbl") != A2iARC_Ok)
	{
		std::cout << "A2iARC_OpenDocumentTable: " << A2iARC_GetLastError() << "\n";
	}
}

Docapost::IA::Tesseract::A2IA* Docapost::IA::Tesseract::A2IAFactory::CreateNew()
{
	return new A2IA(mImageFormat, mDocumentTable);
}
