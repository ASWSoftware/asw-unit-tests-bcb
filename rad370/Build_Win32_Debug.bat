@echo off
rem Builds ASWUnitTestsBCB.exe with bcc32 (Debug) into Win32\Debug. Run from anywhere.
setlocal
cd /d "%~dp0"
call "%Rad370%\bin\rsvars.bat"
msbuild /nologo /t:Build /p:Config=Debug;Platform=Win32 ASWUnitTestsBCB.cbproj
exit /b %ERRORLEVEL%
