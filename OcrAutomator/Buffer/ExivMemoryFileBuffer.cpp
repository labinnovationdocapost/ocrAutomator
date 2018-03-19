#include "ExivMemoryFileBuffer.h"

Docapost::IA::Tesseract::ExivMemoryFileBuffer::ExivMemoryFileBuffer(Exiv2::DataBuf& exiv2Buffer) : mBuffer(exiv2Buffer)
{
}

unsigned char* Docapost::IA::Tesseract::ExivMemoryFileBuffer::data()
{
	return mBuffer.pData_;
}

int Docapost::IA::Tesseract::ExivMemoryFileBuffer::len()
{
	return mBuffer.size_;
}
Docapost::IA::Tesseract::ExivMemoryFileBuffer::~ExivMemoryFileBuffer()
{
}