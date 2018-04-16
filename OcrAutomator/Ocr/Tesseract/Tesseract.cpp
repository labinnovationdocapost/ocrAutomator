#include "Ocr/Tesseract/Tesseract.h"
#include "Ocr/UninitializedOcrException.h"

#include <leptonica/allheaders.h>

#ifdef MAGICK
#include <Magick++.h>
#endif

#include "PDF/MuPDF.h"
#include "Base/Error.h"
#include <boost/format.hpp>
#include <rapidjson/writer.h>


Docapost::IA::Tesseract::Tesseract::Tesseract(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang, ImageFormatEnum format) : BaseOcrWithLoader(format), mPsm(psm), mOem(oem), mLang(lang), mTessBaseAPI(tesseract::TessBaseAPI())
{
	setMsgSeverity(L_SEVERITY_NONE);

	mTessBaseAPI.SetVariable("debug_file", "/dev/null");
	mTessBaseAPI.SetVariable("out", "quiet");

}


std::unique_ptr<std::vector<std::string>> Docapost::IA::Tesseract::Tesseract::ProcessThroughOcr(Docapost::IA::Tesseract::MemoryFileBuffer* imgData) {
	Pix *image = pixReadMem(imgData->data(), imgData->len());
	if (image == nullptr)
	{
		throw std::runtime_error("ProcessThroughOcr: Cannot open image");
	}

	mTessBaseAPI.SetImage(image);
	mTessBaseAPI.Recognize(nullptr);
	auto outtext = mTessBaseAPI.GetUTF8Text();
	auto outtext_s = string(outtext);
	auto ite = mTessBaseAPI.GetIterator();

	rapidjson::StringBuffer s;
	rapidjson::Writer<rapidjson::StringBuffer> writer(s);
	if (!outtext_s.empty())
	{
		std::deque<tesseract::PageIteratorLevel> stack = { tesseract::PageIteratorLevel::RIL_BLOCK, tesseract::PageIteratorLevel::RIL_TEXTLINE, tesseract::PageIteratorLevel::RIL_WORD };
		auto boxs = CreateTree(ite, stack);
		CreateJSON(writer, boxs);
	}
	else
	{
		writer.StartArray();
		writer.EndArray();
	}
	//auto text = std::unique_ptr<std::string>{ new string(outtext) };


	std::vector<std::string>* vector = new std::vector<std::string>();
	vector->push_back(outtext_s);
	vector->push_back(s.GetString());

	pixDestroy(&image);
	delete[] outtext;

	return std::unique_ptr<std::vector<std::string>>(vector);
}

void Docapost::IA::Tesseract::Tesseract::CreateJSON(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::list<std::shared_ptr<Docapost::IA::Tesseract::Tesseract::Box>>& boxs)
{
	writer.StartArray();
	for (auto& box : boxs)
	{
		writer.StartObject();
		{
			writer.Key("Score");
			writer.Double(box->score);
			writer.Key("Text");
			writer.String(box->text);
			writer.Key("Location");
			writer.StartObject();
			{
				writer.Key("X1");
				writer.Double(box->x1);
				writer.Key("X2");
				writer.Double(box->x2);
				writer.Key("Y1");
				writer.Double(box->y1);
				writer.Key("Y2");
				writer.Double(box->y2);
			}
			writer.EndObject();
			writer.Key("SubLevel");
			CreateJSON(writer, box->childs);
		}
		writer.EndObject();
	}
	writer.EndArray();
}

void Docapost::IA::Tesseract::Tesseract::CreateTree(tesseract::ResultIterator* result, std::deque<tesseract::PageIteratorLevel> stack, std::list<std::shared_ptr<Box>>& boxs)
{
	auto level = stack.front();
	stack.pop_front();
	result->Begin();
	std::list<std::shared_ptr<Box>> new_boxs;
	do
	{
		auto box = std::make_shared<Box>();
		box->score = result->Confidence(level);
		box->text = result->GetUTF8Text(level);
		result->BoundingBox(level, &box->x1, &box->y1, &box->x2, &box->y2);
		for (auto& parent_box : boxs)
		{
			if (parent_box->x1 <= box->x1 &&
				parent_box->x2 >= box->x2 &&
				parent_box->y1 <= box->y1 &&
				parent_box->y2 >= box->y2)
			{
				parent_box->childs.push_back(box);
				new_boxs.push_back(box);
				break;
			}
		}
	} while ((result->Next(level)));
	if (stack.size() > 0)
		CreateTree(result, stack, new_boxs);
}

std::list<std::shared_ptr<Docapost::IA::Tesseract::Tesseract::Box>> Docapost::IA::Tesseract::Tesseract::CreateTree(tesseract::ResultIterator* result, std::deque<tesseract::PageIteratorLevel> stack)
{
	auto level = stack.front();
	stack.pop_front();
	std::list<std::shared_ptr<Box>> boxs;
	do
	{
		auto box = std::make_shared<Box>();
		box->score = result->Confidence(level);
		box->text = result->GetUTF8Text(level);
		result->BoundingBox(level, &box->x1, &box->y1, &box->x2, &box->y2);
		boxs.push_back(std::move(box));
	} while ((result->Next(level)));
	CreateTree(result, stack, boxs);

	return boxs;
}

/*void Docapost::IA::Tesseract::Tesseract::CreateJSON(rapidjson::Writer<rapidjson::StringBuffer>& writer, A2iARC_TranscriptionLevelResults& result)
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
}*/

void Docapost::IA::Tesseract::Tesseract::InitEngine()
{
	int res;
	if ((res = mTessBaseAPI.Init(nullptr, mLang.c_str(), mOem))) {
		throw UninitializedOcrException((boost::format("Could not initialize tesseract : %d") % res).str());
	}
	mTessBaseAPI.SetPageSegMode(mPsm);
}

Docapost::IA::Tesseract::Tesseract::~Tesseract()
{
	mTessBaseAPI.End();
}
