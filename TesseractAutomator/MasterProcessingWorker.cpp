#include "MasterProcessingWorker.h"

#include <fstream>
#include <future>
#include <exiv2/exiv2.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <Magick++.h>
#include <boost/format.hpp>
#include <podofo/podofo.h>
#include "Error.h"
#include "TesseractFactory.h"
#include "UninitializedOcrException.h"


Docapost::IA::Tesseract::MasterProcessingWorker::MasterProcessingWorker(OcrFactory& ocr, Docapost::IA::Tesseract::OutputFlags types, int port) :
	BaseProcessingWorker(ocr),
	mOutputTypes(types)
{
	PoDoFo::PdfError::EnableDebug(false);
	PoDoFo::PdfError::EnableLogging(false);

	std::cerr << "Lancement du réseau sur le port " << port << std::endl;
	while (port < 65565)
	{
		try
		{
			mNetwork = boost::make_shared<Network>(port);
			mNetwork->onSlaveConnect.connect(boost::bind(&MasterProcessingWorker::OnSlaveConnectHandler, this, _1, _2, _3));

			mNetwork->onSlaveSynchro.connect(boost::bind(&MasterProcessingWorker::OnSlaveSynchroHandler, this, _1, _2, _3, _4));

			mNetwork->onSlaveDisconnect.connect(boost::bind(&MasterProcessingWorker::OnSlaveDisconnectHandler, this, _1, _2));

			mNetwork->InitBroadcastReceiver();
			mNetwork->InitComm();
			mNetworkThread = new std::thread([this]()
			{
				CatchAllErrorSignals();
				mNetwork->Start();
			});
			break;
		}
		catch (...)
		{
			std::cerr << "/!\ Lancement du réseau impossible sur le port " << port << std::endl;

			if (mNetwork != nullptr)
				mNetwork->Stop();
			if (mNetworkThread != nullptr && mNetworkThread->joinable())
			{
				mNetworkThread->join();
				delete mNetworkThread;
			}
			port++;
		}
	}
}

void Docapost::IA::Tesseract::MasterProcessingWorker::OnSlaveConnectHandler(NetworkSession* ns, int thread, std::string hostname)
{
	mSlaves[ns->Hostname()] = thread;

	auto tf = dynamic_cast<TesseractFactory&>(mOcrFactory);
	ns->SendStatus(this->mDone, this->mSkip, this->mTotal, tf.Psm(), tf.Oem(), tf.Lang());
}

void Docapost::IA::Tesseract::MasterProcessingWorker::OnSlaveSynchroHandler(NetworkSession* ns, int thread, int required, std::vector<std::tuple<std::string, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::string>>& results)
{
	mSlaves[ns->Hostname()] = thread;
	std::lock_guard<std::mutex> lock(mNetworkMutex);
	try
	{
		for (auto& res : results)
		{
			std::string result, uuid;
			boost::posix_time::ptime start, end;
			boost::posix_time::time_duration ellapsed;
			int threadId;
			auto file = mFileSend[std::get<0>(res)];
			std::tie(uuid, file->thread, file->start, file->end, file->ellapsed, result) = res;
			file->isEnd = true;
			file->hostname = ns->Hostname();
			CreateOutput(file, result);
			mDone++;
			mFileSend.erase(std::get<0>(res));
			delete file->data;
		}
		boost::unordered_map<std::string, std::vector<unsigned char>*> fileToSend;
		for (int i = 0; i < required; i++)
		{
			MasterFileStatus*  file;
			if ((file = GetFile()) != nullptr)
			{
				auto id = boost::uuids::to_string(mGen());

				file->uuid = id;
				mFileSend[id] = file;
				fileToSend[id] = mOcrFactory.CreateNew()->LoadFile(file, [this] (MasterFileStatus* file)
				{
					this->AddFile(file);
				});
				file->hostname = ns->Hostname();
				onStartProcessFile(file);
			}
		}
		ns->SendSynchro(mThreads.size(), mDone, mSkip, mTotal, mIsEnd, fileToSend);
	}
	catch (std::exception& e)
	{
		std::cout << "ERROR !!! : " << e.what() << std::endl;
	}
}

