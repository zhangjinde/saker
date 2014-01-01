#!/bin/sh -e

AppName=UGMonitor
AppArg=-d

function AddToinittab
{
	if grep $AppName /etc/inittab >/dev/null
	then
	  echo "inittab contains an $AppName line. I assume that $AppName is already running."
	else
	  echo "Adding $AppName to inittab..."
	  rm -f /etc/inittab'{new}'
	  cat /etc/inittab > /etc/inittab'{new}'
	  echo $* >> /etc/inittab'{new}'
	  mv -f /etc/inittab'{new}' /etc/inittab
	  kill -HUP 1
	  echo "init should start $AppName now."
	fi
}

function AddTorclocal
{
	if grep $AppName /etc/rc.local >/dev/null
	then
	  echo "rc.local contains an $AppName line. I assume that $AppName is already running."
	else
	  echo "Adding $AppName to /etc/rc.local..."
	  rm -f /etc/rc.local'{new}'
	  cat /etc/rc.local > /etc/rc.local'{new}'
	  echo  $* >> /etc/rc.local'{new}'
	  mv -f /etc/rc.local'{new}' /etc/rc.local
	  echo "Reboot now to start $AppName."
	fi
}


function DelFrominittab
{
	if grep $AppName /etc/inittab >/dev/null
	then
	  echo "inittab contains an $AppName line. .Delete it."
	  
	else
	  echo "cannot find $AppName ,I assume you have uninstall it or doesn't install it..."
	fi
}

function DelFromrclocal
{
	if grep $AppName /etc/rc.local >/dev/null
	then
	  echo "rc.local contains an $AppName line.Delete it."
	  
	else
	  echo "cannot find $AppName ,I assume you have uninstall it or doesn't install it..."
	fi
}

function Install
{
	ShFile=`readlink -f $0`
	BinPath=`dirname $ShFile`
	BinFile=$BinPath/$AppName
	umask 022
	test -d $BinPath || ( echo "cannot find $BinPath."; exit 1 )
	chmod u+x $BinPath
	bootstr="SV:123456:respawn:$BinFile $AppArg"
	#nohup app &
	rclocalstr="csh -cf '$BinFile $AppArg &'"
	
	if test -r /etc/inittab
	then
	    AddToinittab $bootstr
	else
	    AddTorclocal $rclocalstr
	fi
}


function Uninstall
{
	if test -r /etc/inittab 
		DelFrominittab
	then
		DelFromrclocal
	fi
	
}


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

