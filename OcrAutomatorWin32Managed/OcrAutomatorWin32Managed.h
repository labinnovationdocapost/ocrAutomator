// OcrAutomatorWin32Managed.h
#pragma once

#include "OcrAutomatorMaster.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading;

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
delegate void FileRecievedHandle(OcrResult* str);

class OcrAutomatorPDF
{
public:
	static int nbpage;
	static std::string name;
	static std::vector<OcrResult*> results;
};
namespace Docapost {
	namespace OCR {
		public ref class OcrAutomatorModel
		{
			OcrAutomatorMaster* mOcr;
		public :
			static std::unordered_map<std::string,OcrAutomatorPDF*>* pdf = new std::unordered_map<std::string,OcrAutomatorPDF*>();
			static AutoResetEvent^ event = gcnew AutoResetEvent(false);

			OcrAutomatorModel();
			~OcrAutomatorModel();
			void RunOcr();
			List<OcrType^>^ GetConfiguration();

		};

	}
}
