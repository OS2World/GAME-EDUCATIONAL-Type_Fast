@echo off
rem compile-wat.cmd - Build TypeFast with OpenWatcom

set LOG=compile-wat.log

echo TypeFast - OpenWatcom Build > %LOG%
echo. >> %LOG%

if "%WATCOM%"=="" (
    echo ERROR: WATCOM not set | tee -a %LOG%
    echo Install OpenWatcom and set WATCOM environment variable. | tee -a %LOG%
    goto :end
)
if "%OS2TK%"==""  set OS2TK=C:\os2tk45

echo WATCOM=%WATCOM% | tee -a %LOG%
echo OS2TK=%OS2TK%   | tee -a %LOG%
echo. | tee -a %LOG%

cd src

echo --- Clean --- | tee -a ..\%LOG%
wmake -f ..\makefile.wat clean 2>&1 | tee -a ..\%LOG%

echo --- Build --- | tee -a ..\%LOG%
wmake -f ..\makefile.wat all 2>&1 | tee -a ..\%LOG%

if not exist ..\bin\typefast.exe goto :failed
echo. | tee -a ..\%LOG%
echo BUILD OK - bin\typefast.exe | tee -a ..\%LOG%
cd ..
goto :end
:failed
echo. | tee -a ..\%LOG%
echo BUILD FAILED | tee -a ..\%LOG%
cd ..

:end
