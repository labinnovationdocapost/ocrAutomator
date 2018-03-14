# Tesseract Automator API
## C/C++
Class : OcrAutomatorMaster

##### **OcrAutomatorMaster**()
Constructeur par d�faut

##### **~OcrAutomatorMaster**()
Destructeur par d�faut

##### std\::vector<std\::string> **GetOcrs**()
Liste l'ensemble des moteur d'OCR disponible sur cette instance
- *return*: liste des noms des moteur d'ocr disponible

##### std\::list<calculated_struct*> **GetOcrParametersDefinition**(std::string ocr);
Liste l'ensemble des propri�t� d'un moteur d'OCR
- *ocr*: nom de l'OCR
- *return*: liste des propi�t�, avec si ce son des enum, l'ensemble des valeurs possibles

##### void **CreateFactory**(std\::string ocr)
cr�er une instance du moteur d'OCR
- *ocr*: nom de l'OCR

##### void **SetFactoryProperty**(std\::string prop, std\::string param)
attribue une valeur � une des propri�t� au moteur d'OCR cr�er par [CreateFactory]()
- *prop*: nom de la propri�t�
- *param*: valeur a attribuer

##### void **SetFactoryProperty**(std\::string prop, int param)
attribue une valeur � une des propri�t�s au moteur d'OCR cr�er par [CreateFactory]()
- *prop*: nom de la propri�t�
- *param*: valeur a attribuer

##### std::string **GetFactoryProperty**(std\::string prop)
r�cup�re la valeur d'une des propri�t�s du moteur d'OCR cr�er par [CreateFactory]()
- *prop*: nom de la propri�t�
- *return*: valeur de la propri�t�

##### void **QuickSetup**(std\::string ocr, std\::unordered_map<std\::string, std\::string> parametersDefinition, Docapost\::IA\::Tesseract\::OutputFlags type = Docapost\::IA\::Tesseract\::OutputFlags\::None, int port = 12000)
cr�er une instance du moteur OcrAutomator et configure le moteur d'OCR
- *ocr*: moteur d'OCR a utiliser
- *parametersDefinition*: liste des propri�t� a d�finir avec leurs valeurs
- *type*: Mode de fonctionnement du moteur OcrAutomator
- *port*: Port r�seau pour la connexion entre le maitre et les esclaves

##### void **CreateInstance**(Docapost\::IA\::Tesseract\::OutputFlags type = Docapost\::IA\::Tesseract\::OutputFlags\::None, int port = 12000)
cr�er une instance du moteur OcrAutomator avec le moteur d'OCR d�finit par [CreateFactory]()
- *type*: Mode de fonctionnement du moteur OcrAutomator
- *port*: Port r�seau pour la connexion entre le maitre et les esclaves

##### void **AddImage**(std\::string id, char* img, int len, std\::string& s_uid)
Envoi une image au moteur OcrAutomator pour traitement
- *id*: nom de l'image
- *img*: binaire de l'image
- *len*: taille du binaire de l'image
- *s_uid*[Out]: uid unique attribu� a l'image

##### int **AddPdf**(std\::string id, char* img, int len, std\::vector<std\::string>& s_uids)
Envoi un pdf au moteur OcrAutomator pour traitement
- *id*: nom du pdf
- *img*: binaire du pdf
- *len*: taille du binaire du pdf
- *s_uids*[Out]: uids unique attribu� au images apr�s conversion du PDF
- *return*: nombre de page du pdf

##### void Start(int maxThread, std::function<void(OcrResult*)> callback)
## HTTP
**GET**: http://<ip>:8888/

Recup�re les informations de l'application au format JSON : ```{"Total":1358,"Done":1236,"Remote":0,"Local":5}```
  - Total: Nombre de fichier total a traiter
  - Done: Nombre de fichier traiter depuis le lancement de l'application
  - Remote: Nombre de thread disponible sur des instances d�port�es de l'applications 
  - Local: Nombre de thread disponible localement sur la machine

*Parametres:*
- Aucun

**POST**: http://<ip>:8888/

Envoi un document au format **JPEG** (.jpeg, .jpg), **PNG** (.png), **TIFF** (.tiff, .tif) ou **PDF** (.pdf). Le document sera trait� par le moteur OCR configur� au lancement de l'application. Le retour est un fichier ZIP (.zip, MIME: application/zip) 

*Parametres:*
- image : un fichier au format JPEG, PNG, TIFF ou PDF avec l'extension correspondante (.jpeg, .jpg, .png, .tiff, .tif, .pdf). Ce param�tre peut �tre fournit a plusieur reprise durant le m�me appel, tous les fichiers seront pris en compte.