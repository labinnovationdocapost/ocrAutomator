#include "Tesseract.h"
#include "UninitializedOcrException.h"

#include <leptonica/allheaders.h>

#ifdef MAGICK
#include <Magick++.h>
#endif

#include "MuPDF.h"
#include "Error.h"
#include "ArrayMemoryFileBuffer.h"

#ifdef MAGICK
Docapost::IA::Tesseract::MemoryFileBuffer* Docapost::IA::Tesseract::Tesseract::ExtractPdfFromImageMagick(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile)
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
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cannot decode file " << file->name << " | " << ed.what() << std::endl;
		return nullptr;
	}
	catch (Magick::Exception &e)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cannot decode file " << file->name << " | " << e.what() << std::endl;
		return nullptr;
	}
	catch (...)
	{
		auto eptr = std::current_exception();
		auto n = eptr.__cxa_exception_type()->name();
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cannot decode file " << file->name << " | " << n << std::endl;
		return nullptr;
	}
	return nullptr;
}

Docapost::IA::Tesseract::MemoryFileBuffer* Docapost::IA::Tesseract::Tesseract::ExtractPdfFromMuPdf(MasterFileStatus * file, const std::function<void(MasterFileStatus*)>& AddFile) { return nullptr; }
#endif

#ifndef MAGICK
Docapost::IA::Tesseract::MemoryFileBuffer* Docapost::IA::Tesseract::Tesseract::ExtractPdfFromImageMagick(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile) { return nullptr; }

Docapost::IA::Tesseract::MemoryFileBuffer* Docapost::IA::Tesseract::Tesseract::ExtractPdfFromMuPdf(MasterFileStatus * file, const std::function<void(MasterFileStatus*)>& AddFile)
{
	std::lock_guard<std::mutex> lock(*file->mutex_siblings);
	if (file->buffer != nullptr && file->buffer->len() > 0)
	{
		return file->buffer;
	}

	try
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Extract PDF file " << file->name;
		Docapost::IA::MuPDF::MuPDF pdf;
		pdf.Extract(file, mImageFormat);
	}
	catch(std::exception &e)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cannot extract PDF file " << file->name << " error: " << e.what();
		return nullptr;
	}
	catch (std::runtime_error &e)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Cannot extract PDF file " << file->name << " error: " << e.what();
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

	return file->buffer;
}
#endif

Docapost::IA::Tesseract::Tesseract::Tesseract(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang, ImageFormatEnum format) : BaseOcr(format), mPsm(psm), mOem(oem), mLang(lang), mTessBaseAPI(tesseract::TessBaseAPI())
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

Docapost::IA::Tesseract::MemoryFileBuffer* Docapost::IA::Tesseract::Tesseract::LoadFile(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile) {
	if (file->filePosition >= 0)
	{

#ifdef MAGICK
		return ExtractPdfFromImageMagick(file, AddFile);
#endif
#ifndef MAGICK
		return ExtractPdfFromMuPdf(file, AddFile);
#endif
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

		file->buffer = new ArrayMemoryFileBuffer(size);

		rewind(f);
		fread(file->buffer->data(), sizeof(unsigned char), size, f);
		fclose(f);

		return file->buffer;
	}
}

std::unique_ptr<std::string> Docapost::IA::Tesseract::Tesseract::ProcessThroughOcr(Docapost::IA::Tesseract::MemoryFileBuffer* imgData) {
	Pix *image = pixReadMem(imgData->data(), imgData->len());
	if (image == nullptr)
	{
		return nullptr;
	}

	mTessBaseAPI.SetImage(image);
	auto outtext = mTessBaseAPI.GetUTF8Text();

	auto text = std::unique_ptr<std::string>{ new string(outtext) };

	pixDestroy(&image);
	delete[] outtext;

	return text;
}

Docapost::IA::Tesseract::Tesseract::~Tesseract()
{
	mTessBaseAPI.End();
}
