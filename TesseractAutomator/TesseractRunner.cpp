#include "TesseractRunner.h"

#include <leptonica/allheaders.h>
#include <future>
#include <exiv2/exiv2.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <Magick++.h>
#include <boost/format.hpp>
#include <podofo/podofo.h>


Docapost::IA::Tesseract::TesseractRunner::TesseractRunner(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang, Docapost::IA::Tesseract::TesseractOutputFlags types) : 
	BaseTesseractRunner(psm, oem, lang),
	mOutputTypes(types)
{
	setMsgSeverity(L_SEVERITY_NONE);
	PoDoFo::PdfError::EnableDebug(false);
	PoDoFo::PdfError::EnableLogging(false);

	mNetwork = boost::make_shared<Network>(12000);
	mNetwork->onSlaveConnect.connect(boost::bind(&TesseractRunner::OnSlaveConnectHandler, this, _1, _2, _3));

	mNetwork->onSlaveSynchro.connect(boost::bind(&TesseractRunner::OnSlaveSynchroHandler, this, _1, _2, _3, _4));

	mNetwork->onSlaveDisconnect.connect(boost::bind(&TesseractRunner::OnSlaveDisconnectHandler, this, _1, _2));

	mNetwork->InitBroadcastReceiver();
	mNetwork->InitComm();
	mNetworkThread = new std::thread([this]()
	{
		mNetwork->Start();
	});
}

void Docapost::IA::Tesseract::TesseractRunner::OnSlaveConnectHandler(NetworkSession* ns, int thread, std::string hostname)
{
	mSlaves[ns->Hostname()] = thread;
	ns->SendStatus(this->mDone, this->mSkip, this->mTotal, this->mPsm, this->mOem, this->mLang);
}

