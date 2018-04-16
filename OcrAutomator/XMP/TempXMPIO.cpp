#include "TempXMPIO.h"
#include <cstring>

TempXMPIO::TempXMPIO() : mBuffer(new std::vector<char>())
{
}

XMP_Uns32 TempXMPIO::Read(void* buffer, XMP_Uns32 count, bool readAll)
{
	if (count > (this->mBuffer->size() - this->mPosition))
	{
		if (readAll) throw XMP_Error(kXMPErr_EnforceFailure, "XMPFiles_IO::Read, not enough data");
		count = (XMP_Uns32)(this->mBuffer->size() - this->mPosition);
	}

	std::memcpy(buffer, this->mBuffer->data() + mPosition, count);
	mPosition += count;
	return count;
}

void TempXMPIO::Write(const void* buffer, XMP_Uns32 count)
{
	if (mPosition + count > this->mBuffer->size())
		this->mBuffer->reserve(this->mPosition + count);
	this->mBuffer->resize(count + mPosition);
	std::memcpy(this->mBuffer->data() + mPosition, (char*)buffer, count);
	mPosition += count;
}

XMP_Int64 TempXMPIO::Seek(XMP_Int64 offset, SeekMode mode)
{
	XMP_Int64 newOffset = offset;
	if (mode == kXMP_SeekFromCurrent)
	{
		newOffset += this->mPosition;
	}
	else if (mode == kXMP_SeekFromEnd)
	{
		newOffset += this->mBuffer->size();
	}

	if (newOffset <= this->mBuffer->size())
	{
		this->mPosition = newOffset;
	}
	else
	{
		/*Host_IO::SetEOF(this->fileRef, newOffset);	// Extend a file open for writing.
		this->buffer->len() = newOffset;*/
		this->mPosition = newOffset;
		mBuffer->reserve(this->mPosition);
	}

	return this->mPosition;
}

XMP_Int64 TempXMPIO::Length()
{
	return mBuffer->size();
}

std::vector<char>* TempXMPIO::Buffer()
{
	mIsBufferRelease = true;
	return mBuffer;
}

void TempXMPIO::Truncate(XMP_Int64 length)
{
	mBuffer->resize(0);
}

XMP_IO* TempXMPIO::DeriveTemp()
{
	return (XMP_IO*)(mTmpIO = new TempXMPIO());
}

void TempXMPIO::AbsorbTemp()
{
	if (!mIsBufferRelease)
		delete mBuffer;
	mBuffer = mTmpIO->Buffer();
}

void TempXMPIO::DeleteTemp()
{
}

TempXMPIO::~TempXMPIO()
{
	if (!mIsBufferRelease)
		delete mBuffer;
	if(mTmpIO != nullptr)
		delete mTmpIO;
}
