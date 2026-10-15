@echo off
rem Builds ASWUnitTestsBCB_GUI.exe with bcc32 (Release) into Win32\Release. Run from anywhere.
setlocal
cd /d "%~dp0"
call "%Rad370%\bin\rsvars.bat"
msbuild /nologo /t:Build /p:Config=Release;Platform=Win32 ASWUnitTestsBCB_GUI.cbproj
exit /b %ERRORLEVEL%