void Docapost::IA::Tesseract::MasterProcessingWorker::OnSlaveDisconnectHandler(NetworkSession* ns, boost::unordered_map<std::string, bool>& noUsed)
{
	if (ns == nullptr)
		return;
	mSlaves.erase(ns->Hostname());
	std::lock_guard<std::mutex> lock(mNetworkMutex);
	try
	{
		for (auto& res : noUsed)
		{
			if (!res.second)
			{
				AddFile(mFileSend[res.first]);
				onFileCanceled(mFileSend[res.first]);
				mFileSend.erase(res.first);
			}
		}

	}
	catch (std::exception& e)
	{
		std::cout << "ERROR !!! : " << e.what() << std::endl;
	}
}


fs::path Docapost::IA::Tesseract::MasterProcessingWorker::ConstructNewTextFilePath(fs::path path) const
{
	fs::path new_path;

	auto it = mOutputs.find(OutputFlags::Text);
	if (it == mOutputs.end() || it->second == mInput)
	{
		new_path = fs::change_extension(path, ".txt");
	}
	else
	{
		auto relative_path = fs::relative(path, mInput);

		new_path = fs::absolute(relative_path, it->second);
		if (mOutputTypes & OutputFlags::Flattern)
			new_path = new_path.parent_path() / boost::replace_all_copy(fs::change_extension(relative_path, ".txt").string(), "/", mSeparator);
		else
			new_path = new_path.parent_path() / fs::change_extension(new_path.filename(), ".txt");

	}
	return new_path;
}


fs::path Docapost::IA::Tesseract::MasterProcessingWorker::ConstructNewExifFilePath(fs::path path) const
{
	fs::path new_path;

	auto relative_path = fs::relative(path, mInput);
	auto it = mOutputs.find(OutputFlags::Exif);
	if (it == mOutputs.end() || it->second == mInput)
	{
		return path;
	}
	else
	{
		new_path = fs::absolute(relative_path, it->second);

		if (mOutputTypes & OutputFlags::Flattern)
			new_path = new_path.parent_path() / boost::replace_all_copy(relative_path.string(), "/", mSeparator);
		else
			new_path = new_path.parent_path() / new_path.filename();

	}
	return new_path;
}

bool Docapost::IA::Tesseract::MasterProcessingWorker::FileExist(fs::path path) const
{
	auto new_path = ConstructNewTextFilePath(path);

	if (fs::exists(new_path))
	{
		return true;
	}
	return false;
}

bool Docapost::IA::Tesseract::MasterProcessingWorker::ExifExist(fs::path path) const
{
	auto new_path = ConstructNewExifFilePath(path);

	if (!fs::exists(new_path))
	{
		return false;
	}

	Exiv2::Image::AutoPtr o_image = Exiv2::ImageFactory::open(new_path.string());
	o_image->readMetadata();
	Exiv2::ExifData &exifData = o_image->exifData();

	Exiv2::ExifKey key("Exif.Image.ProcessingSoftware");
	Exiv2::ExifData::iterator pos = exifData.findKey(key);
	if (pos != exifData.end() && pos->value().toString() == mSoftName)
	{
		return true;
	}
	return false;
}

string Docapost::IA::Tesseract::MasterProcessingWorker::CreatePdfOutputPath(fs::path path, int i)
{
	return (boost::format("%s/%s-%d.jpg") % path.parent_path().string() % fs::change_extension(path.filename(), "").string() % i).str();
}

