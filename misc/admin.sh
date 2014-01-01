#!/bin/sh -e

OSID=`lsb_release -i`
OSVERSION=`lsb_release -r`


function Main
{
	echo "Install----------[1]"
	echo "Uninstall--------[2]"
	read chose 
	if [ "X$chose" == "X1" ] || [ "X$chose" == "Xinstall" ]
	then
		Install
	else 
	  Uninstall
	fi
}

Main

