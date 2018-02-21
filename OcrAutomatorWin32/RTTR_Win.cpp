#include "RTTR.h"
#include "A2IAFactory.h"


RTTR_REGISTRATION
{
	using namespace rttr;

	registration::class_<Docapost::IA::Tesseract::A2IAFactory>("A2IA")(metadata(Metadata_Type::OCR, true))
	.constructor<>()(policy::ctor::as_raw_ptr);

}