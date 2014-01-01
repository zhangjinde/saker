#!/bin/sh

APPNAME='saker'
APPPATH=`pwd`

cd ${APPPATH}/../bin

which valgrind
if [ $? -ne 0 ] ;then
  echo "memcheck require valgrind, need install it"
  exit 1
fi

if [ ! -f ${APPNAME} ];then
  echo "cannot find ${APPNAME} ,need build it frist"
  exit 1
fi

valgrind --tool=memcheck --leak-check=yes --show-reachable=yes ./${APPNAME}



