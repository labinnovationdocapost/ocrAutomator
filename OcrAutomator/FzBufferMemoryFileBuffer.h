#pragma once
#include "MemoryFileBuffer.h"

extern "C" {
#include <mupdf/fitz.h>
}

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class FzBufferMemoryFileBuffer : public MemoryFileBuffer
			{
			private:
				fz_buffer* mBuffer;
				fz_context* mContext;
			public:
				FzBufferMemoryFileBuffer(fz_context* ctx, fz_buffer* buffer);
				unsigned char* data() override;
				int len() override;

				~FzBufferMemoryFileBuffer() override;
			};
		}
	}
}