void Docapost::IA::Tesseract::TesseractRunner::OnSlaveSynchroHandler(NetworkSession* ns, int thread, int required, std::vector<std::tuple<std::string, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::string>>& results)
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
			FileStatus*  file;
			if ((file = GetFile()) != nullptr)
			{
				auto id = boost::uuids::to_string(mGen());

				file->uuid = id;
				mFileSend[id] = file;
				fileToSend[id] = OpenFileForLeptonica(file);
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

void Docapost::IA::Tesseract::TesseractRunner::OnSlaveDisconnectHandler(NetworkSession* ns, boost::unordered_map<std::string, bool>& noUsed)
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


fs::path Docapost::IA::Tesseract::TesseractRunner::ConstructNewTextFilePath(fs::path path) const
{
	fs::path new_path;

	auto it = mOutputs.find(TesseractOutputFlags::Text);
	if (it == mOutputs.end() || it->second == mInput)
	{
		new_path = fs::change_extension(path, ".txt");
	}
	else
	{
		auto relative_path = fs::relative(path, mInput);

		new_path = fs::absolute(relative_path, it->second);
		if (mOutputTypes & TesseractOutputFlags::Flattern)
			new_path = new_path.parent_path() / boost::replace_all_copy(fs::change_extension(relative_path, ".txt").string(), "/", mSeparator);
		else
			new_path = new_path.parent_path() / fs::change_extension(new_path.filename(), ".txt");

	}
	return new_path;
}


fs::path Docapost::IA::Tesseract::TesseractRunner::ConstructNewExifFilePath(fs::path path) const
{
	fs::path new_path;

	auto relative_path = fs::relative(path, mInput);
	auto it = mOutputs.find(TesseractOutputFlags::Exif);
	if (it == mOutputs.end() || it->second == mInput)
	{
		return path;
	}
	else
	{
		new_path = fs::absolute(relative_path, it->second);

		if (mOutputTypes & TesseractOutputFlags::Flattern)
			new_path = new_path.parent_path() / boost::replace_all_copy(relative_path.string(), "/", mSeparator);
		else
			new_path = new_path.parent_path() / new_path.filename();

	}
	return new_path;
}

bool Docapost::IA::Tesseract::TesseractRunner::FileExist(fs::path path) const
{
	auto new_path = ConstructNewTextFilePath(path);

	if (fs::exists(new_path))
	{
		return true;
	}
	return false;
}

bool Docapost::IA::Tesseract::TesseractRunner::ExifExist(fs::path path) const
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

void Docapost::IA::Tesseract::TesseractRunner::_AddFolder(fs::path folder, bool resume)
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
					try
					{

						if (ext == ".pdf")
						{
							PoDoFo::PdfMemDocument document;
							document.Load(path.string().c_str());
							//PoDoFo::PdfStreamedDocument document(path.string().c_str());
							auto nbPages = document.GetPageCount();

							std::vector<FileStatus*>* siblings = new std::vector<FileStatus*>(nbPages);
							std::mutex* mutex_siblings = new std::mutex();
							bool added = false;
							for (int i = 0; i < nbPages; i++)
							{
								auto output_file = (boost::format("%s/%s-%d.jpg") % path.parent_path().string() % fs::change_extension(path.filename(), "").string() % i).str();

								bool toProcess = true;
								if (resume)
								{
									toProcess = false;
									if (mOutputTypes & TesseractOutputFlags::Text)
									{
										toProcess = toProcess || !FileExist(output_file);
									}

									if (mOutputTypes & TesseractOutputFlags::Exif)
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
									auto file = new FileStatus(path.string(), fs::relative(path, mInput).string(), output_file);
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
						}
						else
						{
							bool toProcess = true;
							if (resume)
							{
								toProcess = false;
								if (mOutputTypes & TesseractOutputFlags::Text)
								{
									toProcess = toProcess || !FileExist(path);
								}

								if (mOutputTypes & TesseractOutputFlags::Exif)
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
								AddFileBack(new FileStatus(path.string(), fs::relative(path, mInput).string()));
							}
						}
					}
					catch (PoDoFo::PdfError& e)
					{
						std::cerr << "Impossible de decoder le fichier " << path.string() << " fallback sur ImageMagic| " << e.ErrorMessage(e.GetError()) << std::endl;

						try
						{
							Magick::ReadOptions options;
							options.density(Magick::Geometry(10, 10));

							std::list<Magick::Image> images;
							Magick::readImages(&images, path.string(), options);

							size_t i;
							std::list<Magick::Image>::iterator image;
							std::vector<FileStatus*>* siblings = new std::vector<FileStatus*>(images.size());
							std::mutex* mutex_siblings = new std::mutex();
							bool insert = false;
							for (image = images.begin(), i = 0; image != images.end(); image++, i++)
							{
								auto output_file = (boost::format("%s/%s-%d.jpg") % path.parent_path().string() % fs::change_extension(path.filename(), "").string() % i).str();

								bool toProcess = true;
								if (resume)
								{
									toProcess = false;
									if (mOutputTypes & TesseractOutputFlags::Text)
									{
										toProcess = toProcess || !FileExist(output_file);
									}

									if (mOutputTypes & TesseractOutputFlags::Exif)
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
									auto file = new FileStatus(path.string(), fs::relative(path, mInput).string(), output_file);
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
							if(insert == false)
							{
								delete mutex_siblings;
								delete siblings;
							}
						}
						catch (...)
						{
							auto eptr = std::current_exception();
							auto n = eptr.__cxa_exception_type()->name();
							std::cerr << "Impossible de decoder le fichier " << path.string() << " | " << n << std::endl;
						}
					}
					catch (...)
					{
						auto eptr = std::current_exception();
						auto n = eptr.__cxa_exception_type()->name();
						std::cerr << "Impossible de decoder le fichier " << path.string() << " | " << n << std::endl;
					}
				}
			}
		}
	}
}
void Docapost::IA::Tesseract::TesseractRunner::AddFolder(fs::path folder, bool resume)
{
	new std::thread([this, folder, resume]()
	{
		if (mFiles.size() > 0)
			return;
		mInput = folder;
		_AddFolder(folder, resume);
		mIsEnd = true;
	});
}

std::thread* Docapost::IA::Tesseract::TesseractRunner::Run(int nbThread)
{
	mStart = boost::posix_time::second_clock::local_time();

	for (const auto& kv : mOutputs) {
		fs::create_directories(kv.second);
	};
	for (int i = 0; i < nbThread; i++)
	{
		int id = mNextId++;
		mThreads[id] = new std::thread(&TesseractRunner::ThreadLoop, this, id);
	}
	return nullptr;
}

bool Docapost::IA::Tesseract::TesseractRunner::GetTextFromTesseract(tesseract::TessBaseAPI* api, std::vector<l_uint8>* imgData, std::string& text)
{
	Pix *image = pixReadMem(imgData->data(), imgData->size());
	if (image == nullptr)
	{
		return false;
	}

	api->SetImage(image);
	auto outtext = api->GetUTF8Text();

	text = string(outtext);

	pixDestroy(&image);
	delete[] outtext;

	return true;
}

