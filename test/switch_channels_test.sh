#!/bin/bash
CHANNELS=`java -jar httprequest.jar GET /channels.xml | grep -i channel_id`
for line in $CHANNELS
do
  SINGLEID=`echo $line | cut -d '>' -f 2 | cut -d '<' -f 1`
  if [ "$SINGLEID" != "" ] 
  then
  echo "Now switching to $SINGLEID!"
  RESULT=`java -jar httprequest.jar POST "/remote/switch/$SINGLEID"`
  HTTPRESULT=`echo $RESULT | grep "HTTP/1.1" | cut -d ' ' -f 2`   #HTTP/1.1 200 OK
    if [ $HTTPRESULT != 200 ] 
    then
      echo "Failed!"
    fi
  fi
done
