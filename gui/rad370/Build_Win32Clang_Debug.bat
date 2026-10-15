@echo off
rem Builds ASWUnitTestsBCB_GUI.exe with bcc32c (Debug) into Win32Clang\Debug. Run from anywhere.
rem The project's Win32 platform uses the classic compiler (bcc32). The properties below switch it to bcc32c, give that
rem build its own folders, and pass the Clang options bcc32 would reject. They live here, not in the project, because
rem the IDE drops a hand-written conditional PropertyGroup when it saves the project.
setlocal
cd /d "%~dp0"
call "%Rad370%\bin\rsvars.bat"
msbuild /nologo /t:Build /p:Config=Debug;Platform=Win32 /p:BCC_UseClassicCompiler=false /p:IntermediateOutputDir=.\Win32Clang\Debug\ /p:FinalOutputDir=.\Win32Clang\Debug\ /p:BCC_UserSuppliedOptions=-Wno-padded ASWUnitTestsBCB_GUI.cbproj
exit /b %ERRORLEVEL%
