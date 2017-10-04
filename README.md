# LinuxTesseract


Le projet est compiler grace à Visual Studio for Linux Development (inclu dans l'instalation de Visual Studio 2017)


## Instalation
- Suivre le tutoriel suivant : https://blogs.msdn.microsoft.com/vcblog/2016/03/30/visual-c-for-linux-development/
- Si vous voulez avoir l'autocompletion, copier les fichiers sources de `/user/include/*` ver le dossier Includes de la solution
- telecharger Boost : `wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.bz2` puis extraire sous `~/` avec `tar --bzip2 -xf /path/to/boost_1_65_1.tar.bz2`.
- puis suivre la procédure : (http://www.boost.org/doc/libs/1_65_1/more/getting_started/unix-variants.html)
- Si vous souhaitez utilisé les lib static de Boost, enelver les .so générés précédement

>Pour l'utilisation avec WSl (Windows Subsystem for Linux)
- Suivre le tutoriel : https://blogs.msdn.microsoft.com/vcblog/2017/02/08/targeting-windows-subsystem-for-linux-from-visual-studio/

## Configuration de Linux
- Installer le package Tesseract 4.0 LSTM : https://launchpad.net/~alex-p/+archive/ubuntu/tesseract-ocr
- Installer le package libtesseract-dev et libleptonica-dev

## Utilisation
```Usage:
./LinuxTesseract.out --help
./LinuxTesseract.out [options...] /folder/of/images
./LinuxTesseract.out [options...] --folder /folder/of/images [options...]

Options:
  --PSM NUM (=0)            Page Segmentation Mode
  --OEM NUM (=0)            Ocr Engine Mode
  -l [ --lang ] LANG (=fra) Langue utilis pour l'OCR
  -h [ --help ]
  -t [ --thread ] NUM       Nombre de threads en parralle
  -o [ --output ] DOSSIER   Dossier de sortie
  -c [ --continue ]         Ne pas ecraser les fichiers existant
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
- `LinuxTesseract.out --help`