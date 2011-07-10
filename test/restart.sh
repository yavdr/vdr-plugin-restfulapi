#!/bin/bash
COUNTER=0
SUCCESS=0
while [ $COUNTER -lt 100 ]
do
  /etc/init.d/vdr start
  sleep 3;
  /etc/init.d/vdr stop
  NUMBER=`tail -1 /var/log/syslog | cut -d ',' -f 2 | cut -d ' ' -f 4`
  if [ "$NUMBER" == "0" ]; then
     SUCCESS=$[$SUCCESS+1]
  fi
  echo "Managed to restart vdr $SUCCESS times!"
  COUNTER=$[$COUNTER+1]
done
echo $SUCCESS
