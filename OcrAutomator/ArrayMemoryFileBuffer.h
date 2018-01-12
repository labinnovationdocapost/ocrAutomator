#pragma once
#include "MemoryFileBuffer.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class ArrayMemoryFileBuffer : public MemoryFileBuffer
			{
			private:
				unsigned char* mBuffer;
				int mLength;
			public:
				ArrayMemoryFileBuffer(unsigned char* buffer, int length);
				ArrayMemoryFileBuffer(int length);
				unsigned char* data() override;
				int len() override;

				~ArrayMemoryFileBuffer() override;
			};
		}
	}
}