void Docapost::IA::Tesseract::MasterProcessingWorker::_AddFolder(fs::path folder, bool resume)
{
	if (!fs::is_directory(folder))
	{
		return;
	}

	for (auto& entry : boost::make_iterator_range(fs::directory_iterator(folder), {}))
	{
		fs::path path = entry.path();
		if (!path.filename_is_dot() && !path.filename_is_dot_dot())
		{
			if (fs::is_directory(path))
			{
				this->_AddFolder(path, resume);
			}
			else
			{
				auto ext = path.extension().string();
				if (extensions.count(ext))
				{

					if (ext == ".pdf")
					{
						std::vector<MasterFileStatus*>* siblings = nullptr;
						std::mutex* mutex_siblings = new std::mutex();
						try
						{
							PoDoFo::PdfMemDocument document;
							document.Load(path.string().c_str());
							//PoDoFo::PdfStreamedDocument document(path.string().c_str());
							auto nbPages = document.GetPageCount();

							siblings = new std::vector<MasterFileStatus*>(nbPages);

							bool added = false;
							for (int i = 0; i < nbPages; i++)
							{
								auto output_file = CreatePdfOutputPath(path, i);

								bool toProcess = true;
								if (resume)
								{
									toProcess = false;
									if (mOutputTypes & OutputFlags::Text)
									{
										toProcess = toProcess || !FileExist(output_file);
									}

									if (mOutputTypes & OutputFlags::Exif)
									{
										toProcess = toProcess || !ExifExist(output_file);
									}

									if (!toProcess)
									{
										mSkip++;
									}
								}

								if (toProcess)
								{
									mTotal++;
									auto file = new MasterFileStatus(path.string(), fs::relative(path, mInput).string(), output_file);
									file->filePosition = i;
									(*siblings)[i] = file;
									file->siblings = siblings;
									file->mutex_siblings = mutex_siblings;
									if (added == false)
									{
										added = true;
										AddFileBack(file);
									}
								}
							}
							if (added == false)
							{
								delete mutex_siblings;
								delete siblings;
							}
						}
						catch (PoDoFo::PdfError& e)
						{
							std::cerr << "Impossible de decoder le fichier " << path.string() << " fallback sur ImageMagic| " << e.ErrorMessage(e.GetError()) << std::endl;

							if (siblings != nullptr)
								delete siblings;

							try
							{
								Magick::ReadOptions options;
								options.density(Magick::Geometry(10, 10));

								std::list<Magick::Image> images;
								Magick::readImages(&images, path.string(), options);

								size_t i;
								std::list<Magick::Image>::iterator image;
								siblings = new std::vector<MasterFileStatus*>(images.size());
								bool insert = false;
								for (image = images.begin(), i = 0; image != images.end(); image++, i++)
								{
									auto output_file = (boost::format("%s/%s-%d.jpg") % path.parent_path().string() % fs::change_extension(path.filename(), "").string() % i).str();

									bool toProcess = true;
									if (resume)
									{
										toProcess = false;
										if (mOutputTypes & OutputFlags::Text)
										{
											toProcess = toProcess || !FileExist(output_file);
										}

										if (mOutputTypes & OutputFlags::Exif)
										{
											toProcess = toProcess || !ExifExist(output_file);
										}

										if (!toProcess)
										{
											mSkip++;
										}
									}

									if (toProcess)
									{
										mTotal++;
										auto file = new MasterFileStatus(path.string(), fs::relative(path, mInput).string(), output_file);
										file->filePosition = i;
										(*siblings)[i] = file;
										file->siblings = siblings;
										file->mutex_siblings = mutex_siblings;
										if (!insert)
										{
											insert = true;
											AddFileBack(file);
										}
									}
								}
								if (insert == false)
								{
									delete mutex_siblings;
									delete siblings;
								}
							}
							catch (...)
							{
								auto eptr = std::current_exception();
								auto n = eptr.__cxa_exception_type()->name();
								std::cerr << "Impossible de decoder le fichier, le fichier ne sera pas inclue pour traitement " << path.string() << " | " << n << std::endl;
							}
						}
						catch (...)
						{
							auto eptr = std::current_exception();
							auto n = eptr.__cxa_exception_type()->name();
							std::cerr << "Impossible de decoder le fichier, le fichier ne sera pas inclue pour traitement " << path.string() << " | " << n << std::endl;
						}
					}
					else
					{
						bool toProcess = true;
						if (resume)
						{
							toProcess = false;
							if (mOutputTypes & OutputFlags::Text)
							{
								toProcess = toProcess || !FileExist(path);
							}

							if (mOutputTypes & OutputFlags::Exif)
							{
								toProcess = toProcess || !ExifExist(path);
							}

							if (!toProcess)
							{
								mSkip++;
							}
						}
						if (toProcess)
						{
							mTotal++;
							AddFileBack(new MasterFileStatus(path.string(), fs::relative(path, mInput).string()));
						}
					}
				}
			}
		}
	}
}
void Docapost::IA::Tesseract::MasterProcessingWorker::AddFolder(fs::path folder, bool resume)
{
	std::thread([this, folder, resume]()
	{
		CatchAllErrorSignals();
		if (mFiles.size() > 0)
			return;
		mInput = folder;
		_AddFolder(folder, resume);
		mIsEnd = true;
	}).detach();
}

