#pragma once
#include "MemoryFileBuffer.h"
#include <vector>

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class VectorMemoryFileBuffer : public MemoryFileBuffer
			{
			private:
				std::vector<char>* mBuffer;
			public:
				VectorMemoryFileBuffer(std::vector<char>*);
				VectorMemoryFileBuffer(int size);
				unsigned char* data() override;
				int len() override;
				std::vector<char>* Buffer();

				~VectorMemoryFileBuffer() override;
			};
		}
	}
}