
#include "MasterProcessingWorker.h"
#include "Base/Error.h"
#include "Ocr/Tesseract/TesseractFactory.h"

void Docapost::IA::Tesseract::MasterProcessingWorker::OnSlaveConnectHandler(NetworkSession* ns, int thread, std::string hostname)
{
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Client connected : " << hostname;

	if (mSlaves[ns->Id()] == nullptr) mSlaves[ns->Id()] = std::make_shared<SlaveState>();
	mSlaves[ns->Id()]->NbThread = thread;
	mSlaves[ns->Id()]->Name = ns->Hostname();

	auto tf = dynamic_cast<TesseractFactory&>(mOcrFactory);
	ns->SendStatus(this->mDone, this->mSkip, this->mTotal, tf.Psm(), tf.Oem(), tf.Lang());
}

void Docapost::IA::Tesseract::MasterProcessingWorker::OnSlaveSynchroHandler(NetworkSession* ns, int thread, int required, std::vector<std::tuple<boost::uuids::uuid, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::vector<std::string>*>>& results)
{
	CatchAllErrorSignals();
	// On garde une référence sur l'objet pou qu'il ne soit supprimer que quand plus personne ne l'utilise
	auto slave = mSlaves[ns->Id()];
	slave->NbThread = thread;
	slave->PendingProcessed += required;
	slave->PendingNotProcessed += required;
	try
	{
		for (auto& res : results)
		{
			boost::uuids::uuid uuid;
			boost::posix_time::ptime start, end;
			boost::posix_time::time_duration ellapsed;
			auto file = GetFileSend(std::get<0>(res));

			if (file == nullptr)
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << ns->Hostname() << "Unable to locate : " << std::get<0>(res) << "\n";
				continue;
			}

			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << ns->Hostname() << "Getting back " << file->name << "[" << file->filePosition << "]" << "\n";

			std::vector<std::string>* vector = nullptr;
			std::tie(uuid, file->thread, file->start, file->end, file->ellapsed, vector) = res;
			file->isEnd = true;
			file->hostname = ns->Hostname();

			file->result = std::unique_ptr<std::vector<std::string>>(vector);

			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << ns->Hostname() << "Writing Output " << file->name << "[" << file->filePosition << "]" << "\n";
			CreateOutput(file);
			auto isMerge = MergeResult(file);
			mDone++;

			onEndProcessFile(file);

			RemoveFileSend(uuid);

			FreeBuffers(file, mOutputTypes & OutputFlags::MemoryImage, mOutputTypes & OutputFlags::MemoryText, isMerge);

			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << ns->Hostname() << "Clean " << file->name << "[" << file->filePosition << "]" << "\n";
		}

		std::lock_guard<std::mutex> lock(slave->ClientMutex);
		if (slave->PendingNotProcessed > 0 && slave->PendingProcessed > 0 && !mIsTerminated && slave->Thread == nullptr)
		{
			slave->Thread = new boost::thread(&MasterProcessingWorker::SendFilesToClient, this, ns);
		}
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::debug) << "Synchro Ack";
		ns->SendSynchro(mThreads.size(), mDone, mSkip, mTotal, mIsEnd, slave->PendingNotProcessed, boost::unordered_map<boost::uuids::uuid, Docapost::IA::Tesseract::MemoryFileBuffer*>());
	}
	catch (std::exception& e)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Synchro error : " << e.what();
	}
}

void Docapost::IA::Tesseract::MasterProcessingWorker::OnSlaveDisconnectHandler(NetworkSession* ns, boost::unordered_map<boost::uuids::uuid, bool>& noUsed)
{
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Client disconnected : " << ns->Hostname() << " | " << "File get back : " << noUsed.size();
	if (ns == nullptr)
		return;


	if(mSlaves.find(ns->Id()) != mSlaves.end())
	{
		auto slave = mSlaves[ns->Id()];
		slave->Terminated = true;
		std::lock_guard<std::mutex> lock2(slave->ClientMutex);
		mSlaves.erase(ns->Id());
	}
	std::lock_guard<std::mutex> lock(mNetworkMutex);
	try
	{
		for (auto& file : noUsed)
		{
			if (!file.second)
			{
				mFileSend[file.first]->hostname = "";
				AddFile(mFileSend[file.first]);
				onFileCanceled(mFileSend[file.first]);
				mFileSend.erase(file.first);
			}
		}

	}
	catch (std::exception& e)
	{
		std::cout << "ERROR !!! : " << e.what() << std::endl;
	}
}


bool Docapost::IA::Tesseract::MasterProcessingWorker::HasFileToSend(std::shared_ptr<SlaveState> slave)
{
	if (slave->Terminated)
		return false;
	// Cette lambda expression permet de boucler sans conflit par client
	std::lock_guard<std::mutex> lock(slave->ClientMutex);
	if (slave->PendingProcessed > 0)
	{
		--slave->PendingProcessed;
		return true;
	}
	return false;
}

void Docapost::IA::Tesseract::MasterProcessingWorker::SendFilesToClient(NetworkSession* ns)
{
	auto slave = mSlaves[ns->Id()];
	try
	{
		auto ocr = mOcrFactory.CreateNew();
		while (HasFileToSend(slave))
		{
			MasterFileStatus*  file = GetFile();
			if (file == nullptr)
			{
				++slave->PendingProcessed;
				break;
			}

			boost::uuids::uuid id;
			{
				std::lock_guard<std::mutex> lock(mNetworkMutex);
				id = mGen();
			}

			file->uuid = id;
			file->hostname = ns->Hostname();


			if (nullptr != ocr->LoadFile(file, [this](MasterFileStatus* file) {this->AddFile(file); }))
			{
				if (slave->Terminated)
				{
					file->hostname = "";
					this->AddFile(file);
					break;
				}

				boost::unordered_map<boost::uuids::uuid, MemoryFileBuffer*> filesToSend;
				filesToSend[id] = file->buffer;
				AddFileSend(file);


				if (filesToSend.size() == 0)
					return;
				std::lock_guard<std::mutex> lock(slave->ClientMutex);
				if (!slave->Terminated)
				{
					onStartProcessFile(file);
					BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::debug) << std::this_thread::get_id() << " | Netowrk interface is open -> sending 1 files\n";
					--slave->PendingNotProcessed;
					ns->SendSynchro(mThreads.size(), mDone, mSkip, mTotal, mIsEnd, slave->PendingNotProcessed, filesToSend);
				}
				else
				{
					BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << std::this_thread::get_id() << " | Netowrk interface closed -> rollback\n";
					std::lock_guard<std::mutex> lock2(mNetworkMutex);
					for (auto fileToSend : filesToSend)
					{
						auto file = GetFileSend(fileToSend.first);

						file->hostname = "";
						this->AddFile(file);
						onFileCanceled(file);
						RemoveFileSend(fileToSend.first);
					}
				}
			}
			else
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << std::this_thread::get_id() << " | Rollback PendingProcessed " << slave->PendingProcessed << "\n";
				++slave->PendingProcessed;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
		}
		delete ocr;
	}
	catch (std::exception& e)
	{
		std::cout << "ERROR !!! : " << e.what() << std::endl;
	}
	std::lock_guard<std::mutex> lock(slave->ClientMutex);
	slave->Thread->detach();
	delete slave->Thread;
	slave->Thread = nullptr;

}