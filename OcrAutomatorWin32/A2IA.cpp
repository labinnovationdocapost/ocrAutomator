#include "A2IA.h"
#include "Tesseract.h"

Docapost::IA::Tesseract::A2IA::~A2IA()
{
	A2iARC_CloseChannel(mChannelId);
}

std::unique_ptr<std::string> Docapost::IA::Tesseract::A2IA::ProcessThroughOcr(MemoryFileBuffer* imgData)
{
	uint32_t documentId;
	uint32_t requestId;
	if(A2iARC_GetDefaultDocument(&mDocumentTable, &documentId) != A2iARC_Ok)
	{
		std::cout << "A2iARC_GetDefaultDocument: " << A2iARC_GetLastError() << "\n";
		return std::make_unique<string>();
	}
	A2iARC_Input * input = A2iARC_GetInputDocumentAdd(documentId);
	if(A2iARC_DefineImageJpegMemory(input, imgData->data(), imgData->len()) != A2iARC_Ok)
	{
		std::cout << "A2iARC_DefineImageJpegMemory: " << A2iARC_GetLastError() << "\n";
		return std::make_unique<string>();
	}
	if(A2iARC_OpenRequest(mChannelId, &requestId, input) != A2iARC_Ok)
	{
		std::cout << "A2iARC_OpenRequest: " << A2iARC_GetLastError() << "\n";
		return std::make_unique<string>();
	}

	A2iARC_Output result;
	if(A2iARC_GetResult(mChannelId, &requestId, &result, 100000) != A2iARC_Ok)
	{
		std::cout << "A2iARC_GetResult: " << A2iARC_GetLastError() << "\n";
		return std::make_unique<string>();
	}
	
	std::stringstream stream;
	for(int i =0; i < result.documentTypeInfo.CaseSingleField.field.fieldTypeInfo.CaseLooselyStructured.results.transcriptionResults.topLevelResults.cTab; i++)
	{
		stream << result.documentTypeInfo.CaseSingleField.field.fieldTypeInfo.CaseLooselyStructured.results.transcriptionResults.topLevelResults.pTab[i].reco << "\n";
	}
	A2iARC_CloseRequest(mChannelId, requestId);
	A2iARC_CloseDocument(documentId);

	return std::make_unique<string>(stream.str());
}

void Docapost::IA::Tesseract::A2IA::InitEngine()
{
	if (A2iARC_OpenChannel(&mChannelId) != A2iARC_Ok)
	{
		std::cout << "A2iARC_OpenChannel: " << A2iARC_GetLastError() << "\n";
	}
	if (A2iARC_UploadDocumentTablePersistentData(&mDocumentTable, mChannelId) != A2iARC_Ok)
	{
		std::cout << "A2iARC_UploadDocumentTablePersistentData: " << A2iARC_GetLastError() << "\n";
	}
}
