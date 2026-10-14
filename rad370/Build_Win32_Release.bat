@echo off
rem Builds ASWUnitTestsBCB.exe with bcc32 (Release) into Win32\Release. Run from anywhere.
setlocal
cd /d "%~dp0"
call "%Rad370%\bin\rsvars.bat"
msbuild /nologo /t:Build /p:Config=Release;Platform=Win32 ASWUnitTestsBCB.cbproj
exit /b %ERRORLEVEL%
