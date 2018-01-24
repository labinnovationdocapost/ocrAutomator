source Env/env.config

VERSION=1.4.6

printf "Copy to Nexus"
curl --fail --progress-bar -u $NEXUSACCOUNT --upload-file ./TesseractAutomator$VERSION.deb "$NEXUSURL/OcrAutomator$VERSION.deb"

for ip in "${REMOTE[@]}"
do
printf "\n\n"
printf "************************\n"
printf "Machine $ip\n"
printf "************************\n"
sshpass -p $ROOTPASS scp ./TesseractAutomator$VERSION.deb $ROOTACCOUNT@$ip:/home/$ROOTACCOUNT/Bureau/
sshpass -p $ROOTPASS ssh $ROOTACCOUNT@$ip "{ echo $ROOTPASS | sudo -S apt install -y ./Bureau/TesseractAutomator$VERSION.deb; rm ./Bureau/TesseractAutomator$VERSION.deb }"
done


#curl -u 'ctisserand:If(malko){mail}' 'https://outils.docapost.tech/nexus/service/extdirect' -H 'Content-Type: application/json' -H 'Accept: */*' --data-binary '{"action":"coreui_Component","method":"readAssets","data":[{"page":1,"start":0,"limit":300,"sort":[{"property":"name","direction":"ASC"}],"filter":[{"property":"repositoryName","value":"innovation-raw"}]}],"type":"rpc","tid":15}' --compressed