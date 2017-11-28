#pragma once

#include <string>

using std::string;
#include <tesseract/baseapi.h>

#include <iostream>
#include <boost/format.hpp>
#include "BaseFileStatus.h"
#include "MasterFileStatus.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {

			class BaseOcr
			{
			protected:
			public:
				virtual ~BaseOcr() = default;

				BaseOcr() 
				{

				}

				virtual std::vector<unsigned char>* LoadFile(MasterFileStatus* file, const std::function<void(MasterFileStatus*)>& AddFile) = 0;
				virtual bool ProcessThroughOcr(std::vector<unsigned char>* imgData, std::string& text) = 0;
			};
			class OcrFactory
			{
			public:
				virtual ~OcrFactory() = default;
				virtual std::shared_ptr<BaseOcr> CreateNew() = 0;
			};

		}
	}
}