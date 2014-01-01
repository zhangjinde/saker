#!/bin/sh
APP=saker

APPPID=`pidof $APP`
SMAPSFILE=${APP}.smaps


while [ 1 ]
do
    if [ -d /proc/$APPPID ];then
        date +"%Y-%m-%d %H:%M:%S" >> $SMAPSFILE
        cat /proc/${APPPID}/smaps >> $SMAPSFILE
    else 
        echo "$APP exit "
        exit 0
    fi
    sleep 5
done





