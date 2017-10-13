# Tesseract Automator


Le projet est compilé grâce à Visual Studio for Linux Development (inclus dans l'installation de Visual Studio 2017)


## Installation
- Suivre le tutoriel suivant : https://blogs.msdn.microsoft.com/vcblog/2016/03/30/visual-c-for-linux-development/
- Si vous voulez avoir l'auto complétion, copiez les fichiers sources de `/user/include/*` vers le dossier Includes de la solution

>Pour l'utilisation avec WSL (Windows Subsystem for Linux)
- Suivre le tutoriel : https://blogs.msdn.microsoft.com/vcblog/2017/02/08/targeting-windows-subsystem-for-linux-from-visual-studio/

## Configuration de Linux
- Installer le package Tesseract 4.0 LSTM : https://launchpad.net/~alex-p/+archive/ubuntu/tesseract-ocr
- Installer les package libtesseract-dev, libleptonica-dev, libncurses5-dev et libexiv2-dev
- Télécharger Boost : `wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.bz2` puis extraire dans `/root/boost_1_65_1` avec `tar --bzip2 -xf /path/to/boost_1_65_1.tar.bz2`.
- puis suivre la procédure : (http://www.boost.org/doc/libs/1_65_1/more/getting_started/unix-variants.html)
- Si vous souhaitez utiliser les lib static de Boost, enlever les .so générés précédemments

## Utilisation
```Usage:
./TesseractAutomator --help
./TesseractAutomator [options...] /folder/of/images
./TesseractAutomator [options...] --folder /folder/of/images [options...]

Options:
  --psm NUM (=3)            Page Segmentation Mode
  --oem NUM (=3)            Ocr Engine Mode
  -l [ --lang ] LANG (=fra) Langue utilis pour l'OCR
  -h [ --help ]
  -p [ --parallel ] NUM     Nombre de threads en parralle
  -o [ --output ] DOSSIER   Dossier de sortie (dfaut: dossier actuel)
  -c [ --continue ]         Ne pas ecraser les fichiers existant
  -n [ --nodisplay ]        Ne pas afficher l'interface
  -e [ --exif ]             Copier l'image dans le fichier de sortie et crire
                            le rsulat dans les Exif
  -t [ --text ]             Ecrire le rsultat dans un fichier texte (.txt)
                            dans le dossier de sortie
  -f [ --folder ] DOSSIER


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