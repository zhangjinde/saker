
#!/bin/sh

APPNAME='saker'
APPPATH=`pwd`

cd ${APPPATH}/../bin

./saker-cli info

./saker-cli usage

./saker-cli shutdown