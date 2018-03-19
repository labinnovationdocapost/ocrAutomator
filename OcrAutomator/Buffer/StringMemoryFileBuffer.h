#pragma once
#include "MemoryFileBuffer.h"
#include <string>

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class StringMemoryFileBuffer : public MemoryFileBuffer
			{
			private:
				std::string* mBuffer;
			public:
				StringMemoryFileBuffer(std::string* str);
				unsigned char* data() override;
				int len() override;

				virtual ~StringMemoryFileBuffer();
			};
		}
	}
}