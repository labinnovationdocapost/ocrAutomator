#pragma once
#include "MemoryFileBuffer.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class JpegTurboMemoryFileBuffer : public MemoryFileBuffer
			{
			private:
				unsigned char* mBuffer;
				int mLength;
			public:
				JpegTurboMemoryFileBuffer(unsigned char* buffer, int length);
				unsigned char* data() override;
				int len() override;

				~JpegTurboMemoryFileBuffer() override;
			};
		}
	}
}