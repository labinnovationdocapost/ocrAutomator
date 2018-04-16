#include "VectorMemoryFileBuffer.h"

Docapost::IA::Tesseract::VectorMemoryFileBuffer::VectorMemoryFileBuffer(std::vector<char>* vector) : mBuffer(vector)
{
}

unsigned char* Docapost::IA::Tesseract::VectorMemoryFileBuffer::data()
{
	return (unsigned char*)mBuffer->data();
}

int Docapost::IA::Tesseract::VectorMemoryFileBuffer::len()
{
	return mBuffer->size();
}

std::vector<char>* Docapost::IA::Tesseract::VectorMemoryFileBuffer::Buffer()
{
	return mBuffer;
}

Docapost::IA::Tesseract::VectorMemoryFileBuffer::~VectorMemoryFileBuffer()
{
	delete mBuffer;
}
