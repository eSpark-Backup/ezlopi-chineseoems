#!/bin/bash

PROVISION_FILE=_provision_data.json
PROTOCOL=https

if ! [ -f $PROVISION_FILE ]; then
    echo "File $PROVISION_FILE does not exist!"
    exit 1
fi

data=$( cat "$PROVISION_FILE" )

hardware_type=$( echo "$data" | jq -cMr ".hardware_type" )
provision_token=$( echo "$data" | jq -cMr ".provision_token" )
up_domain=$( echo "$data" | jq -cMr ".up_domain" )
version=$( echo "$data" | jq -cMr ".version" )

echo "Provision data:"
echo "- Hardware type: $hardware_type"
echo "- Provision token: $provision_token"
echo "- Up domain: $up_domain"
echo "- Version: $version"

min=""
fields=""

while (( "$#" )); do
    case "$1" in
        --version)
            version=$2
            shift 2
            echo "Using version from parameter: $version"
            ;;
        --min)
            min="&min=1"
            shift 1
            echo "Using min parameter"
            ;;
        --fields)
            fields="&fields=$2"
            shift 2
            echo "Using fields parameter with value: $fields"
            ;;
        -*|--*=) # unsupported flags
            echo "Error: Unsupported flag $1" >&2
            exit 1
            ;;
        *) # preserve positional arguments
            PARAMS="$PARAMS $1"
            shift
            ;;
    esac
done

# set positional arguments in their proper place
eval set -- "$PARAMS"

url="$PROTOCOL://$hardware_type.$up_domain/provision-sync?token=$provision_token&version=$version$min$fields"

echo "Sending request to $url"
response=$(curl --location --insecure --silent --write-out "HTTPSTATUS:%{http_code}" $url)

# split the response and status into two variables
HTTP_BODY=$(echo $response | sed -e 's/HTTPSTATUS\:.*//g')
HTTP_STATUS=$(echo $response | tr -d '\n' | sed -e 's/.*HTTPSTATUS://')

# print the body and status
echo "HTTP Status: $HTTP_STATUS"
echo "Body:"
echo $HTTP_BODY | jq .
