# Tesseract Automator API
## C/C++
Class : OcrAutomatorMaster

##### **OcrAutomatorMaster**()
Constructeur par défaut

##### **~OcrAutomatorMaster**()
Destructeur par défaut

##### std\::vector<std\::string> **GetOcrs**()
Liste l'ensemble des moteur d'OCR disponible sur cette instance
- *return*: liste des noms des moteur d'ocr disponible

##### std\::list<calculated_struct*> **GetOcrParametersDefinition**(std::string ocr);
Liste l'ensemble des propriété d'un moteur d'OCR
- *ocr*: nom de l'OCR
- *return*: liste des propiété, avec si ce son des enum, l'ensemble des valeurs possibles

##### void **CreateFactory**(std\::string ocr)
créer une instance du moteur d'OCR
- *ocr*: nom de l'OCR

##### void **SetFactoryProperty**(std\::string prop, std\::string param)
attribue une valeur à une des propriété au moteur d'OCR créer par [CreateFactory]()
- *prop*: nom de la propriété
- *param*: valeur a attribuer

##### void **SetFactoryProperty**(std\::string prop, int param)
attribue une valeur à une des propriétés au moteur d'OCR créer par [CreateFactory]()
- *prop*: nom de la propriété
- *param*: valeur a attribuer

##### std::string **GetFactoryProperty**(std\::string prop)
récupère la valeur d'une des propriétés du moteur d'OCR créer par [CreateFactory]()
- *prop*: nom de la propriété
- *return*: valeur de la propriété

##### void **QuickSetup**(std\::string ocr, std\::unordered_map<std\::string, std\::string> parametersDefinition, Docapost\::IA\::Tesseract\::OutputFlags type = Docapost\::IA\::Tesseract\::OutputFlags\::None, int port = 12000)
créer une instance du moteur OcrAutomator et configure le moteur d'OCR
- *ocr*: moteur d'OCR a utiliser
- *parametersDefinition*: liste des propriété a définir avec leurs valeurs
- *type*: Mode de fonctionnement du moteur OcrAutomator
- *port*: Port réseau pour la connexion entre le maitre et les esclaves

##### void **CreateInstance**(Docapost\::IA\::Tesseract\::OutputFlags type = Docapost\::IA\::Tesseract\::OutputFlags\::None, int port = 12000)
créer une instance du moteur OcrAutomator avec le moteur d'OCR définit par [CreateFactory]()
- *type*: Mode de fonctionnement du moteur OcrAutomator
- *port*: Port réseau pour la connexion entre le maitre et les esclaves

##### void **AddImage**(std\::string id, char* img, int len, std\::string& s_uid)
Envoi une image au moteur OcrAutomator pour traitement
- *id*: nom de l'image
- *img*: binaire de l'image
- *len*: taille du binaire de l'image
- *s_uid*[Out]: uid unique attribué a l'image

##### int **AddPdf**(std\::string id, char* img, int len, std\::vector<std\::string>& s_uids)
Envoi un pdf au moteur OcrAutomator pour traitement
- *id*: nom du pdf
- *img*: binaire du pdf
- *len*: taille du binaire du pdf
- *s_uids*[Out]: uids unique attribué au images après conversion du PDF
- *return*: nombre de page du pdf

##### void Start(int maxThread, std::function<void(OcrResult*)> callback)
## HTTP
**GET**: http://<ip>:8888/

Recupère les informations de l'application au format JSON : ```{"Total":1358,"Done":1236,"Remote":0,"Local":5}```
  - Total: Nombre de fichier total a traiter
  - Done: Nombre de fichier traiter depuis le lancement de l'application
  - Remote: Nombre de thread disponible sur des instances déportées de l'applications 
  - Local: Nombre de thread disponible localement sur la machine

*Parametres:*
- Aucun

**POST**: http://<ip>:8888/

Envoi un document au format **JPEG** (.jpeg, .jpg), **PNG** (.png), **TIFF** (.tiff, .tif) ou **PDF** (.pdf). Le document sera traité par le moteur OCR configuré au lancement de l'application. Le retour est un fichier ZIP (.zip, MIME: application/zip) 

*Parametres:*
- image : un fichier au format JPEG, PNG, TIFF ou PDF avec l'extension correspondante (.jpeg, .jpg, .png, .tiff, .tif, .pdf). Ce paramètre peut être fournit a plusieur reprise durant le même appel, tous les fichiers seront pris en compte.