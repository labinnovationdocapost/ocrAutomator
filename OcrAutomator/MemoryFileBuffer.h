#pragma once



namespace Docapost {
	namespace IA {
		namespace Tesseract {
			/**
			 * \brief Wrapper for the internal buffer containing the file.
			 */
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