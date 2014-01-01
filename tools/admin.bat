@echo off
set AppName=saker
set AppArg=-d
set CmdName="%cd%\..\bin\%AppName%.exe %AppArg%"


:begin
echo **********************************
echo install--------[1]
echo uninstall------[2]

set/p chose="Please chose:"

if "%chose%" == "1" goto Install
if "%chose%" == "install" goto Install

if "%chose%" == "2" goto Uninstall
if "%chose%" == "iuninstall" goto Uninstall

:Install
echo install %AppName% 
sc create %AppName% binPath= %CmdName% start= auto
sc start %AppName%
goto End

	
:Uninstall
echo uninstall %AppName% 
sc stop %AppName%
sc delete %AppName% 
goto End


:End
pause