source Env/env.config

VERSION=1.4.8

printf "Copy to Nexus"
curl --fail --progress-bar -u $NEXUSACCOUNT --upload-file ./TesseractAutomator$VERSION.deb "$NEXUSURL/OcrAutomator$VERSION.deb"
curl --progress-bar -u $NEXUSACCOUNT -X POST -H "Content-Type: multipart/form-data" --data-binary @TesseractAutomator$VERSION.deb $NEXUSAPTURL

for ip in "${REMOTE[@]}"
do
printf "\n\n"
printf "************************\n"
printf "Machine $ip\n"
printf "************************\n"
sshpass -p $ROOTPASS scp ./TesseractAutomator$VERSION.deb $ROOTACCOUNT@$ip:/home/$ROOTACCOUNT/Bureau/
sshpass -p $ROOTPASS ssh $ROOTACCOUNT@$ip "{ echo $ROOTPASS | sudo -S apt install -y ./Bureau/TesseractAutomator$VERSION.deb; rm ./Bureau/TesseractAutomator$VERSION.deb }"
done