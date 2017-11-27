# Image
FROM ubuntu:16.04

# Add packages
RUN DEBIAN_FRONTEND=noninteractive apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y software-properties-common nano language-pack-en language-pack-fr

ENV LANGUAGE en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LC_ALL en_US.UTF-8
ENV PYTHONIOENCODING UTF-8

#add to use tesseract 4 alpha use
RUN DEBIAN_FRONTEND=noninteractive add-apt-repository -y ppa:alex-p/tesseract-ocr
RUN DEBIAN_FRONTEND=noninteractive apt-get update

#tesseract
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y curl tesseract-ocr tesseract-ocr-fra tesseract-ocr-osd libtesseract-dev libleptonica-dev

RUN curl https://www.dropbox.com/s/cfjdbyaxhqa8fpa/TesseractAutomator1.3.7.deb?dl=1 -L -O -J
RUN apt-get install -y './TesseractAutomator1.3.7.deb'
