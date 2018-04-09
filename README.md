
# Tesseract Automator


Le projet a été testé sur Visual Studio (Windows) et GCC (linux/Ubuntu 16.04)

## Installation
- Utiliser CMake (https://cmake.org/) pour généré le projet en fonction de votre environnement et IDE
## Dépendances:
- MuPDF 1.12
- JpegTurbo 1.4.0
- RapidJson
- A2IA (Windows uniquement)
- Exiv2 0.25
- Zlib 1.2.11
- RTTR 0.9.5
- Archive 3.3.2
- Tesseract 4.00.00
- Leptonica 1.74
## Configuration pour compiler un ELF (Linux)
- Installer le package Tesseract 4.0 LSTM : https://launchpad.net/~alex-p/+archive/ubuntu/tesseract-ocr
- Installer les package libtesseract-dev, tesseract-ocr-fra, libleptonica-dev, libncurses5-dev, libexiv2-dev, libmagick++-dev, libprotobuf-dev et libarchive `sudo apt install libtesseract-dev libtesseract-ocr-fra libleptonica-dev libncurses5-dev libexiv2-dev libmagick++-dev libprotobuf-dev libarchive-dev`
- Télécharger Boost : (https://dl.bintray.com/boostorg/release/)
	- puis suivre la procédure : (http://www.boost.org/doc/libs/1_66_0/more/getting_started/unix-variants.html)
	- Si vous souhaitez utiliser les lib static de Boost, enlever les .so générés précédemments
- Télécharge MuPDF: `wget https://mupdf.com/downloads/mupdf-1.12.0-source.tar.gz -o mupdf-1.12.0-source.tar.gz`
	- puis suivre la procédure : (https://mupdf.com/docs/building.html)
- Télécharge RTTR : `wget http://www.rttr.org/releases/rttr-0.9.5-src.tar.gz -o rttr-0.9.5-src.tar.gz` puis extraire avec `tar -xf /path/to/rttr-0.9.5-src.tar.gz`.
	- puis suivre la procédure : (http://www.rttr.org/doc/master/building_install_page.html)
	- Pour compiler une lib static : ```cmake -DBUILD_STATIC=ON```
	- Copier le contenu du dossier ```install``` dans ```/user/local``` (pincipalement ```lib``` et ```include```)
- Télécharge RapidJson: `wget https://github.com/Tencent/rapidjson/archive/v1.1.0.zip -o rapidjson-1.1.0.tar.gz` puis extraire avec `tar -xf /path/to/rapidjson-1.1.0.tar.gz`.
	- Suivez la procédure : https://github.com/Tencent/rapidjson (```cmake .; make install```)

## Configuration pour créer un .deb et le deploiyer (Linux)
- Copier le fichier Env/env-example.config vers Env/env.config et renseiger les informations de connexion
- Compiler le projet
- Executer le script CreatePackage.sh
- Executer le script Deploy.sh

## Configuration pour compiler un EXE (Windows)
- Instaler les outils nécéssaire et ajouter les a votre PATH
  - cmake (https://cmake.org/)
  - VCPKG (ou un autre Package Manager)
  - Visual studio 2017 ou ultérieur
- Tesseract
  - Suivez la procédure de génération du projet
    - Lancer **cmake**
	  - Spécifié votre toolchain si besoin: ```Specify toolchain file for cross-compiling``` pour la liaison des dépendances si vous en avez une (Avec VCPKG par exemple)
      - Entrer le chemin vers le code source de tesseract et le chemin de sortie de la génération 
      - Bouton ```Configure```
      - Si Leptonica a bien été installer la propriété **Leptonica_DIR** est préremplie
      - Vous pouvez decocher la propriété **BUILD_TRAINING_TOOLS**
      - Bouton ```Generate```
  - Ouvre le projet avec Visual Studio et compiler le.
  - installer le (INSTALL), un dossier ```c:\programmes\tesseract``` doit être créer
  - Si le chemin de vers la lib tesseract contient un espace (ex: c:\programme files\tesseract)
	  - modifier le fichier cmake/TesseractConfig.cmake
	  - Ligne 37 ajouter un backslash ```\``` devants chaque espace du chemin
	  - Exemple : ``C:/Program Files/tesseract/include;C:/Program Files/tesseract/include/tesseract`` vers ``C:/Program\ Files/tesseract/include;C:/Program\ Files/tesseract/include/tesseract``
  - Créer un dossier ```c:\programmes\tesseract\tessdata```
  - Télécharger dans ce dossier vos packs de langues (https://github.com/tesseract-ocr/tesseract/wiki/Data-Files)

## Utilisation
```Usage:
./TesseractAutomator --help
./TesseractAutomator [options...] /folder/of/images
./TesseractAutomator [options...] --input /folder/of/images [options...]

Options:
  -v [ --version ]      Affiche le numero de version de l'application
  -h [ --help ]         Affiche l'aide


Communes:
  -p [ --parallel ] NUM (=2) Nombre de threads en parrallele
  -s [ --silent ]            Ne pas afficher l'interface
  --port PORT (=12000)       Utiliser le port reseau definit pour toute
                             communication


Master:
  --psm NUM (=3)                        Page Segmentation Mode
  --oem NUM (=3)                        Ocr Engine Mode
  -l [ --lang ] LANG (=fra)             Langue utilise pour l'OCR
  -o [ --output ] DOSSIER (=/mnt/f/Docs/Visual Studio 2017/Projects/LinuxTesseract/TesseractAutomator/bin/x64/Release)
                                        Dossier de sortie (defaut: dossier
                                        actuel)
  -c [ --continue ]                     le fichier (ou la page pour le PDF)
                                        n'est pas traite si le fichier text
                                        et/ou l'exif existe deja
  -e [ --exif ] DOSSIER                 Copier l'image dans le fichier de
                                        sortie et crire le resulat dans les
                                        Exif. Si non spcifi le paramtre
                                        --output est utilis
  -t [ --text ] DOSSIER                 Ecrire le resultat dans un fichier
                                        texte (.txt) dans le dossier de sortie.
                                        Si non spcifi le paramtre --output
                                        est utilis
  -f [ --prefixe ] [=SEPARATOR(=__)] (=__)
                                        Ajout le chemin relatif a [input] en
                                        prefixe du fichier.Defaut: __
  -i [ --input ] DOSSIER                Dossier d'entree a partir de laquelle
                                        seront listee les fichiers a traiter


Slave:
  -a [ --slave ]        Le programme agira comme un noeud de calcul et
                        cherchera a ce connecter a un noeud maitre disponible
                        pour rcuprer des images a traiter


Information sur --exif et --text
Si aucun dossier n'est specifie le dossier utilise sera celui dfini par --output.
Si --output n'est pas definit, le dossier de sortie sera le dossier courrant.
Exemple:
./TesseractAutomator --input /folder/of/images -et --output /output/folder
./TesseractAutomator --input /folder/of/images -e --text /text/output/folder --output /output/folder
./TesseractAutomator --input /folder/of/images --exif /image/output/folder --text /text/output/folder


Page segmentation modes:
  0    Orientation and script detection (OSD) only.
  1    Automatic page segmentation with OSD.
  2    Automatic page segmentation, but no OSD, or OCR.
  3    Fully automatic page segmentation, but no OSD. (Default)
  4    Assume a single column of text of variable sizes.
  5    Assume a single uniform block of vertically aligned text.
  6    Assume a single uniform block of text.
  7    Treat the image as a single text line.
  8    Treat the image as a single word.
  9    Treat the image as a single word in a circle.
 10    Treat the image as a single character.
 11    Sparse text. Find as much text as possible in no particular order.
 12    Sparse text with OSD.
 13    Raw line. Treat the image as a single text line,
                        bypassing hacks that are Tesseract-specific.
OCR Engine modes:
  0    Original Tesseract only.
  1    Neural nets LSTM only.
  2    Tesseract + LSTM.
  3    Default, based on what is available.
```

Pour plus d'info :
- `TesseractAutomator --help`
