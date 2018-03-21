#include "Base/RTTR.h"
#include <rttr/policy.h>
#include "Ocr/NoOcr/NoOcrFactory.h"

RTTR_REGISTRATION
{
	using namespace rttr;

	registration::class_<Docapost::IA::Tesseract::OcrFactory>("OcrFactory")(metadata(Metadata_Type::OCR, false));

	auto ptr = policy::ctor::as_raw_ptr;
	registration::class_<Docapost::IA::Tesseract::TesseractFactory>("Tesseract")(metadata(Metadata_Type::OCR, true))
		.constructor<>()(ptr)
		.property("PSM", select_overload<tesseract::PageSegMode(void) const>(&Docapost::IA::Tesseract::TesseractFactory::Psm), select_overload<void(tesseract::PageSegMode)>(&Docapost::IA::Tesseract::TesseractFactory::Psm))
		.property("OEM", select_overload<tesseract::OcrEngineMode(void) const>(&Docapost::IA::Tesseract::TesseractFactory::Oem), select_overload<void(tesseract::OcrEngineMode)>(&Docapost::IA::Tesseract::TesseractFactory::Oem))
		.property("Lang", select_overload<std::string(void) const>(&Docapost::IA::Tesseract::TesseractFactory::Lang), select_overload<void(std::string)>(&Docapost::IA::Tesseract::TesseractFactory::Lang));

	registration::class_<Docapost::IA::Tesseract::NoOcrFactory>("NoOcr")(metadata(Metadata_Type::OCR, true))
		.constructor<>()(ptr)
		.property("Num", select_overload<int(void) const>(&Docapost::IA::Tesseract::NoOcrFactory::Num), select_overload<void(int)>(&Docapost::IA::Tesseract::NoOcrFactory::Num));

	registration::enumeration<Docapost::IA::Tesseract::OutputFlags>("OutputFlags")
	(
		value("None", Docapost::IA::Tesseract::OutputFlags::None),
		value("Text", Docapost::IA::Tesseract::OutputFlags::Text),
		value("Exif", Docapost::IA::Tesseract::OutputFlags::Exif),
		value("Flattern", Docapost::IA::Tesseract::OutputFlags::Flattern),
		value("MemoryText", Docapost::IA::Tesseract::OutputFlags::MemoryText),
		value("MemoryImage", Docapost::IA::Tesseract::OutputFlags::MemoryImage)
	);

	registration::enumeration<tesseract::PageSegMode>("PageSegMode")
	(
		value("PSM_AUTO", tesseract::PageSegMode::PSM_AUTO),
		value("PSM_AUTO_ONLY", tesseract::PageSegMode::PSM_AUTO_ONLY),
		value("PSM_AUTO_OSD", tesseract::PageSegMode::PSM_AUTO_OSD),
		value("PSM_CIRCLE_WORD", tesseract::PageSegMode::PSM_CIRCLE_WORD),
		value("PSM_COUNT", tesseract::PageSegMode::PSM_COUNT),
		value("PSM_OSD_ONLY", tesseract::PageSegMode::PSM_OSD_ONLY),
		value("PSM_RAW_LINE", tesseract::PageSegMode::PSM_RAW_LINE),
		value("PSM_SINGLE_BLOCK", tesseract::PageSegMode::PSM_SINGLE_BLOCK),
		value("PSM_SINGLE_BLOCK_VERT_TEXT", tesseract::PageSegMode::PSM_SINGLE_BLOCK_VERT_TEXT),
		value("PSM_SINGLE_CHAR", tesseract::PageSegMode::PSM_SINGLE_CHAR),
		value("PSM_SINGLE_COLUMN", tesseract::PageSegMode::PSM_SINGLE_COLUMN),
		value("PSM_SINGLE_LINE", tesseract::PageSegMode::PSM_SINGLE_LINE),
		value("PSM_SINGLE_WORD", tesseract::PageSegMode::PSM_SINGLE_WORD),
		value("PSM_SPARSE_TEXT", tesseract::PageSegMode::PSM_SPARSE_TEXT),
		value("PSM_SPARSE_TEXT_OSD", tesseract::PageSegMode::PSM_SPARSE_TEXT_OSD)
	);
	registration::enumeration<tesseract::OcrEngineMode>("OcrEngineMode")
	(
		value("OEM_CUBE_ONLY", tesseract::OcrEngineMode::OEM_CUBE_ONLY),
		value("OEM_DEFAULT", tesseract::OcrEngineMode::OEM_DEFAULT),
		value("OEM_LSTM_ONLY", tesseract::OcrEngineMode::OEM_LSTM_ONLY),
		value("OEM_TESSERACT_CUBE_COMBINED", tesseract::OcrEngineMode::OEM_TESSERACT_CUBE_COMBINED),
		value("OEM_TESSERACT_LSTM_COMBINED", tesseract::OcrEngineMode::OEM_TESSERACT_LSTM_COMBINED),
		value("OEM_TESSERACT_ONLY", tesseract::OcrEngineMode::OEM_TESSERACT_ONLY)
	);
}