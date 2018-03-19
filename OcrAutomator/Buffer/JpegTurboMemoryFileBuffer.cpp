#include "JpegTurboMemoryFileBuffer.h"
#include <turbojpeg.h>

Docapost::IA::Tesseract::JpegTurboMemoryFileBuffer::JpegTurboMemoryFileBuffer(unsigned char* buffer, int length) : mBuffer(buffer), mLength(length)
{

}

unsigned char* Docapost::IA::Tesseract::JpegTurboMemoryFileBuffer::data()
{
	return mBuffer;
}

int Docapost::IA::Tesseract::JpegTurboMemoryFileBuffer::len()
{
	return mLength;
}
Docapost::IA::Tesseract::JpegTurboMemoryFileBuffer::~JpegTurboMemoryFileBuffer()
{
	tjFree(mBuffer);
}