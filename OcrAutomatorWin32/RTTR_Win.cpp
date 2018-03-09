#include "RTTR.h"
#include "A2IAFactory.h"
#include <rttr/policy.h>


RTTR_REGISTRATION
{
	using namespace rttr;

auto ptr = policy::ctor::as_raw_ptr;
	registration::class_<Docapost::IA::Tesseract::A2IAFactory>("A2IA")(metadata(Metadata_Type::OCR, true))
	.constructor<>()(ptr);

}