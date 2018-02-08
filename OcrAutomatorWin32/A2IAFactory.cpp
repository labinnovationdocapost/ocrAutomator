#include "A2IAFactory.h"
#include "A2IA.h"

Docapost::IA::Tesseract::A2IA* Docapost::IA::Tesseract::A2IAFactory::CreateNew()
{
	return new A2IA(mImageFormat);
}
