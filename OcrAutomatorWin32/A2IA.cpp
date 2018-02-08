#include "A2IA.h"
#include "Tesseract.h"

std::unique_ptr<std::string> Docapost::IA::Tesseract::A2IA::ProcessThroughOcr(MemoryFileBuffer* imgData)
{
	uint32_t documentId;
	uint32_t requestId;
	if(A2iARC_GetDefaultDocument(&mDocumentTable, &documentId) != A2iARC_Ok)
	{
		std::cout << A2iARC_GetLastError() << "\n";
	}
	if(A2iARC_UploadDocumentTablePersistentData(&mDocumentTable, mChannelId) != A2iARC_Ok)
	{
		std::cout << A2iARC_GetLastError() << "\n";
	}
	A2iARC_Input * input = A2iARC_GetInputDocumentAdd(documentId);
	if(A2iARC_DefineImageJpegMemory(input, imgData->data(), imgData->len()) != A2iARC_Ok)
	{
		std::cout << A2iARC_GetLastError() << "\n";
	}
	if(A2iARC_OpenRequest(mChannelId, &requestId, input) != A2iARC_Ok)
	{
		std::cout << A2iARC_GetLastError() << "\n";
	}

	A2iARC_Output result;
	if(A2iARC_GetResult(mChannelId, &requestId, &result, 10000) != A2iARC_Ok)
	{
		std::cout << A2iARC_GetLastError() << "\n";
	}
	auto res = new std::string(result.documentTypeInfo.CaseSingleField.field.fieldTypeInfo.CaseLooselyStructured.results.transcriptionResults.topLevelResults.pTab[0].reco);

	auto text = std::unique_ptr<std::string>{ res };

	return text;
}

void Docapost::IA::Tesseract::A2IA::InitEngine()
{
	if(A2iARC_Identify("INSERT PASSWORD HERE") != A2iARC_Ok)
	{
		std::cout << A2iARC_GetLastError() << "\n";
	}
	//A2iARC_LoadRecognitionEngine("C:\\Program Files\\A2iA\\Configuration\\CFTest", "test.tbl", 20000, 1, &mEngineId);
	if(A2iARC_Init("C:\\Program Files\\A2iA\\A2iA TextReader V5.0\\Parms\\TRSoft\\Parms") != A2iARC_Ok)
	{
		std::cout << A2iARC_GetLastError() << "\n";
	}
	if(A2iARC_OpenChannel(&mChannelId) != A2iARC_Ok)
	{
		std::cout << A2iARC_GetLastError() << "\n";
	}
	if(A2iARC_OpenDocumentTable(&mDocumentTable, "C:\\Program Files\\A2iA\\Configuration\\CFTest\\Test.tbl") != A2iARC_Ok)
	{
		std::cout << A2iARC_GetLastError() << "\n";
	}
}
