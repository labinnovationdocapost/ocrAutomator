#pragma once
#include "BaseProcessingWorker.h"


#include <string>
#include <mutex>
#include <thread>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/signals2.hpp>
#include <boost/unordered_map.hpp>
#include "OutputFlags.h"
#include "SlaveState.h"
using std::string;
#include "Network.h"
#include "MasterFileStatus.h"
#include "Export.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using boost::program_options::value;


namespace Docapost {
	namespace IA {
		namespace Tesseract {
			class EXPOSE MasterProcessingWorker : public BaseProcessingWorker<MasterFileStatus> {
			private:
				boost::uuids::basic_random_generator<boost::mt19937> mGen = boost::uuids::basic_random_generator<boost::mt19937>();

				std::mutex mNetworkMutex;


				boost::unordered_map<OutputFlags, fs::path> mOutputs;
				boost::unordered_map<boost::uuids::uuid, std::shared_ptr<SlaveState>> mSlaves;
				boost::unordered_map<boost::uuids::uuid, MasterFileStatus*> mFileSend;


				OutputFlags mOutputTypes;

				fs::path mInput;

				bool mIsTerminated = false;


				std::string mSeparator = "__";

				boost::shared_ptr<Network> mNetwork;

				std::thread* mNetworkThread;
				std::thread* ListingThread;

				void ThreadLoop(int id) override;

				void _AddFolder(fs::path folder, bool resume);
				/**
				 * \brief Determine si un fichier texte de sortie existe
				 * \param path Chemin du document original
				 * \return True si un fichier texte existe
				 */
				bool FileExist(fs::path path)  const;
				/**
				 * \brief Determine si un exif de sortie existe
				 * \param path Chemin du document original
				 * \return True si l'Exif existe
				 */
				bool ExifExist(fs::path path)  const;
				/**
				 * \brief créer le chemin d'origine pour une page d'un document pdf
				 * \param path chemin du pdf original
				 * \param i numéro de la page (Zero-based numbering)
				 * \return le chemin d'origine décrivant le pdf + numéro de page
				 */
				string CreatePdfOutputPath(fs::path path, int i);
				void InitPdfMasterFileStatus(MasterFileStatus* file, std::mutex* mutex_siblings, std::vector<MasterFileStatus*>* siblings, int i);

				/**
				 * \brief Ajout un document PDF au pipeline
				 * \param resume Faut il écraser les fichiers déja présent (false) ou ignorer le travail si il existe déja (true)
				 * \param nbPages Nombre de page du PDF
				 * \param path 
				 */
				int AddPdfFile(bool resume, fs::path path);
				void AddImageFile(bool resume, fs::path path);
				/**
				 * \brief Créer le chemin de sortie (texte) pour le fichier spécifié
				 * \param path Chemin vers le fichier original
				 * \return Chemin vers le fichier de sortie
				 */
				fs::path ConstructNewExifFilePath(fs::path path) const;
				/**
				 * \brief Créer le chemin de sortie (exif) pour le fichier spécifié
				 * \param path Chemin vers le fichier original
				 * \return Chemin vers le fichier de sortie
				 */
				fs::path ConstructNewTextFilePath(fs::path path, std::string ext) const;

				void MergeResult(MasterFileStatus* file);

				std::string Compress(std::string& str, int compressionlevel);
				void CreateOutput(MasterFileStatus* file);
				void FreeBuffers(MasterFileStatus* file, int memoryImage, int memoryText);

				void TerminateThread(int id);

				MasterFileStatus* GetFileSend(boost::uuids::uuid uuid) { std::lock_guard<std::mutex> lock(mNetworkMutex); return mFileSend[uuid]; }
				void AddFileSend(MasterFileStatus* file)
				{
					std::lock_guard<std::mutex> lock(mNetworkMutex);
					if (mFileSend[file->uuid] != nullptr)
					{
						std::cout << "File " << file->uuid << " exist already\n";
					}
					mFileSend[file->uuid] = file;
				}
				void RemoveFileSend(boost::uuids::uuid uuid) { std::lock_guard<std::mutex> lock(mNetworkMutex); mFileSend.erase(uuid); }


				void SendFilesToClient(NetworkSession* ns);
				bool HasFileToSend(std::shared_ptr<SlaveState> slave);

				void OnSlaveConnectHandler(NetworkSession* ns, int thread, std::string hostname);
				void OnSlaveDisconnectHandler(NetworkSession* ns, boost::unordered_map<boost::uuids::uuid, bool>& noUsed);
				void OnSlaveSynchroHandler(NetworkSession* ns, int thread, int required, std::vector<std::tuple<boost::uuids::uuid, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::vector<std::string>*>>& results);
			public:
				static const std::vector<char*> EXIF_FIELD;
				MasterProcessingWorker(OcrFactory& ocr, OutputFlags type = OutputFlags::None, int port = 12000);

				void AddFolder(fs::path folder, bool resume = false);
				std::thread* Run(int nbThread) override;
				void SetOutput(boost::unordered_map<OutputFlags, fs::path> folders);

				int AddPdfFile(std::string id, char* pdf, int len, std::vector<boost::uuids::uuid>& uid);
				void AddImageFile(std::string id, char* image, int len, boost::uuids::uuid& uids);

				std::set<std::string> extensions = { ".tif", ".tiff", ".png", ".jpg", ".jpeg", ".pdf" };

				boost::signals2::signal<void(MasterFileStatus*)> onFileCanceled;
				boost::signals2::signal<void(MasterFileStatus*)> onStartProcessFile;
				boost::signals2::signal<void(MasterFileStatus*)> onEndProcessFile;
				boost::signals2::signal<void()> onProcessEnd;

				bool NetworkEnable() const { return mNetwork != nullptr/* && mNetworkThread != nullptr*/; }
				int Port() const
				{
					if (NetworkEnable())
						return mNetwork->Port();

					return 0;
				}
				void Separator(std::string separator) { this->mSeparator = separator; }
				std::string Separator() const { return mSeparator; }
				OutputFlags OutputTypes() const { return mOutputTypes; }
				fs::path Input() const { return mInput; }
				boost::unordered_map<OutputFlags, fs::path> Output() const { return mOutputs; }
				int TotalRemoteThreads() const
				{
					int total = 0;
					for (auto& slave : mSlaves)
						total += slave.second->NbThread;
					return total;
				}

				boost::unordered_map<boost::uuids::uuid, std::shared_ptr<SlaveState>> Slaves() const { return mSlaves; }

				~MasterProcessingWorker();
			};
		}
	}
}