std::thread* Docapost::IA::Tesseract::MasterProcessingWorker::Run(int nbThread)
{
	mStart = boost::posix_time::second_clock::local_time();

	for (const auto& kv : mOutputs) {
		fs::create_directories(kv.second);
	};
	for (int i = 0; i < nbThread; i++)
	{
		int id = mNextId++;
		mThreads[id] = new std::thread(&MasterProcessingWorker::ThreadLoop, this, id);
	}
	return nullptr;
}

void Docapost::IA::Tesseract::MasterProcessingWorker::CreateOutput(MasterFileStatus* file, std::string outText)
{
	fs::path new_path;
	if (mOutputTypes & OutputFlags::Text)
	{
		new_path = ConstructNewTextFilePath(file->new_name);

		fs::create_directories(new_path.parent_path());

		std::ofstream output;
		output.open(new_path.string(), std::ofstream::trunc | std::ofstream::out);
		output << outText;
		output.close();

		file->output.push_back(new_path.string());
		file->relative_output.push_back(fs::relative(new_path, mOutputs[OutputFlags::Text]).string());
	}

	if (mOutputTypes & OutputFlags::Exif)
	{

		Exiv2::ExifData exifData;
		auto v = Exiv2::Value::create(Exiv2::asciiString);
		v->read(outText);
		Exiv2::ExifKey key("Exif.Image.ImageDescription");
		exifData.add(key, v.get());
		exifData["Exif.Image.ImageID"] = fs::relative(file->name, mInput).string();
		exifData["Exif.Image.ProcessingSoftware"] = mSoftName;

		auto o_image = Exiv2::ImageFactory::open(file->data->data(), file->fileSize);
		o_image->setExifData(exifData);
		o_image->writeMetadata();
		auto buffersize = o_image->io().size();
		o_image->io().seek(0, Exiv2::BasicIo::beg);
		auto exif_data = o_image->io().read(buffersize);

		new_path = ConstructNewExifFilePath(file->new_name);
		auto it = mOutputs.find(OutputFlags::Exif);
		if (it != mOutputs.end() && it->second != mInput)
		{
			fs::create_directories(new_path.parent_path());
			if (fs::exists(new_path))
			{
				fs::remove(new_path);
			}
		}
		std::ofstream stream(new_path.string());
		stream.write((char*)exif_data.pData_, buffersize);


		file->output.push_back(new_path.string());
		file->relative_output.push_back(fs::relative(new_path, mOutputs[OutputFlags::Exif]).string());
	}
}

