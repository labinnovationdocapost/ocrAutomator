
VERSION=$(OcrAutomatorLinux/bin/x64/Release/TesseractAutomator -v | grep Version | cut -d ':' -f 2 | cut -d " " -f 2)
echo "Version: ${VERSION}"
if [ -z $VERSION ]
then
	exit
fi

mkdir -p "/tmp/Package/DEBIAN"
mkdir -p "/tmp/Package/usr/bin"
touch "/tmp/Package/DEBIAN/control"
cp "OcrAutomatorLinux/bin/x64/Release/TesseractAutomator" "/tmp/Package/usr/bin/"
echo "" > "/tmp/Package/DEBIAN/control"
echo "Package: TesseractAutomator
Version: $VERSION
Section: base
Priority: optional
Architecture: amd64
Depends: libtesseract4 (>= 4), libncurses5 (>= 6.0), libexiv2-14 (>= 0.25), libprotobuf-lite9v5 (>= 2.6.1), libprotobuf9v5 (>=2.6.1), libmagick++-6.q16-5v5 (>=6.8.9.9), libarchive13 (>=3.1.2)
Maintainer: Innovation <cyril.tisserand@docapost.fr>
Description: Automatise l'Ocr par Tesseract sur plusieur thread et dans plusieur modes différents
Homepage: http://docapost.com" >> "/tmp/Package/DEBIAN/control"
chmod -R 775 "/tmp/Package/"
sudo dpkg-deb --build "/tmp/Package/" TesseractAutomator$VERSION.deb
rm -R /tmp/Package
#dpkg -i TesseractAutomator1.3.0.deb