std::vector<l_uint8> * Docapost::IA::Tesseract::TesseractRunner::OpenFileForLeptonica(FileStatus* file)
{
	if (file->filePosition >= 0)
	{
		std::lock_guard<std::mutex> lock(*file->mutex_siblings);
		if (file->fileSize > 0)
		{
			return file->data;
		}
		Magick::ReadOptions options;
		options.density(Magick::Geometry(250, 250));
		std::list<Magick::Image> images;

		Magick::readImages(&images, file->name, options);

		size_t i;
		std::list<Magick::Image>::iterator image;
		for (image = images.begin(), i = 0; image != images.end(); image++, i++)
		{
			if ((*file->siblings)[i] == nullptr)
				continue;
			Magick::Blob blobOut;
			image->colorSpace(MagickCore::RGBColorspace);
			image->quality(80);
			image->density(Magick::Geometry(300, 300));
			image->resolutionUnits(MagickCore::ResolutionType::PixelsPerInchResolution);
			image->magick("JPEG");
			image->write(&blobOut);
			auto data = (char*)blobOut.data();
			(*file->siblings)[i]->data = new std::vector<l_uint8>(data, data + blobOut.length());
			(*file->siblings)[i]->fileSize = (*file->siblings)[i]->data->size();
			if (file != (*file->siblings)[i])
			{
				AddFile((*file->siblings)[i]);
			}
		}
		return file->data;
	}
	else
	{
		FILE* f = fopen(file->name.c_str(), "r");
		if (f == nullptr)
		{
			std::cerr << "Impossible d'ouvrir le fichier " << file->name << std::endl;
			return nullptr;
		}

		// Determine file size
		fseek(f, 0, SEEK_END);
		size_t size = ftell(f);

		file->data = new std::vector<l_uint8>(size);

		rewind(f);
		fread(file->data->data(), sizeof(l_uint8), size, f);
		fclose(f);

		file->fileSize = file->data->size();

		return file->data;
	}
}

void Docapost::IA::Tesseract::TesseractRunner::CreateOutput(FileStatus* file, std::string outText)
{
	fs::path new_path;
	if (mOutputTypes & TesseractOutputFlags::Text)
	{
		new_path = ConstructNewTextFilePath(file->new_name);

		fs::create_directories(new_path.parent_path());

		std::ofstream output;
		output.open(new_path.string(), std::ofstream::trunc | std::ofstream::out);
		output << outText;
		output.close();

		file->output.push_back(new_path.string());
		file->relative_output.push_back(fs::relative(new_path, mOutputs[TesseractOutputFlags::Text]).string());
	}

	if (mOutputTypes & TesseractOutputFlags::Exif)
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
		auto it = mOutputs.find(TesseractOutputFlags::Exif);
		if (it != mOutputs.end() && it->second != mInput)
		{
			fs::create_directories(new_path.parent_path());
			if (fs::exists(new_path))
			{
				fs::remove(new_path);
			}
			//fs::copy_file(file->name, new_path);
			//std::copy(exif_data.pData_, file->data->end(), std::ostreambuf_iterator<char>(stream));

		}
		std::ofstream stream(new_path.string());
		stream.write((char*)exif_data.pData_, buffersize);


		file->output.push_back(new_path.string());
		file->relative_output.push_back(fs::relative(new_path, mOutputs[TesseractOutputFlags::Exif]).string());
	}
}

void Docapost::IA::Tesseract::TesseractRunner::ThreadLoop(int id)
{
	auto api = new tesseract::TessBaseAPI();
	api->SetVariable("debug_file", "/dev/null");
	api->SetVariable("out", "quiet");
	int res;

	if ((res = api->Init(nullptr, mLang.c_str(), mOem))) {
		std::stringstream cstring;
		cstring << "Thread " << id << " - " << "Could not initialize tesseract\n";
		std::cerr << cstring.str();
		return;
	}
	api->SetPageSegMode(mPsm);

	while (true)
	{
		try
		{
			FileStatus * file = GetFile();
			if (file == nullptr)
			{
				if (mFileSend.size() == 0 && mIsEnd)
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

			if (nullptr == OpenFileForLeptonica(file))
			{
				AddFileBack(file);
				continue;
			}

			std::string outText;
			if (!GetTextFromTesseract(api, file->data, outText))
			{
				AddFileBack(file);
				continue;
			}

			CreateOutput(file, outText);

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
		std::cerr << "Thread end api " << id << std::endl;
		api->End();
		std::cerr << "Thread delete api " << id << std::endl;
		delete api;
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

	std::cerr << "Thread remove from list " << id << std::endl;
	mThreads.erase(id);
	boost::lock_guard<std::mutex> lock(mThreadMutex);
	if (mThreads.size() == 0)
	{
		while (mFileSend.size() > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
		}
		mEnd = boost::posix_time::second_clock::local_time();
		mNetwork->Stop();
		mNetworkThread->join();
		onProcessEnd();
	}
}

void Docapost::IA::Tesseract::TesseractRunner::SetOutput(boost::unordered_map<TesseractOutputFlags, fs::path> folders)
{
	this->mOutputs = folders;
}

Docapost::IA::Tesseract::TesseractRunner::~TesseractRunner()
{
	mNetwork.reset();
}