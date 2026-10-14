@echo off
rem Builds ASWUnitTestsBCB_GUI.exe with bcc64x (Release) into Win64x\Release. Run from anywhere.
setlocal
cd /d "%~dp0"
call "%Rad370%\bin\rsvars.bat"
msbuild /nologo /t:Build /p:Config=Release;Platform=Win64x ASWUnitTestsBCB_GUI.cbproj
exit /b %ERRORLEVEL%
