#!/bin/bash

PROVISION_FILE=_provision_data.json

if ! [ -f $PROVISION_FILE ]; then
    echo "File $PROVISION_FILE does not exist!"
    exit 1
fi

data=$( cat $PROVISION_FILE )
echo $data | jq .