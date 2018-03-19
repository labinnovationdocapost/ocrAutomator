#include "Ocr/A2IA/A2IA.h"
#include "Ocr/tesseract/Tesseract.h"
#include <iostream>
#include <sstream>
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include <boost/format.hpp>


Docapost::IA::Tesseract::A2IA::~A2IA()
{
	A2iARC_CloseChannel(mChannelId);
}

std::unique_ptr<std::vector<std::string>> Docapost::IA::Tesseract::A2IA::ProcessThroughOcr(MemoryFileBuffer* imgData)
{
	uint32_t documentId;
	uint32_t requestId;
	if(A2iARC_GetDefaultDocument(&mDocumentTable, &documentId) != A2iARC_Ok)
	{
		throw std::runtime_error((boost::format("A2iARC_GetDefaultDocument: %s") % A2iARC_GetLastError()).str());
	}
	A2iARC_Input * input = A2iARC_GetInputDocumentAdd(documentId);
	if(A2iARC_DefineImageJpegMemory(input, imgData->data(), imgData->len()) != A2iARC_Ok)
	{
		throw std::runtime_error((boost::format("A2iARC_DefineImageJpegMemory: %s") % A2iARC_GetLastError()).str());
	}
	if(A2iARC_OpenRequest(mChannelId, &requestId, input) != A2iARC_Ok)
	{
		throw std::runtime_error((boost::format("A2iARC_OpenRequest: %s") % A2iARC_GetLastError()).str());
	}

	A2iARC_Output result;
	if(auto ret = A2iARC_GetResult(mChannelId, &requestId, &result, 100000) != A2iARC_Ok)
	{
		if(ret != A2iARC_Err_NoAnsw)
		{
			throw std::runtime_error((boost::format("A2iARC_GetResult: %s") % A2iARC_GetLastError()).str());
		}
		return std::make_unique<std::vector<std::string>>(2);
	}

	rapidjson::StringBuffer s;
	rapidjson::Writer<rapidjson::StringBuffer> writer(s);
	CreateJSON(writer, result.documentTypeInfo.CaseSingleField.field.fieldTypeInfo.CaseLooselyStructured.results.transcriptionResults);

	std::stringstream stream;
	for(int i =0; i < result.documentTypeInfo.CaseSingleField.field.fieldTypeInfo.CaseLooselyStructured.results.transcriptionResults.topLevelResults.cTab; i++)
	{
		stream << result.documentTypeInfo.CaseSingleField.field.fieldTypeInfo.CaseLooselyStructured.results.transcriptionResults.topLevelResults.pTab[i].reco << "\n";
	}
	A2iARC_CloseRequest(mChannelId, requestId);
	A2iARC_CloseDocument(documentId);
	auto vector = std::make_unique<std::vector<std::string>>();
	vector->push_back(stream.str());
	vector->push_back(s.GetString());
	return vector;
}
void Docapost::IA::Tesseract::A2IA::CreateJSON(rapidjson::Writer<rapidjson::StringBuffer>& writer, A2iARC_TranscriptionResults& result)
{
	writer.StartArray();
	for (int i = 0; i < result.topLevelResults.cTab; i++)
	{
		writer.StartObject();
		{
			writer.Key("Score");
			writer.Uint(result.topLevelResults.pTab[i].score);
			writer.Key("Text");
			writer.String(result.topLevelResults.pTab[i].reco);
			writer.Key("Location");
			writer.StartObject();
			{
				writer.Key("X1");
				writer.Double(result.topLevelResults.pTab[i].extractedImage.location.x_tl);
				writer.Key("X2");
				writer.Double(result.topLevelResults.pTab[i].extractedImage.location.x_br);
				writer.Key("Y1");
				writer.Double(result.topLevelResults.pTab[i].extractedImage.location.y_tl);
				writer.Key("Y2");
				writer.Double(result.topLevelResults.pTab[i].extractedImage.location.y_br);
			}
			writer.EndObject();
			CreateJSON(writer, result.topLevelResults.pTab[i]);
		}
		writer.EndObject();
	}
	writer.EndArray();
}
void Docapost::IA::Tesseract::A2IA::CreateJSON(rapidjson::Writer<rapidjson::StringBuffer>& writer, A2iARC_TranscriptionLevelResults& result)
{
	if (result.subLevelResults.cTab == 0)
		return;
	writer.Key("SubLevel");
	writer.StartArray();
	for (int i = 0; i < result.subLevelResults.cTab; i++)
	{
		writer.StartObject();
		{
			writer.Key("Score");
			writer.Uint(result.subLevelResults.pTab[i].score);
			writer.Key("Text");
			writer.String(result.subLevelResults.pTab[i].reco);
			writer.Key("Location");
			writer.StartObject();
			{
				writer.Key("X1");
				writer.Double(result.subLevelResults.pTab[i].extractedImage.location.x_tl);
				writer.Key("X2");
				writer.Double(result.subLevelResults.pTab[i].extractedImage.location.x_br);
				writer.Key("Y1");
				writer.Double(result.subLevelResults.pTab[i].extractedImage.location.y_tl);
				writer.Key("Y2");
				writer.Double(result.subLevelResults.pTab[i].extractedImage.location.y_br);
			}
			writer.EndObject();
			CreateJSON(writer, result.subLevelResults.pTab[i]);
		}
		writer.EndObject();
	}
	writer.EndArray();
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
