@echo off


set VSVER=14.0


echo.
echo Searching for VisualStudio setup folders
reg.exe query HKLM\Software\Wow6432Node > nul 2>&1
if errorlevel 1 (
	echo "We are at 32bit system"
	set wow=
) else (
	echo "We are at 64bit system, good :-)"
	set wow=\Wow6432Node
)

for /f "tokens=2*" %%a in ('reg.exe query HKLM\Software%wow%\Microsoft\VisualStudio\%VSVER%\Setup\VS /v EnvironmentPath') do set DEVENV=%%b
for /f "tokens=2*" %%a in ('reg.exe query HKLM\Software%wow%\Microsoft\VisualStudio\%VSVER%\Setup\VS /v ProductDir') do set PRODDIR=%%b
echo PRODDIR: %PRODDIR%
echo DEVENV: %DEVENV%

set THISDIR=%~dp0
set VSDIR="%PRODDIR%\VC"
set BOOST=C:\tmp\boost_1_61_0

call %VSDIR%\vcvarsall.bat x86

cl /EHsc /I%BOOST% /DHAVE_BOOST_LOCALE game.cxx /link /LIBPATH:%BOOST%\stage\lib

