#include "StringMemoryFileBuffer.h"

Docapost::IA::Tesseract::StringMemoryFileBuffer::StringMemoryFileBuffer(std::string* str) : mBuffer(str)
{

}

unsigned char* Docapost::IA::Tesseract::StringMemoryFileBuffer::data()
{
	return (unsigned char*)mBuffer->data();
}

int Docapost::IA::Tesseract::StringMemoryFileBuffer::len()
{
	return mBuffer->length();
}
Docapost::IA::Tesseract::StringMemoryFileBuffer::~StringMemoryFileBuffer()
{
	delete mBuffer;
}