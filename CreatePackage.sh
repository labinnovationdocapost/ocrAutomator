#!/bin/bash
VERSION=$(./OcrAutomator -v | grep Version | cut -d ':' -f 2 | cut -d " " -f 2)
echo "Version: ${VERSION}"
if [ -z $VERSION ]
then
	exit
fi

mkdir -p "/tmp/Package/DEBIAN"
mkdir -p "/tmp/Package/usr/bin"
touch "/tmp/Package/DEBIAN/control"
cp "OcrAutomator" "/tmp/Package/usr/bin/"
echo "" > "/tmp/Package/DEBIAN/control"
echo "Package: OcrAutomator
Version: $VERSION
Section: base
Priority: optional
Architecture: amd64
Depends: libtesseract4 (>= 4), libncurses5 (>= 6.0)
Maintainer: Innovation <cyril.tisserand@docapost.fr>
Description: Tool based on Tesseract for parallelizing OCR, adding exifs in files efficiently and process big dataset 
Homepage: http://leia.io" >> "/tmp/Package/DEBIAN/control"
chmod -R 775 "/tmp/Package/"
sudo dpkg-deb --build "/tmp/Package/" OcrAutomator$VERSION.deb
rm -R /tmp/Package
#dpkg -i TesseractAutomator1.3.0.deb
