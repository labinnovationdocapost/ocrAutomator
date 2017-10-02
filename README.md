# LinuxTesseract


Le projet est compiler grace à Visual Studio for Linux Development (inclu dans l'instalation de Visual Studio 2017)


## Instalation
- Suivre le tutoriel suivant : https://blogs.msdn.microsoft.com/vcblog/2016/03/30/visual-c-for-linux-development/
- Si vous voulez avoir l'autocompletion, copier les fichiers sources de /user/include/* ver le dossier Includes de la solution

>Pour l'utilisation avec WSl (Windows Subsystem for Linux)
- Suivre le tutoriel : https://blogs.msdn.microsoft.com/vcblog/2017/02/08/targeting-windows-subsystem-for-linux-from-visual-studio/

## Configuration de Linux
- Installer le package Tesseract 4.0 LSTM : https://launchpad.net/~alex-p/+archive/ubuntu/tesseract-ocr
- Installer le package libtesseract-dev et libleptonica-dev

## Utilisation
`LinuxTesseract.out /chemin/vers/mes/images x`
- **/chemin/vers/mes/images** : chemin vers un dossier contenant les images a traiter (peut contenir des sous dossiers)
- **x** : [optionnel, default : 2] nombre de thread exécutant le moteur Tesseract