#pragma once
#include <XMP_IO.hpp>
#include "Buffer/MemoryFileBuffer.h"
#include "TempXMPIO.h"

class MemoryXMPIO : XMP_IO
{
	Docapost::IA::Tesseract::MemoryFileBuffer* mBuffer;
	//std::vector<char>* buffer;
	signed long long mPosition = 0;

	bool mIsBufferRelease = false;
	TempXMPIO* mTmpIO = nullptr;
public:
	MemoryXMPIO(Docapost::IA::Tesseract::MemoryFileBuffer* buffer);

	XMP_Uns32 Read(void* buffer, XMP_Uns32 count, bool readAll = false) override;

	void Write(const void* buffer, XMP_Uns32 count) override;

	XMP_Int64 Seek(XMP_Int64 offset, SeekMode mode) override;

	XMP_Int64 Length() override;

	Docapost::IA::Tesseract::MemoryFileBuffer* Buffer();

	void Truncate(XMP_Int64 length) override;

	XMP_IO* DeriveTemp() override;

	void AbsorbTemp() override;

	void DeleteTemp() override;

	~MemoryXMPIO() override;
};
