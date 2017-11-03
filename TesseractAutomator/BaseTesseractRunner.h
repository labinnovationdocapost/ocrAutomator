#pragma once

#if __cplusplus <= 199711L
#error This library needs at least a C++11 compliant compiler
#endif

#include <string>
#include <stack>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <map>
#include <thread>
#include <boost/date_time.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
using std::string;
#include <tesseract/baseapi.h>

#include "FileStatus.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			template<typename T>
			class BaseTesseractRunner
			{
			protected:
				std::atomic_int mNextId{0};

				std::mutex mStackMutex;
				std::mutex mThreadMutex;
				std::map<int, std::thread*> mThreads;

				const std::string mSoftName = "Docapost Tesseract Automator";

				std::deque<T*> mFiles;

				int mNbThreadToStop = 0;
				int mTotal = 0;
				int mSkip = 0;
				int mDone = 0;

				bool mIsEnd = false;

				tesseract::PageSegMode mPsm = tesseract::PageSegMode::PSM_AUTO;
				tesseract::OcrEngineMode mOem = tesseract::OcrEngineMode::OEM_DEFAULT;
				std::string mLang;

				boost::posix_time::ptime mStart;
				boost::posix_time::ptime mEnd;

				BaseTesseractRunner() {}
				BaseTesseractRunner(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang) : mPsm(psm), mOem(oem), mLang(lang) {}

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
				virtual ~BaseTesseractRunner() = default;
				int Total() const { return mTotal; }
				int Done() const { return mDone; }
				int Skip() const { return mSkip; }

				tesseract::PageSegMode Psm() const { return mPsm; }
				tesseract::OcrEngineMode Oem() const { return mOem; }
				std::size_t NbThreads() { boost::lock_guard<std::mutex> lock(mThreadMutex); return mThreads.size(); }
				boost::posix_time::ptime StartTime() const { return mStart; }
				boost::posix_time::ptime EndTime() const { return mEnd; }
				int NbThreadToStop() const { return mNbThreadToStop; }


				virtual std::thread* Run(int nbThread) = 0;
				void Wait()
				{
					for (auto& th : mThreads)
						th.second->join();
				}
				void AddThread()
				{
					int id = mNextId++;
					mThreads[id] = new std::thread(&BaseTesseractRunner<T>::ThreadLoop, this, id);
				}
				void RemoveThread()
				{
					boost::lock_guard<std::mutex> lock(mThreadMutex);

					if (mThreads.size() > mNbThreadToStop + 1)
						mNbThreadToStop++;
				}
			};
		}
	}
}