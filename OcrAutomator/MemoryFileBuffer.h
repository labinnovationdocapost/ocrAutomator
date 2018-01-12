#pragma once



namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class MemoryFileBuffer
			{
			public:
				virtual ~MemoryFileBuffer() = default;
				virtual unsigned char* data() = 0;
				virtual int len() = 0;
			};
		}
	}
}