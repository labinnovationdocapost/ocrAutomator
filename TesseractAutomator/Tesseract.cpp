#include "Tesseract.h"
#include "UninitializedOcrException.h"

#include <leptonica/allheaders.h>
#include <Magick++.h>
#include "MuPDF.h"
#include "Error.h"

std::vector<unsigned char>* Docapost::IA::Tesseract::Tesseract::ExtractPdfFromImageMagick(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile)
{
	try
	{
		std::lock_guard<std::mutex> lock(*file->mutex_siblings);
		if (file->fileSize > 0)
		{
			return file->data;
		}
		Magick::ReadOptions options;
		options.density(Magick::Geometry(250, 250));
		std::list<Magick::Image> images;

		Magick::readImages(&images, file->name, options);

		size_t i;
		std::list<Magick::Image>::iterator image;
		for (image = images.begin(), i = 0; image != images.end(); image++, i++)
		{
			if ((*file->siblings)[i] == nullptr)
				continue; 
			Magick::Blob blobOut;
			image->colorSpace(MagickCore::RGBColorspace);
			image->quality(80);
			image->density(Magick::Geometry(300, 300));
			image->resolutionUnits(MagickCore::ResolutionType::PixelsPerInchResolution);
			image->magick("JPEG");
			image->write(&blobOut);
			auto data = (char*)blobOut.data();
			(*file->siblings)[i]->data = new std::vector<l_uint8>(data, data + blobOut.length());
			(*file->siblings)[i]->fileSize = (*file->siblings)[i]->data->size();
			if (file->filePosition != (*file->siblings)[i]->filePosition)
			{
				AddFile((*file->siblings)[i]);
			}
		}
		return file->data;
	}
	catch (Magick::ErrorDelegate &ed)
	{
		std::cerr << "Impossible de decoder le fichier " << file->name << " | " << ed.what() << std::endl;
		return nullptr;
	}
	catch (Magick::Exception &e)
	{
		std::cerr << "Impossible de decoder le fichier " << file->name << " | " << e.what() << std::endl;
		return nullptr;
	}
	catch (...)
	{
		auto eptr = std::current_exception();
		auto n = eptr.__cxa_exception_type()->name();
		std::cerr << "Impossible de decoder le fichier " << file->name << " | " << n << std::endl;
		return nullptr;
	}
	return nullptr;
}

std::vector<unsigned char>* Docapost::IA::Tesseract::Tesseract::ExtractPdfFromMuPdf(MasterFileStatus * file, const std::function<void(MasterFileStatus*)>& AddFile)
{
	std::lock_guard<std::mutex> lock(*file->mutex_siblings);
	if (file->fileSize > 0)
	{
		return file->data;
	}

	try
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Extract PDF file " << file->name;
		MuPDF pdf;
		pdf.Extract(file);
	}
	catch(std::exception &e)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Cannot extract PDF file " << file->name << " error: " << e.what();
		return nullptr;
	}
	catch (std::runtime_error &e)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Cannot extract PDF file " << file->name << " error: " << e.what();
		return nullptr;
	}

	bool first = false;
	for(auto s : *file->siblings)
	{
		if (s == nullptr) continue;
		if(file->filePosition != s->filePosition)
		{
			AddFile(s);
		}
	}

	return file->data;
}

Docapost::IA::Tesseract::Tesseract::Tesseract(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang) : BaseOcr(), mPsm(psm), mOem(oem), mLang(lang), mTessBaseAPI(tesseract::TessBaseAPI())
{
	setMsgSeverity(L_SEVERITY_NONE);

	mTessBaseAPI.SetVariable("debug_file", "/dev/null");
	mTessBaseAPI.SetVariable("out", "quiet");

	int res;
	if ((res = mTessBaseAPI.Init(nullptr, mLang.c_str(), mOem))) {
		throw UninitializedOcrException((boost::format("Could not initialize tesseract : %d") % res).str());
	}
	mTessBaseAPI.SetPageSegMode(mPsm);
}

std::vector<unsigned char>* Docapost::IA::Tesseract::Tesseract::LoadFile(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile) {
	if (file->filePosition >= 0)
	{
		//return ExtractPdfFromImageMagick(file, AddFile);
		return ExtractPdfFromMuPdf(file, AddFile);
	}
	else
	{
		FILE* f = fopen(file->name.c_str(), "r");
		if (f == nullptr)
		{
			std::cerr << "Impossible d'ouvrir le fichier " << file->name << std::endl;
			return nullptr;
		}

		// Determine file size
		fseek(f, 0, SEEK_END);
		size_t size = ftell(f);

		file->data = new std::vector<l_uint8>(size);

		rewind(f);
		fread(file->data->data(), sizeof(l_uint8), size, f);
		fclose(f);

		file->fileSize = file->data->size();

		return file->data;
	}
}

bool Docapost::IA::Tesseract::Tesseract::ProcessThroughOcr(std::vector<l_uint8>* imgData, std::string& text) {
	Pix *image = pixReadMem(imgData->data(), imgData->size());
	if (image == nullptr)
	{
		return false;
	}

	mTessBaseAPI.SetImage(image);
	auto outtext = mTessBaseAPI.GetUTF8Text();

	text = string(outtext);

	pixDestroy(&image);
	delete[] outtext;

	return true;
}

Docapost::IA::Tesseract::Tesseract::~Tesseract()
{
	mTessBaseAPI.End();
}
