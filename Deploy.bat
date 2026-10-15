@echo off
rem Copies ASWUnitTestsBCB, without the files only its contributors use, to the folder given (e.g. a staging folder).
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "TARGET_DIR=%~1"

if "%TARGET_DIR%"=="" (
    echo ERROR: A staging/deploy folder is required.
    echo Usage: Deploy.bat "C:\path\to\staging"
    exit /b 1
)

echo Deploying ASWUnitTestsBCB files to:
echo   %TARGET_DIR%

if not exist "%TARGET_DIR%\." (
    echo Creating target folder...
    mkdir "%TARGET_DIR%"
    if errorlevel 1 (
        echo ERROR: Could not create target folder.
        exit /b 1
    )
)

for %%D in (docs gui\rad370 gui\src rad370 src tests toTest) do (
    if not exist "%TARGET_DIR%\%%D\." (
        echo Creating %%D folder...
        mkdir "%TARGET_DIR%\%%D"
        if errorlevel 1 (
            echo ERROR: Could not create "%TARGET_DIR%\%%D".
            exit /b 1
        )
    )
)

call :CopyFile ".gitattributes" ".gitattributes"
if errorlevel 1 exit /b 1
call :CopyFile "CHANGELOG.md" "CHANGELOG.md"
if errorlevel 1 exit /b 1
call :CopyFile "LICENSE" "LICENSE"
if errorlevel 1 exit /b 1
call :CopyFile "README.md" "README.md"
if errorlevel 1 exit /b 1
call :CopyFile "docs\Differences.md" "docs\Differences.md"
if errorlevel 1 exit /b 1
call :CopyFile "docs\PortingFromASWUnitTests.md" "docs\PortingFromASWUnitTests.md"
if errorlevel 1 exit /b 1
call :CopyFile "docs\PortingFromDUnit.md" "docs\PortingFromDUnit.md"
if errorlevel 1 exit /b 1
call :CopyFile "gui\rad370\ASWUnitTestsBCB_GUI.cbproj" "gui\rad370\ASWUnitTestsBCB_GUI.cbproj"
if errorlevel 1 exit /b 1
call :CopyFile "gui\rad370\ASWUnitTestsBCB_GUI.cpp" "gui\rad370\ASWUnitTestsBCB_GUI.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "gui\rad370\Build_Win32Clang_Debug.bat" "gui\rad370\Build_Win32Clang_Debug.bat"
if errorlevel 1 exit /b 1
call :CopyFile "gui\rad370\Build_Win32Clang_Release.bat" "gui\rad370\Build_Win32Clang_Release.bat"
if errorlevel 1 exit /b 1
call :CopyFile "gui\rad370\Build_Win32_Debug.bat" "gui\rad370\Build_Win32_Debug.bat"
if errorlevel 1 exit /b 1
call :CopyFile "gui\rad370\Build_Win32_Release.bat" "gui\rad370\Build_Win32_Release.bat"
if errorlevel 1 exit /b 1
call :CopyFile "gui\rad370\Build_Win64x_Debug.bat" "gui\rad370\Build_Win64x_Debug.bat"
if errorlevel 1 exit /b 1
call :CopyFile "gui\rad370\Build_Win64x_Release.bat" "gui\rad370\Build_Win64x_Release.bat"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_CommandLine.cpp" "gui\src\ASWUnitTestsBCB_GUI_CommandLine.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_CommandLine.h" "gui\src\ASWUnitTestsBCB_GUI_CommandLine.h"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_Layout.cpp" "gui\src\ASWUnitTestsBCB_GUI_Layout.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_Layout.h" "gui\src\ASWUnitTestsBCB_GUI_Layout.h"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_MainForm.cpp" "gui\src\ASWUnitTestsBCB_GUI_MainForm.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_MainForm.dfm" "gui\src\ASWUnitTestsBCB_GUI_MainForm.dfm"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_MainForm.h" "gui\src\ASWUnitTestsBCB_GUI_MainForm.h"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_RunObserver.cpp" "gui\src\ASWUnitTestsBCB_GUI_RunObserver.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_RunObserver.h" "gui\src\ASWUnitTestsBCB_GUI_RunObserver.h"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_Selection.cpp" "gui\src\ASWUnitTestsBCB_GUI_Selection.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_Selection.h" "gui\src\ASWUnitTestsBCB_GUI_Selection.h"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_Shuffle.cpp" "gui\src\ASWUnitTestsBCB_GUI_Shuffle.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_Shuffle.h" "gui\src\ASWUnitTestsBCB_GUI_Shuffle.h"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_ShuffleSeedDialog.cpp" "gui\src\ASWUnitTestsBCB_GUI_ShuffleSeedDialog.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_ShuffleSeedDialog.h" "gui\src\ASWUnitTestsBCB_GUI_ShuffleSeedDialog.h"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_TestList.cpp" "gui\src\ASWUnitTestsBCB_GUI_TestList.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_TestList.h" "gui\src\ASWUnitTestsBCB_GUI_TestList.h"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_TextDialog.cpp" "gui\src\ASWUnitTestsBCB_GUI_TextDialog.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "gui\src\ASWUnitTestsBCB_GUI_TextDialog.h" "gui\src\ASWUnitTestsBCB_GUI_TextDialog.h"
if errorlevel 1 exit /b 1
call :CopyFile "rad370\ASWUnitTestsBCB.cbproj" "rad370\ASWUnitTestsBCB.cbproj"
if errorlevel 1 exit /b 1
call :CopyFile "rad370\ASWUnitTestsBCB_Group.groupproj" "rad370\ASWUnitTestsBCB_Group.groupproj"
if errorlevel 1 exit /b 1
call :CopyFile "rad370\Build_Win32Clang_Debug.bat" "rad370\Build_Win32Clang_Debug.bat"
if errorlevel 1 exit /b 1
call :CopyFile "rad370\Build_Win32Clang_Release.bat" "rad370\Build_Win32Clang_Release.bat"
if errorlevel 1 exit /b 1
call :CopyFile "rad370\Build_Win32_Debug.bat" "rad370\Build_Win32_Debug.bat"
if errorlevel 1 exit /b 1
call :CopyFile "rad370\Build_Win32_Release.bat" "rad370\Build_Win32_Release.bat"
if errorlevel 1 exit /b 1
call :CopyFile "rad370\Build_Win64x_Debug.bat" "rad370\Build_Win64x_Debug.bat"
if errorlevel 1 exit /b 1
call :CopyFile "rad370\Build_Win64x_Release.bat" "rad370\Build_Win64x_Release.bat"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_CLI.cpp" "src\ASWUnitTestsBCB_CLI.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_CLI.h" "src\ASWUnitTestsBCB_CLI.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Console.cpp" "src\ASWUnitTestsBCB_Console.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Console.h" "src\ASWUnitTestsBCB_Console.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_CrashGuard.cpp" "src\ASWUnitTestsBCB_CrashGuard.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_CrashGuard.h" "src\ASWUnitTestsBCB_CrashGuard.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Exception.cpp" "src\ASWUnitTestsBCB_Exception.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Exception.h" "src\ASWUnitTestsBCB_Exception.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Handler.cpp" "src\ASWUnitTestsBCB_Handler.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Handler.h" "src\ASWUnitTestsBCB_Handler.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_JUnitReport.cpp" "src\ASWUnitTestsBCB_JUnitReport.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_JUnitReport.h" "src\ASWUnitTestsBCB_JUnitReport.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Registry.cpp" "src\ASWUnitTestsBCB_Registry.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Registry.h" "src\ASWUnitTestsBCB_Registry.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_StdOutRedirect.cpp" "src\ASWUnitTestsBCB_StdOutRedirect.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_StdOutRedirect.h" "src\ASWUnitTestsBCB_StdOutRedirect.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_TestBase.cpp" "src\ASWUnitTestsBCB_TestBase.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_TestBase.h" "src\ASWUnitTestsBCB_TestBase.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Types.cpp" "src\ASWUnitTestsBCB_Types.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Types.h" "src\ASWUnitTestsBCB_Types.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Utils.cpp" "src\ASWUnitTestsBCB_Utils.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Utils.h" "src\ASWUnitTestsBCB_Utils.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Values.cpp" "src\ASWUnitTestsBCB_Values.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Values.h" "src\ASWUnitTestsBCB_Values.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\ASWUnitTestsBCB_Version.h" "src\ASWUnitTestsBCB_Version.h"
if errorlevel 1 exit /b 1
call :CopyFile "src\main.cpp" "src\main.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWExample_WordCounter.cpp" "tests\Test_ASWExample_WordCounter.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWExample_WordCounter.h" "tests\Test_ASWExample_WordCounter.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_CLI.cpp" "tests\Test_ASWUnitTestsBCB_CLI.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_CLI.h" "tests\Test_ASWUnitTestsBCB_CLI.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Console.cpp" "tests\Test_ASWUnitTestsBCB_Console.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Console.h" "tests\Test_ASWUnitTestsBCB_Console.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_CommandLine.cpp" "tests\Test_ASWUnitTestsBCB_GUI_CommandLine.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_CommandLine.h" "tests\Test_ASWUnitTestsBCB_GUI_CommandLine.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_Layout.cpp" "tests\Test_ASWUnitTestsBCB_GUI_Layout.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_Layout.h" "tests\Test_ASWUnitTestsBCB_GUI_Layout.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_RunObserver.cpp" "tests\Test_ASWUnitTestsBCB_GUI_RunObserver.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_RunObserver.h" "tests\Test_ASWUnitTestsBCB_GUI_RunObserver.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_Selection.cpp" "tests\Test_ASWUnitTestsBCB_GUI_Selection.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_Selection.h" "tests\Test_ASWUnitTestsBCB_GUI_Selection.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_Shuffle.cpp" "tests\Test_ASWUnitTestsBCB_GUI_Shuffle.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_Shuffle.h" "tests\Test_ASWUnitTestsBCB_GUI_Shuffle.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_TestList.cpp" "tests\Test_ASWUnitTestsBCB_GUI_TestList.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_GUI_TestList.h" "tests\Test_ASWUnitTestsBCB_GUI_TestList.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Handler.cpp" "tests\Test_ASWUnitTestsBCB_Handler.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Handler.h" "tests\Test_ASWUnitTestsBCB_Handler.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_JUnitReport.cpp" "tests\Test_ASWUnitTestsBCB_JUnitReport.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_JUnitReport.h" "tests\Test_ASWUnitTestsBCB_JUnitReport.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_StdOutRedirect.cpp" "tests\Test_ASWUnitTestsBCB_StdOutRedirect.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_StdOutRedirect.h" "tests\Test_ASWUnitTestsBCB_StdOutRedirect.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Suite.cpp" "tests\Test_ASWUnitTestsBCB_Suite.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Suite.h" "tests\Test_ASWUnitTestsBCB_Suite.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_TestBase.cpp" "tests\Test_ASWUnitTestsBCB_TestBase.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_TestBase.h" "tests\Test_ASWUnitTestsBCB_TestBase.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Utils.cpp" "tests\Test_ASWUnitTestsBCB_Utils.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Utils.h" "tests\Test_ASWUnitTestsBCB_Utils.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Values.cpp" "tests\Test_ASWUnitTestsBCB_Values.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Values.h" "tests\Test_ASWUnitTestsBCB_Values.h"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Version.cpp" "tests\Test_ASWUnitTestsBCB_Version.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "tests\Test_ASWUnitTestsBCB_Version.h" "tests\Test_ASWUnitTestsBCB_Version.h"
if errorlevel 1 exit /b 1
call :CopyFile "toTest\ASWExample_WordCounter.cpp" "toTest\ASWExample_WordCounter.cpp"
if errorlevel 1 exit /b 1
call :CopyFile "toTest\ASWExample_WordCounter.h" "toTest\ASWExample_WordCounter.h"
if errorlevel 1 exit /b 1

echo Deployment completed successfully.
exit /b 0

:CopyFile
set "SOURCE_FILE=%~1"
set "DESTINATION_FILE=%~2"
echo Copying %SOURCE_FILE%...
if not exist "%SCRIPT_DIR%%SOURCE_FILE%" (
    echo ERROR: Source file not found: "%SCRIPT_DIR%%SOURCE_FILE%"
    exit /b 1
)
copy /Y "%SCRIPT_DIR%%SOURCE_FILE%" "%TARGET_DIR%\%DESTINATION_FILE%" >nul
if errorlevel 1 (
    echo ERROR: Failed to copy "%SOURCE_FILE%".
    exit /b 1
)
exit /b 0
