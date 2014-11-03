@echo off
set SourDir=%cd%\..\src
set Astyle=%cd%\AStyle.exe
set Format=--style=java --lineend=linux --convert-tabs 4 --align-pointer=name 
%Astyle% -r %SourDir%\core\*.c %Format%
%Astyle% -r %SourDir%\proto\*.c %Format%
%Astyle% -r %SourDir%\protocol\*.c %Format%
%Astyle% -r %SourDir%\sysinfo\*.c %Format%
%Astyle% -r %SourDir%\utils\*.c %Format%
%Astyle% %SourDir%\*.c %Format%
pause
