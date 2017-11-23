#pragma once

#include "BaseTesseractRunner.h"

#include <cstdio>

#include <string>
#include <queue>
#include <iostream>
#include <mutex>
#include <thread>
#include <fstream>
#include <atomic>
#include "FileStatus.h"

#include <boost/program_options.hpp>
#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
using std::string;
#include <tesseract/baseapi.h>
#include "Network.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using boost::program_options::value;


namespace Docapost {
	namespace IA {
		namespace Tesseract {
			enum TesseractOutputFlags
			{
				Flattern = 4,
				Exif = 2,
				Text = 1,
				None = 0
			};
			inline TesseractOutputFlags operator|(TesseractOutputFlags a, TesseractOutputFlags b)
			{
				return static_cast<TesseractOutputFlags>(static_cast<int>(a) | static_cast<int>(b));
			}
			inline TesseractOutputFlags& operator|=(TesseractOutputFlags& a, TesseractOutputFlags b)
			{
				return a = a | b;
			}

			class TesseractRunner : public BaseTesseractRunner<FileStatus> {
			private:
				boost::uuids::basic_random_generator<boost::mt19937> mGen = boost::uuids::basic_random_generator<boost::mt19937>();

				std::mutex mNetworkMutex;


				boost::unordered_map<TesseractOutputFlags, fs::path> mOutputs;
				boost::unordered_map<std::string, int> mSlaves;
				boost::unordered_map<std::string, FileStatus*> mFileSend;


				TesseractOutputFlags mOutputTypes;

				fs::path mInput;



				std::string mSeparator = "__";

				boost::shared_ptr<Network> mNetwork;

				std::thread* mNetworkThread;

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
				fs::path ConstructNewTextFilePath(fs::path path) const;

				bool GetTextFromTesseract(tesseract::TessBaseAPI* api, std::vector<unsigned char>* image, std::string& text);
				void CreateOutput(FileStatus* file, std::string outText);
				std::vector<unsigned char>* OpenFileForLeptonica(FileStatus* file);

				void TerminateThread(int id);


				void OnSlaveConnectHandler(NetworkSession* ns, int thread, std::string hostname);
				void OnSlaveDisconnectHandler(NetworkSession* ns, boost::unordered_map<std::string, bool>& noUsed);
				void OnSlaveSynchroHandler(NetworkSession* ns, int thread, int required, std::vector<std::tuple<std::string, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::string>>& results);
			public:
				explicit TesseractRunner(tesseract::PageSegMode psm, tesseract::OcrEngineMode oem, std::string lang = "fra", TesseractOutputFlags type = TesseractOutputFlags::None);
				TesseractRunner(std::string lang = "fra") : TesseractRunner(tesseract::PSM_OSD_ONLY, tesseract::OcrEngineMode::OEM_TESSERACT_ONLY, lang) {}
				explicit TesseractRunner(tesseract::PageSegMode psm, std::string lang = "fra") : TesseractRunner(psm, tesseract::OcrEngineMode::OEM_TESSERACT_ONLY, lang) {}
				explicit TesseractRunner(tesseract::OcrEngineMode ocr, std::string lang = "fra") : TesseractRunner(tesseract::PSM_OSD_ONLY, ocr, lang) {}

				void AddFolder(fs::path folder, bool resume = false);
				std::thread* Run(int nbThread) override;
				void SetOutput(boost::unordered_map<TesseractOutputFlags, fs::path> folders);

				std::set<std::string> extensions = { ".tif", ".tiff", ".png", ".jpg", ".jpeg", ".pdf" };

				boost::signals2::signal<void(FileStatus*)> onFileCanceled;
				boost::signals2::signal<void(FileStatus*)> onStartProcessFile;
				boost::signals2::signal<void()> onProcessEnd;

				void Separator(std::string separator) { this->mSeparator = separator; }
				std::string Separator() const { return mSeparator; }
				TesseractOutputFlags OutputTypes() const { return mOutputTypes; }
				fs::path Input() const { return mInput; }
				boost::unordered_map<TesseractOutputFlags, fs::path> Output() const { return mOutputs; }
				int TotalRemoteThreads() const
				{
					int total = 0;
					for (auto& slave : mSlaves)
						total += slave.second;
					return total;
				}

				boost::unordered_map<std::string, int> Slaves() const { return mSlaves; }

				~TesseractRunner();
			};
		}
	}
}