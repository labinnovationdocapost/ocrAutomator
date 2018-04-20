#pragma once
#include "Buffer/MemoryFileBuffer.h"
#include <XMP_IO.hpp>

class TempXMPIO : XMP_IO
{
	std::vector<char>* mBuffer;
	signed long long mPosition = 0;

	bool mIsBufferRelease = false;
	TempXMPIO* mTmpIO = nullptr;
public:
	TempXMPIO();

	XMP_Uns32 Read(void* buffer, XMP_Uns32 count, bool readAll = false) override;

	void Write(const void* buffer, XMP_Uns32 count) override;

	XMP_Int64 Seek(XMP_Int64 offset, SeekMode mode) override;

	XMP_Int64 Length() override;

	std::vector<char>* Buffer();

	void Truncate(XMP_Int64 length) override;

	XMP_IO* DeriveTemp() override;

	void AbsorbTemp() override;

	void DeleteTemp() override;

	~TempXMPIO() override;
};
