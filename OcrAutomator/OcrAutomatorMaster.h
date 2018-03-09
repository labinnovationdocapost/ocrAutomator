#pragma once
#include <memory>
#include "OutputFlags.h"
#include <unordered_map>
#include <vector>
#include "MemoryFileBuffer.h"

#ifdef _WIN32
#ifdef LIB_EXPORT
#define SHARED_EXPORT __declspec(dllexport)
#else
#define SHARED_EXPORT __declspec(dllimport)
#endif
#else
#define SHARED_EXPORT
#endif

struct calculated_struct
{
public:
	std::string name;
	std::string typeName;
	bool isEnum;
	std::vector<std::string> enu;
};

class SHARED_EXPORT OcrResult
{
private:
	struct Impl;
	std::unique_ptr<Impl> data;
	friend class OcrAutomatorMaster;
public:
	std::string Name();
	std::vector<std::string>* Text();
	Docapost::IA::Tesseract::MemoryFileBuffer* Image();
	OcrResult();
	~OcrResult();
};


 namespace detail
 {
	 
 }
class SHARED_EXPORT OcrAutomatorMaster
{
public:
	OcrAutomatorMaster();
	~OcrAutomatorMaster();

	std::vector<std::string> GetOcrs();
	std::list<calculated_struct*> GetOcrParametersDefinition(std::string ocr);
	void CreateFactory(std::string ocr);
	template<typename T>
	void SetFactoryProperty(std::string prop, T param);
	void SetFactoryProperty(std::string prop, std::string param);
	void SetFactoryProperty(std::string prop, int param);

	std::string GetFactoryProperty(std::string prop);
	void QuickSetup(std::string ocr, std::unordered_map<std::string, std::string> parametersDefinition, Docapost::IA::Tesseract::OutputFlags type = Docapost::IA::Tesseract::OutputFlags::None, int port = 12000);

	void CreateInstance(Docapost::IA::Tesseract::OutputFlags type = Docapost::IA::Tesseract::OutputFlags::None, int port = 12000);
	void AddImage(std::string id, char* img, int len);
	int AddPdf(std::string id, char* img, int len);


	/**
	 * \brief Démarre le système et lance les threads d'OCRization
	 * \param maxThread 
	 * \param callback resultat de l'OCR et de l'extraction de l'image, si Docapost::IA::Tesseract::OutputFlags::MemoryImage est spécifié, il est de ovtre réponsabilité de libérer l'objet
	 */
	void Start(int maxThread, std::function<void(OcrResult*)> callback);
private:

	friend class OcrResult;
	struct Impl;
	std::unique_ptr<Impl> d_ptr;
};

