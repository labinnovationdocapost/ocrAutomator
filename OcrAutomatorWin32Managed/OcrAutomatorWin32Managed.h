// OcrAutomatorWin32Managed.h
#pragma once

#include "OcrAutomatorMaster.h"

using namespace System;
using namespace System::Collections::Generic;

public ref struct OcrType
{
	calculated_struct* mStruct;
public:

	OcrType(calculated_struct* stru) { mStruct = stru; }

	property bool IsEnum 
	{
		bool get()
		{
			return mStruct->isEnum;
		}
	}
	property List<String^>^ Enum
	{
		List<String^>^ get()
		{
			List<String^>^ list = gcnew List<String^>();
			for(auto& e : mStruct->enu)
			{
				list->Add(gcnew String(e.c_str()));
			}
			return list;
		}
	}
	property String^ Name
	{
		String^ get()
		{
			return gcnew String(mStruct->name.c_str());
		}
	}
	property String^ TypeName
	{
		String^ get()
		{
			return gcnew String(mStruct->typeName.c_str());
		}
	}
};

namespace Docapost {
	namespace OCR {
		public ref class OcrAutomatorModel
		{
			OcrAutomatorMaster* mOcr;


		public :
			OcrAutomatorModel();
			~OcrAutomatorModel();

			void RunOcr();
			List<OcrType^>^ GetConfiguration();

		};
	}
}
