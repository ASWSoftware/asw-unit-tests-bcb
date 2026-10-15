@echo off
rem Builds ASWUnitTestsBCB.exe with bcc64x (Debug) into Win64x\Debug. Run from anywhere.
setlocal
cd /d "%~dp0"
call "%Rad370%\bin\rsvars.bat"
msbuild /nologo /t:Build /p:Config=Debug;Platform=Win64x ASWUnitTestsBCB.cbproj
exit /b %ERRORLEVEL%
