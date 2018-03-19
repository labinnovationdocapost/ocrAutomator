#pragma once
#pragma once
#include "MemoryFileBuffer.h"
#include <exiv2/exiv2.hpp>

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class ExivMemoryFileBuffer : public MemoryFileBuffer
			{
			private:
				Exiv2::DataBuf mBuffer;
			public:
				ExivMemoryFileBuffer(Exiv2::DataBuf& exiv2Buffer);
				unsigned char* data() override;
				int len() override;

				virtual ~ExivMemoryFileBuffer();
			};
		}
	}
}