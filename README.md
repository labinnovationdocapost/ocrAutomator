
# OCR Automator
[![codecov](https://codecov.io/gh/labinnovationdocapost/ocrAutomator/branch/master/graph/badge.svg)](https://codecov.io/gh/labinnovationdocapost/ocrAutomator)

Le projet a été testé sur Visual Studio (Windows) et GCC (linux/Ubuntu 16.04)

## Installation
- Utiliser CMake (https://cmake.org/) pour générer le projet en fonction de votre environnement et IDE

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

## COnfigure and compile an ELF (Linux)
- Install package Tesseract 4.0 LSTM : https://launchpad.net/~alex-p/+archive/ubuntu/tesseract-ocr
- Install packages libtesseract-dev, tesseract-ocr-fra, libleptonica-dev, libncurses5-dev, libexiv2-dev, libmagick++-dev, libprotobuf-dev et libarchive `sudo apt install libtesseract-dev libtesseract-ocr-fra libleptonica-dev libncurses5-dev libexiv2-dev libmagick++-dev libprotobuf-dev libarchive-dev`
- Download Boost : (https://dl.bintray.com/boostorg/release/)
	- follow the procedure : (http://www.boost.org/doc/libs/1_66_0/more/getting_started/unix-variants.html)
	- if you want to use static boost's static lib, remove .so files generated in /usr/local/lib
- Download MuPDF: `wget https://mupdf.com/downloads/mupdf-1.12.0-source.tar.gz -o mupdf-1.12.0-source.tar.gz`
    - on linux, up to version and 1.13 (higher version have not been tested), Modify source/fitz/colorspace.c line 3243 (https://bugs.ghostscript.com/show_bug.cgi?id=698927)
      - \- if (srcs != dsts)
      - \+ if (srcs != dsts && srcs->to_ccs != NULL)
    - To avoid conflict with jpegturbo, Remove all file in thirdparty/libjpeg except 
      - jinclude.h
      - jmemsys.h
	- follow the procedure : (https://mupdf.com/docs/building.html)
- Download RTTR : `wget http://www.rttr.org/releases/rttr-0.9.5-src.tar.gz -o rttr-0.9.5-src.tar.gz` and extract with `tar -xf /path/to/rttr-0.9.5-src.tar.gz`.
	- follow the procedure : (http://www.rttr.org/doc/master/building_install_page.html)
	- Compile a static lib : ```cmake -DBUILD_STATIC=ON```
	- copy the folder content ```install``` in ```/user/local``` (mainly ```lib``` and ```include```)
- Download RapidJson: `wget https://github.com/Tencent/rapidjson/archive/v1.1.0.zip -o rapidjson-1.1.0.tar.gz` and extract with `tar -xf /path/to/rapidjson-1.1.0.tar.gz`.
	- follow the procedure : https://github.com/Tencent/rapidjson (```cmake .; make install```)
- Jpeg 

## Configuration pour créer un .deb et le deployer (Linux)
- Copier le fichier Env/env-example.config vers Env/env.config et renseigner les informations de connexion
- Compiler le projet
- Executer le script CreatePackage.sh
- Executer le script Deploy.sh

## Configuration pour compiler un .EXE (Windows)
- Installer les outils nécéssaires et ajouter les à votre PATH
  - cmake (https://cmake.org/)
  - VCPKG (ou un autre Package Manager)
  - Visual studio 2017 ou ultérieur
- Tesseract
  - Suivez la procédure de génération du projet
  - Lancer **cmake**
  	- Spécifier votre toolchain si besoin: ```Specify toolchain file for cross-compiling``` pour la liaison des dépendances si vous en avez une (avec VCPKG par exemple)
      	- Entrer le chemin vers le code source de tesseract et le chemin de sortie de la génération 
      	- Bouton ```Configure```
      	- Si Leptonica a bien été installé, la propriété **Leptonica_DIR** est préremplie
      	- Vous pouvez décocher la propriété **BUILD_TRAINING_TOOLS**
      	- Bouton ```Generate```
  - ouvrez le projet avec Visual Studio et compilez le.
  - installez le (INSTALL), un dossier ```c:\programmes\tesseract``` doit être créer
  - si le chemin vers la lib tesseract contient un espace (ex: c:\programme files\tesseract)
	  - modifier le fichier cmake/TesseractConfig.cmake
	  - Ligne 37 ajouter un backslash ```\``` devant chaque espace du chemin
	  - Exemple : ``C:/Program Files/tesseract/include;C:/Program Files/tesseract/include/tesseract`` vers ``C:/Program\ Files/tesseract/include;C:/Program\ Files/tesseract/include/tesseract``
  - Créer un dossier ```c:\programmes\tesseract\tessdata```
  - Télécharger dans ce dossier vos packs de langues (https://github.com/tesseract-ocr/tesseract/wiki/Data-Files)

## Utilisation
```Usage:
./OcrAutomator --help
./OcrAutomator [options...] /folder/of/images
./OcrAutomator [options...] --input /folder/of/images [options...]

Options:
  -v [ --version ]      Affiche le numéro de version de l'application
  -h [ --help ]         Affiche l'aide


Communes:
  -p [ --parallel ] NUM (=2) Nombre de threads en parallèle
  -s [ --silent ]            Ne pas afficher l'interface
  --port PORT (=12000)       Utiliser le port reseau définit pour toute
                             communication


Master:
  --psm NUM (=3)                        Page Segmentation Mode
  --oem NUM (=3)                        Ocr Engine Mode
  -l [ --lang ] LANG (=fra)             Langue utilisée pour l'OCR
  -o [ --output ] DOSSIER (=/mnt/f/Docs/Visual Studio 2017/Projects/LinuxTesseract/TesseractAutomator/bin/x64/Release)
                                        Dossier de sortie (defaut: dossier
                                        actuel)
  -c [ --continue ]                     le fichier (ou la page pour le PDF)
                                        n'est pas traité si le fichier texte
                                        et/ou l'exif existe deja
  -e [ --exif ] DOSSIER                 Copier l'image dans le fichier de
                                        sortie et écrire le resulat dans les
                                        Exif. Si non spécifié le paramtre
                                        --output est utilisé
  -t [ --text ] DOSSIER                 Ecrire le resultat dans un fichier
                                        texte (.txt) dans le dossier de sortie.
                                        Si non spécifié le paramètre --output
                                        est utilisé
  -f [ --prefixe ] [=SEPARATOR(=__)] (=__)
                                        Ajout le chemin relatif a [input] en
                                        prefixe du fichier.Defaut: __
  -i [ --input ] DOSSIER                Dossier d'entrée à partir duquel
                                        sont listés les fichiers à traiter


Slave:
  -a [ --slave ]        Le programme agira comme un noeud de calcul et
                        cherchera à se connecter à un noeud maitre disponible
                        pour récupérer des images à traiter


Information sur --exif et --text
Si aucun dossier n'est specifié, le dossier utilisé sera celui défini par --output.
Si --output n'est pas défini, le dossier de sortie sera le dossier courant.

Exemple:
./OcrAutomator --input /folder/of/images -et --output /output/folder
./OcrAutomator --input /folder/of/images -e --text /text/output/folder --output /output/folder
./OcrAutomator --input /folder/of/images --exif /image/output/folder --text /text/output/folder


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

Pour plus d'infos :
- `OcrAutomator --help`
