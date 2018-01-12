#include "FzBufferMemoryFileBuffer.h"
#include "fz_buffer_impl.h"

Docapost::IA::Tesseract::FzBufferMemoryFileBuffer::FzBufferMemoryFileBuffer(fz_context* ctx, fz_buffer* buffer) : mBuffer(buffer), mContext(fz_clone_context(ctx))
{

}

unsigned char* Docapost::IA::Tesseract::FzBufferMemoryFileBuffer::data()
{
	return mBuffer->data;
}

int Docapost::IA::Tesseract::FzBufferMemoryFileBuffer::len()
{
	return mBuffer->len;
}
Docapost::IA::Tesseract::FzBufferMemoryFileBuffer::~FzBufferMemoryFileBuffer()
{
	fz_drop_buffer(mContext,mBuffer);
}