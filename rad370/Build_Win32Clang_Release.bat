@echo off
rem Builds ASWUnitTestsBCB.exe with bcc32c (Release) into Win32Clang\Release. Run from anywhere.
rem The project's Win32 platform uses the classic compiler (bcc32). The properties below switch it to bcc32c, give that
rem build its own folders, and pass the Clang options bcc32 would reject. They live here, not in the project, because
rem the IDE drops a hand-written conditional PropertyGroup when it saves the project.
setlocal
cd /d "%~dp0"
call "%Rad370%\bin\rsvars.bat"
msbuild /nologo /t:Build /p:Config=Release;Platform=Win32 /p:BCC_UseClassicCompiler=false /p:IntermediateOutputDir=.\Win32Clang\Release\ /p:FinalOutputDir=.\Win32Clang\Release\ /p:BCC_UserSuppliedOptions=-Wno-padded ASWUnitTestsBCB.cbproj
exit /b %ERRORLEVEL%
