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
/**
 * \brief Wrapper autour de MasterProcessingWorker permetant de masquer l'ensemble des libraries utilisé pour limité les dépendances
 */
class SHARED_EXPORT OcrAutomatorMaster
{
public:
	/**
	 * \brief Constructeur par défaut
	 */
	OcrAutomatorMaster();
	/**
	 * \brief Destructeur par défaut
	 */
	~OcrAutomatorMaster();

	/**
	 * \brief Liste l'ensemble des moteur d'OCR disponible sur cette instance
	 * \return liste des noms des moteur d'ocr disponible
	 */
	std::vector<std::string> GetOcrs();
	/**
	 * \brief Liste l'ensemble des propriété d'un moteur d'OCR
	 * \param ocr nom de l'OCR
	 * \return liste des propiété, avec si ce son des enum, l'ensemble des valeurs possibles
	 */
	std::list<calculated_struct*> GetOcrParametersDefinition(std::string ocr);
	/**
	 * \brief créer une instance du moteur d'OCR
	 * \param ocr nom de l'OCR
	 */
	void CreateFactory(std::string ocr);
	/**
	 * \brief attribue une valeur à une des propriétés au moteur d'OCR créer par CreateFactory
	 * \tparam T 
	 * \param prop nom de la propriété
	 * \param param valeur a attribuer
	 */
	template<typename T>
	void SetFactoryProperty(std::string prop, T param);
	/**
	 * \brief attribue une valeur à une des propriétés au moteur d'OCR créer par CreateFactory
	 * \param prop nom de la propriété
	 * \param param valeur a attribuer
	 */
	void SetFactoryProperty(std::string prop, std::string param);
	/**
	 * \brief attribue une valeur à une des propriétés au moteur d'OCR créer par CreateFactory
	 * \param prop nom de la propriété
	 * \param param valeur a attribuer
	 */
	void SetFactoryProperty(std::string prop, int param);
	/**
	 * \brief récupère la valeur d'une des propriétés du moteur d'OCR créer par CreateFactory
	 * \param prop nom de la propriété
	 * \return valeur de la propriété
	 */
	std::string GetFactoryProperty(std::string prop);
	/**
	 * \brief créer une instance du moteur OcrAutomator et configure le moteur d'OCR
	 * \param ocr moteur d'OCR a utiliser
	 * \param parametersDefinition liste des propriété a définir avec leurs valeurs
	 * \param type Mode de fonctionnement du moteur OcrAutomator
	 * \param port Port réseau pour la connexion entre le maitre et les esclaves
	 */
	void QuickSetup(std::string ocr, std::unordered_map<std::string, std::string> parametersDefinition, Docapost::IA::Tesseract::OutputFlags type = Docapost::IA::Tesseract::OutputFlags::None, int port = 12000);
	/**
	 * \brief créer une instance du moteur OcrAutomator avec le moteur d'OCR définit par CreateFactory
	 * \param type Mode de fonctionnement du moteur OcrAutomator
	 * \param port Port réseau pour la connexion entre le maitre et les esclaves
	 */
	void CreateInstance(Docapost::IA::Tesseract::OutputFlags type = Docapost::IA::Tesseract::OutputFlags::None, int port = 12000);
	/**
	 * \brief Envoi une image au moteur OcrAutomator pour traitement
	 * \param id nom de l'image
	 * \param img binaire de l'image
	 * \param len taille du binaire de l'image
	 * \param s_uid uid unique attribué a l'image
	 */
	void AddImage(std::string id, char* img, int len, std::string& s_uid);
	/**
	 * \brief Envoi un pdf au moteur OcrAutomator pour traitement
	 * \param id nom du pdf
	 * \param img binaire du pdf
	 * \param len taille du binaire du pdf
	 * \param s_uids uids unique attribué au images après conversion du PDF
	 * \return nombre de page du pdf
	 */
	int AddPdf(std::string id, char* img, int len, std::vector<std::string>& s_uids);
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

