#!/bin/bash

PROVISION_APPK_KEY=UQIhfmB9Qq3XvHK7oDE2wMRCMQ6ajutt0j%2F9ddQQOwVW2ljNtqdB%2BXZyqqHihbZHh%2BSI87LwvJ%2F%2FzaD%2B%2F%2B6j7fGwk8lwy85hPIYfQda1%2Fr1guZ1osnRrM7855q%2FMRbUnkDaIeY2IbFdc0sgxogglEw4V%2Brd%2Bkv2EmhhyAfa6Jqv54W1rN7%2F1iCC4m0azPZPCfZS1t4hajFlbdZrEcdDFIP6RvuuJ2%2B%2FhCBrkwVn6oo0qkKGCTYApeyv6hfLvlu3f
PROVISION_ENVIRONMENT_UUID=b9c0e91d-7614-48b7-b9cd-09eef78200b9
PROVISION_FILE=_provision_data.json
MAC_FILE=_mac_address.txt
MAC_BT_FILE=_mac_bt_address.txt
KEY_FILE=_key.txt
PROTOCOL=https
HARDWARE_TYPE=ezlopiesp32
UP_DOMAIN="uppublic.mios.com"
ENV_SUFFIX=""

mac=""
if [ -z "$1" ]; then
    echo "No parameter supplied"
    exit 1
else
  mac=$1
  shift
fi

if [[ "$ENV_SUFFIX" == "" ]]; then
    echo "Using production environment"
else
    echo "Using environment suffix: $ENV_SUFFIX, NOT PRODUCTION!"
    UP_DOMAIN=${UP_DOMAIN/up./up$ENV_SUFFIX.}
    echo "Using UP domain: $UP_DOMAIN"
fi


if [ -f $PROVISION_FILE ]; then
    echo "File $PROVISION_FILE exists. Please remove it first"
    exit 1
fi

if [ -f $MAC_FILE ]; then
    echo "File $MAC_FILE exists. Please remove it first"
    exit 1
fi

if [[ "$mac" == "random" ]]; then
    mac=$(openssl rand -hex 6 | sed 's/\(..\)/\1:/g; s/.$//')
    echo "Random MAC address generated: $mac"
    mac_bt=$(openssl rand -hex 6 | sed 's/\(..\)/\1:/g; s/.$//')
    echo "Random bluetooth MAC address generated: wifi: $mac_bt"
    key=$(openssl rand -hex 64)
    echo "Random key: $key"
fi

echo "Getting provision data for $mac and saving it to $PROVISION_FILE"

url="$PROTOCOL://$HARDWARE_TYPE.$UP_DOMAIN/first-provision?appkey=$PROVISION_APPK_KEY&environment_uuid=$PROVISION_ENVIRONMENT_UUID"
echo ".. using url $url"

response=$(curl --verbose --location $url \
--header 'Content-Type: application/json' \
--insecure \
--data "{
    \"hardware\": {
        \"mac_address\": \"$mac\",
        \"mac_bt\": \"$mac_bt\",
        \"key\": \"$key\"
    }
}")

if [ $? -ne 0 ]; then
    echo "Curl command failed"
    exit 1
else
   echo "Curl exited with success!"
   echo "Response: $response"
   response=$(echo "$response" | jq -cM .data )
   echo $mac > $MAC_FILE
   echo $mac_bt > $MAC_BT_FILE
   echo $key > $KEY_FILE
   echo -n "Writing to $PROVISION_FILE ... "
   echo $response > $PROVISION_FILE
   echo "Done!"
fi