#pragma once
#include "BaseTesseractRunner.h"
#include "NetworkClient.h"
#include "SlaveFileStatus.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class TesseractSlaveRunner : public BaseTesseractRunner<SlaveFileStatus>
			{
			private:

				std::atomic<int> threadToRun;
								
				std::shared_ptr<NetworkClient> mNetwork;

				bool GetTextFromTesseract(tesseract::TessBaseAPI* api, std::vector<unsigned char>* image, std::string& text);
				void ThreadLoop(int id) override;

				void OnMasterConnectedHandler();
				void OnMasterDisconnectHandler();
				void OnMasterStatusChangedHandler(int threadToRun, int done, int skip, int total, int psm, int oem, std::string lang);
				void OnMasterSynchroHandler(int thread, int done, int skip, int total, bool end, boost::unordered_map<std::string, std::vector<unsigned char>*> files);
			public:
				boost::signals2::signal<void(SlaveFileStatus*)> onStartProcessFile;
				boost::signals2::signal<void()> onProcessEnd;

				TesseractSlaveRunner(int port = 12000);
				~TesseractSlaveRunner();

				bool NetworkEnable() const { return mNetwork != nullptr;  }
				int Port() const
				{
					if (NetworkEnable())
						return mNetwork->Port();
					else
						return 0;
				}
				std::thread* Run(int nbThread) override;

				std::string remote_address() const { return mNetwork->GetRemoteAddress(); }
				bool remote_isconnected() const { return mNetwork != nullptr && mNetwork->IsOpen(); }
			};
		}
	}
}
