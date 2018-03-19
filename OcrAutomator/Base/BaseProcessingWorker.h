#pragma once

/*#if __cplusplus <= 199711L
#error This library needs at least a C++11 compliant compiler
#endif*/

#include <string>
#include <stack>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <map>
#include <thread>
#include <condition_variable>
#include <boost/date_time.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
using std::string;

#include "Base/BaseFileStatus.h"
#include "Ocr/BaseOcr.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			template<typename T>
			class BaseProcessingWorker
			{
			protected:
				OcrFactory& mOcrFactory;
				std::atomic_int mNextId{0};

				std::mutex mStackMutex;
				std::mutex mThreadMutex;
				std::mutex mThreadManagementMutex;
				std::map<int, boost::thread*> mThreads;

				const std::string mSoftName = "Docapost Tesseract Automator";

				std::deque<T*> mFiles;

				std::atomic_int mNbThreadToStop{ 0 };
				int mTotal = 0;
				int mSkip = 0;
				int mDone = 0;

				bool mIsEnd = false;
				std::condition_variable mIsWorkDone;
				std::mutex mIsWorkDoneMutex;


				boost::posix_time::ptime mStart;
				boost::posix_time::ptime mEnd;

				BaseProcessingWorker(OcrFactory& ocr) : mOcrFactory(ocr) {}

				virtual void ThreadLoop(int id) = 0;
				void AddFile(T* file)
				{
					boost::lock_guard<std::mutex> lock(mStackMutex);
					mFiles.push_front(file);
				}
				void AddFileBack(T* file)
				{
					boost::lock_guard<std::mutex> lock(mStackMutex);
					mFiles.push_back(file);
				}
				T* GetFile()
				{
					boost::lock_guard<std::mutex> lock(mStackMutex);
					if (mFiles.empty())
					{
						return nullptr;
					}

					auto f = mFiles.front();

					mFiles.pop_front();

					return f;
				}
			public:
				virtual ~BaseProcessingWorker() = default;
				int Total() const { return mTotal; }
				int Done() const { return mDone; }
				int Skip() const { return mSkip; }

				OcrFactory& ocrFactory() const
				{
					return mOcrFactory;
				}

				std::size_t NbThreads() { std::lock_guard<std::mutex> lock(mThreadMutex); return mThreads.size(); }
				boost::posix_time::ptime StartTime() const { return mStart; }
				boost::posix_time::ptime EndTime() const { return mEnd; }
				int NbThreadToStop() const { return mNbThreadToStop; }


				virtual std::thread* Run(int nbThread) = 0;
				void Wait()
				{
					std::unique_lock<std::mutex> lk(mIsWorkDoneMutex);
					mIsWorkDone.wait(lk);
				}
				void Terminate()
				{
					std::unique_lock<std::mutex> lk(mIsWorkDoneMutex);
					mIsWorkDone.notify_all();
				}
				void AddThread()
				{
					int id = mNextId++;
					boost::lock_guard<std::mutex> lockManagement(mThreadManagementMutex);
					mThreads[id] = new boost::thread(&BaseProcessingWorker<T>::ThreadLoop, this, id);
				}
				void RemoveThread()
				{
					boost::lock_guard<std::mutex> lockManagement(mThreadManagementMutex);
					boost::lock_guard<std::mutex> lock(mThreadMutex);

					if (mThreads.size() > mNbThreadToStop + 1)
						mNbThreadToStop++;
				}
			};
		}
	}
}