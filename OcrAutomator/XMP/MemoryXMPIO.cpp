#include "MemoryXMPIO.h"
#include "Buffer/VectorMemoryFileBuffer.h"
#include <cstring>

MemoryXMPIO::MemoryXMPIO(Docapost::IA::Tesseract::MemoryFileBuffer* buffer): mBuffer(buffer)
{
}

XMP_Uns32 MemoryXMPIO::Read(void* buffer, XMP_Uns32 count, bool readAll)
{
	if (count > (this->mBuffer->len() - this->mPosition))
	{
		if (readAll) throw XMP_Error(kXMPErr_EnforceFailure, "XMPFiles_IO::Read, not enough data");
		count = (XMP_Uns32)(this->mBuffer->len() - this->mPosition);
	}

	std::memcpy(buffer, this->mBuffer->data() + mPosition, count);
	mPosition += count;
	return count;
}

void MemoryXMPIO::Write(const void* buffer, XMP_Uns32 count)
{
	if (mPosition + count > this->mBuffer->len())
		throw XMP_Error(0,"Not supported");
	std::memcpy(this->mBuffer->data() + mPosition, (char*)buffer, count);
	mPosition += count;
}

XMP_Int64 MemoryXMPIO::Seek(XMP_Int64 offset, SeekMode mode)
{
	XMP_Int64 newOffset = offset;
	if (mode == kXMP_SeekFromCurrent)
	{
		newOffset += this->mPosition;
	}
	else if (mode == kXMP_SeekFromEnd)
	{
		newOffset += this->mBuffer->len();
	}

	if (newOffset <= this->mBuffer->len())
	{
		this->mPosition = newOffset;
	}
	else
	{
		throw XMP_Error(kXMPErr_EnforceFailure, "XMPFiles_IO::Seek, Cannot increase size");
	}

	return this->mPosition;
}

XMP_Int64 MemoryXMPIO::Length()
{
	return mBuffer->len();
}

Docapost::IA::Tesseract::MemoryFileBuffer* MemoryXMPIO::Buffer()
{
	mIsBufferRelease = true;
	return mBuffer;
}

void MemoryXMPIO::Truncate(XMP_Int64 length)
{
}

XMP_IO* MemoryXMPIO::DeriveTemp()
{
	return (XMP_IO*)(mTmpIO = new TempXMPIO());
}

void MemoryXMPIO::AbsorbTemp()
{
	delete mBuffer;
	mBuffer = new Docapost::IA::Tesseract::VectorMemoryFileBuffer(mTmpIO->Buffer());
}

void MemoryXMPIO::DeleteTemp()
{
	delete mTmpIO;
}

MemoryXMPIO::~MemoryXMPIO()
{
	if (!mIsBufferRelease)
		delete mBuffer;
	if (mTmpIO != nullptr)
		delete mTmpIO;
}
