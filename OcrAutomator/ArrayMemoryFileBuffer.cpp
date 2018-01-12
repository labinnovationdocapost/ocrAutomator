#include "ArrayMemoryFileBuffer.h"

Docapost::IA::Tesseract::ArrayMemoryFileBuffer::ArrayMemoryFileBuffer(unsigned char* buffer, int length) : mBuffer(buffer), mLength(length)
{
	
}
Docapost::IA::Tesseract::ArrayMemoryFileBuffer::ArrayMemoryFileBuffer(int length) : mBuffer(new unsigned char[length]), mLength(length)
{

}

unsigned char* Docapost::IA::Tesseract::ArrayMemoryFileBuffer::data()
{
	return mBuffer;
}

int Docapost::IA::Tesseract::ArrayMemoryFileBuffer::len()
{
	return mLength;
}
Docapost::IA::Tesseract::ArrayMemoryFileBuffer::~ArrayMemoryFileBuffer() 
{
	delete mBuffer;
}