void Docapost::IA::Tesseract::MasterProcessingWorker::ThreadLoop(int id)
{
	CatchAllErrorSignals();

	std::shared_ptr<BaseOcr> ocr;
	try
	{
		ocr = mOcrFactory.CreateNew();
	}
	catch(UninitializedOcrException& e)
	{
		std::cerr << "Thread " << id << " - " << e.message() << std::endl;
	}

	while (ocr != nullptr)
	{
		try
		{
			MasterFileStatus * file = GetFile();
			if (file == nullptr)
			{
				{
					boost::lock_guard<std::mutex> lock(mThreadMutex);
					if (mNbThreadToStop > 0)
					{
						--mNbThreadToStop;
						//mThreads.erase(id);

						break;
					}
				}
				if (mDone == mTotal && mFileSend.size() == 0 && mIsEnd)
				{
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(300));
				continue;
			}
			file->uuid = "";
			file->thread = id;
			onStartProcessFile(file);
			file->start = boost::posix_time::microsec_clock::local_time();

			if(nullptr == ocr->LoadFile(file, [this](MasterFileStatus* file)
			{
				this->AddFile(file);
			}))
			{
				AddFileBack(file);
				continue;
			}

			
			if (!ocr->ProcessThroughOcr(file->data, file->result))
			{
				AddFileBack(file);
				continue;
			}

			CreateOutput(file, file->result);

			if (file->filePosition >= 0)
			{
				std::lock_guard<std::mutex>(*file->mutex_siblings);
				file->isCompleted = true;
				bool completed = true;
				for (int i = 0; i < file->siblings->size(); i++)
				{
					auto item = (*file->siblings)[i];
					if (item != nullptr)
					{
						if (!item->isCompleted)
						{
							completed = false;
							break;
						}
					}
				}

				if (completed)
				{
					auto new_path = ConstructNewTextFilePath(file->name);
					std::ofstream stream{ new_path.string(), std::ios::out | std::ios::trunc };

					for (int i = 0; i < file->siblings->size(); i++)
					{
						auto item = (*file->siblings)[i];
						if (item != nullptr)
						{
							stream << item->result << std::endl;
						}
						else
						{
							auto pdf_path = CreatePdfOutputPath(file->name, i);
							auto output_file = ConstructNewTextFilePath(pdf_path);
							if (!fs::exists(output_file))
							{
								stream.close();
								fs::remove(new_path);
								break;
							}
							std::ifstream ifile(output_file.string(), std::ios::in);
							stream << ifile.rdbuf() << std::endl;
						}
					}
				}
			}
			else
			{
				file->isCompleted = true;
			}

			delete file->data;

			mDone++;

			file->end = boost::posix_time::microsec_clock::local_time();
			file->ellapsed = file->end - file->start;
			file->isEnd = true;


			boost::lock_guard<std::mutex> lock(mThreadMutex);
			if (mNbThreadToStop > 0) 
			{
				--mNbThreadToStop;
				//mThreads.erase(id);

				break;
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Thread - " << id << " " << e.what();
		}
		catch (...)
		{
			auto eptr = std::current_exception();
			auto n = eptr.__cxa_exception_type()->name();
			std::cerr << "Thread - " << id << " | " << n << std::endl;
		}
	}

	try
	{
		// Destroy Ocr class
		std::cerr << "Thread end api " << id << std::endl;
		ocr.reset();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Thread - " << id << " " << e.what();
	}
	catch (...)
	{
		auto eptr = std::current_exception();
		auto n = eptr.__cxa_exception_type()->name();
		std::cerr << "Thread - " << id << " | " << n << std::endl;
	}

	TerminateThread(id);
}

void Docapost::IA::Tesseract::MasterProcessingWorker::TerminateThread(int id)
{
	boost::lock_guard<std::mutex> lock(mThreadMutex);

	mThreads[id]->detach();
	delete mThreads[id];
	mThreads.erase(id);

	std::cerr << "Thread remove from list " << id << std::endl;

	if (mThreads.size() == 0)
	{
		while (mFileSend.size() > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
		}
		mEnd = boost::posix_time::second_clock::local_time();
		mNetwork->Stop();
		mNetworkThread->join();
		delete mNetworkThread;
		onProcessEnd();

		std::unique_lock<std::mutex> lk(mIsWorkDoneMutex);
		mIsWorkDone.notify_all();
	}
}

void Docapost::IA::Tesseract::MasterProcessingWorker::SetOutput(boost::unordered_map<OutputFlags, fs::path> folders)
{
	this->mOutputs = folders;
}

Docapost::IA::Tesseract::MasterProcessingWorker::~MasterProcessingWorker()
{
	mNetwork.reset();
}