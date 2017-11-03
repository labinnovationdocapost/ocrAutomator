
VERSION=1.3.1.3

mkdir -p "/tmp/Package/DEBIAN"
mkdir -p "/tmp/Package/usr/bin"
touch "/tmp/Package/DEBIAN/control"
cp "TesseractAutomator/bin/x64/Release/TesseractAutomator" "/tmp/Package/usr/bin/"
echo "" > "/tmp/Package/DEBIAN/control"
echo "Package: TesseractAutomator
Version: $VERSION
Section: base
Priority: optional
Architecture: all
Depends: libtesseract4 (>= 4), libncurses5 (>= 6.0), libexiv2-14 (>= 0.25), libprotobuf-lite9v5 (>= 2.6.1), libprotobuf9v5 (>=2.6.1), libmagick++-6.q16-5v5 (>=6.8.9.9)
Maintainer: Innovation <cyril.tisserand@docapost.fr>
Description: Automatise l'Ocr par Tesseract sur plusieur thread et dans plusieur modes différents
Homepage: http://docapost.com" >> "/tmp/Package/DEBIAN/control"
chmod -R 775 "/tmp/Package/"
sudo dpkg-deb --build "/tmp/Package/" TesseractAutomator$VERSION.deb
rm -R /tmp/Package
#dpkg -i TesseractAutomator1.3.0.deb