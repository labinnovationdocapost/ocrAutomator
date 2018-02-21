# Tesseract Automator


Le projet est compilé grâce à Visual Studio for Linux Development (inclus dans l'installation de Visual Studio 2017)

## Build
### Linux
- https://outils.docapost.tech/nexus/#browse/browse/assets:innovation-raw
- https://outils.docapost.tech/recettenexus/service/rest/repository/browse/innovation-apt/
  - Ajoutez le repository : ```sudo add-apt-repository https://outils.docapost.tech/recettenexus/repository/innovation-apt/```
  - Ajoutez les authorisation : ```sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 5E0C99EE```
  - Mettez a jour : ```sudo apt update```
  - Mettez a jour : ```sudo apt install tesseractautomator```
  - Verifier l'instalation avec ```TesseractAutomator -v```

## Installation
- Suivre le tutoriel suivant : https://blogs.msdn.microsoft.com/vcblog/2016/03/30/visual-c-for-linux-development/
- Si vous voulez avoir l'auto complétion, copiez les fichiers sources de `/user/include/*` vers le dossier Includes de la solution

>Pour l'utilisation avec WSL (Windows Subsystem for Linux)
- Suivre le tutoriel : https://blogs.msdn.microsoft.com/vcblog/2017/02/08/targeting-windows-subsystem-for-linux-from-visual-studio/

## Configuration pour compiler un ELF (Linux)
- Installer le package Tesseract 4.0 LSTM : https://launchpad.net/~alex-p/+archive/ubuntu/tesseract-ocr
- Installer les package libtesseract-dev, tesseract-ocr-fra, libleptonica-dev, libncurses5-dev, libexiv2-dev, libmagick++-dev et libprotobuf-dev `sudo apt install libtesseract-dev libtesseract-ocr-fra libleptonica-dev libncurses5-dev libexiv2-dev libmagick++-dev libprotobuf-dev`
- Télécharger Boost : `wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.bz2` puis extraire dans `/root/boost_1_65_1` avec `tar --bzip2 -xf /path/to/boost_1_65_1.tar.bz2`.
- puis suivre la procédure : (http://www.boost.org/doc/libs/1_65_1/more/getting_started/unix-variants.html)
- Si vous souhaitez utiliser les lib static de Boost, enlever les .so générés précédemments
- Télécharge Podofo : `wget https://mupdf.com/downloads/mupdf-1.12.0-source.tar.gz -o mupdf-1.12.0-source.tar.gz` puis extraire avec `tar -xf /path/to/mupdf-1.12.0-source.tar.gz`.
- puis suivre la procédure : (https://mupdf.com/docs/building.html)
- Télécharge Podofo : `wget http://www.rttr.org/releases/rttr-0.9.5-src.tar.gz -o rttr-0.9.5-src.tar.gz` puis extraire avec `tar -xf /path/to/rttr-0.9.5-src.tar.gz`.
- puis suivre la procédure : (http://www.rttr.org/doc/master/building_install_page.html)
- Pour compiler une lib static : ```cmake -DBUILD_STATIC=ON```
- Copier le contenu du dossier ```install``` dans ```/user/local``` (pincipalement ```lib``` et ```include```)

## Configuration pour créer un .deb et le deploiyer (Linux)
- Copier le fichier Env/env-example.config vers Env/env.config et renseiger les informations de connexion
- Compiler le projet
- Executer le script CreatePackage.sh
- Executer le script Deploy.sh

## Configuration pour compiler un EXE (Windows)
- Telecharger les trois librairies suivantes : 
  - leptonica (https://github.com/DanBloomberg/leptonica)
  - tesseract4 LTSM (https://github.com/tesseract-ocr/tesseract)
  - jpegturbo (https://github.com/libjpeg-turbo/libjpeg-turbo)
- Instaler les outils nécéssaire et ajouter les a votre PATH
  - cmake (https://cmake.org/)
  - NASM (http://www.nasm.us/)
  - CPPAN (https://cppan.org/client/)
  - Visual studio 2017
- Leptonica:
  - Suivez la procédure de génération du projet
    - depuis le dossier ou ce situe les sources de leptonica
    - ```cppan .```
    - Lancer **cmake**
      - Entrer le chemin vers le code source de leptonica et le chemin de sortie de la génération 
      - Bouton ```Configure```
      - Modifier la propriété **CPPAN_BUILD_SHARED_LIBS** pour qu'elle soit =0
      - Bouton ```Generate```
  - compiler en 64bits (ALL_BUILD)
  - installer le (INSTALL), un dossier ```c:\programmes\leptonica``` doit être créer
- Tesseract
  - Suivez la procédure de génération du projet
    - Lancer **cmake**
      - Entrer le chemin vers le code source de tesseract et le chemin de sortie de la génération 
      - Bouton ```Configure```
      - Si leptonica a bien été installer la propriété **Leptonica_DIR** est préremplie
      - Vous pouvez decocher la propriété **BUILD_TRAINING_TOOLS**
      - Bouton ```Generate```
  - Ouvrer le projet et modifier ses propriétés
    - Click droit Properties sur libtesseract
    - ```Configuration properties```
    - ```C/C++```
    - ```General```
    - ```Additional Include Directories```
    - Ajouter ```C:\Program Files\leptonica\include```
    - Ajouter ```C:\Program Files\leptonica\include\leptonica```
    - 
    - ```Configuration properties```
    - ```Linker```
    - ```Input```
    - ```Additional Directories```
    - Supprimer toutes les références à pvt.cppan.demo.* 
  - compiler en 64bits (ALL_BUILD)
  - installer le (INSTALL), un dossier ```c:\programmes\tesseract``` doit être créer
  - Créer un dossier ```c:\programmes\tesseract\tessdata```
  - Télécharger dans ce dossier vos packs de langues (https://github.com/tesseract-ocr/tesseract/wiki/Data-Files)
- JpegTurbo
    - Lancer **cmake**
      - Entrer le chemin vers le code source de jpegturbo et le chemin de sortie de la génération 
      - Bouton ```Configure```
      - Modifier la propriété **CMAKE_INSTALL_PREFIX** pour qu'elle soit ```C:\Program Files\libjpeg-turbo64```
      - Bouton ```Generate```
  - compiler en 64bits (ALL_BUILD)
  - installer le (INSTALL), un dossier ```C:\Program Files\libjpeg-turbo64``` doit être créer

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