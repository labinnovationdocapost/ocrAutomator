#pragma once
#include <rttr/registration>

namespace Docapost {
	namespace IA {
		namespace Tesseract {

			enum OutputFlags
			{
				MemoryImage = 16,
				MemoryText = 8,
				Flattern = 4,
				Exif = 2,
				Text = 1,
				None = 0
			};
			inline OutputFlags operator|(OutputFlags a, OutputFlags b)
			{
				return static_cast<OutputFlags>(static_cast<int>(a) | static_cast<int>(b));
			}
			inline OutputFlags& operator|=(OutputFlags& a, OutputFlags b)
			{
				return a = a | b;
			}
		}
	}
}
