# Script requirements
This scripts will run with bash and require the following command line utilities to be installed. They also need read/write access to current folder.
- curl
- jq
- mqttx cli https://mqttx.app/cli

# Scripts

## get provision data

This script gets provision all the provision data for a shade, by mac address and saves:
- mac address into _mac_address.txt
- provision data in 

Start with
- ```./get_provision_data.sh random``` to use a random mac address
- ```./get_provision_data.sh random 00:00:...``` to use specific mac address

## show provision data
Start with
- ```./show_provision_data.sh``` 

It will display the currently saved provision data as a nicely formated json object (formatted with jq)

## check provision data
Start this with
- ```./check_provision_for_updates``` and it will query provision server for latest version based on stored provision data
- ```./check_provision_for_updates --version <number>``` and it will query provision server for latest version based on stored provision data but it will use the given version number form parameter
- ```./check_provision_for_updates --min``` and it will return list of fields only
- ```./check_provision_for_updates --fields field1,field2``` and it will return minimal response with only those fields in the list

You can combine parameters, for example you can run
```
./check_provision_for_updates --version <number> --min
```

If you start it with version 0 you will force provision 0 to always return data

## shade provision flow

!Important! This script does not save anything locally, it just emulates general shade provision flow

Start with ```./shade__provision_flow.sh 00:00...``` or ``` ./shade__provision_flow.sh random```, same as get provision data script

Flow during manufacturing

1. call provision api to get new provision data, with min=1 parameter
    1. response will contain provision token, up domain and version
    2. this should be written to shade

Then once shade connects to internet

2. do call to provision sync 
    1. call with min=1 and version=0
    2. will return list of fields
3. then for each field or set of fields do call to provision sync
    1. call with version=0&fields=a,b,c...
    2. will return data for those fields only
4. save all the data on the shade
5. shade can then connect to cloud

## mqtt

Start with
- ```./mqtt.sh sub <parameters>``` run mosquitto_subscribe with the given parameters and inject host/port/client/etc from provision data and cloud api
- ```./mqtt.sh pub <parameters>``` run mosquitto_subscribe with the given parameters and inject host/port/client/etc from provision data and cloud api

The "^" character will be replaced by the correct topic prefix "client/<username>", so if you run script with
```
./mqtt.sh sub -t "^some_topic"
```
the command will be expanded to
```
mosquitto_sub .... -t "client/<username>/some_topic