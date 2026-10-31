/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_Theme.cpp
Author: Anthony S. West - ASW Software

See header for info.

Copyright 2026 ASW Software

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************** */

#include <System.hpp>
#pragma hdrstop

//---------------------------------------------------------------------------
// Module header
#include "Test_ASWUnitTestsBCB_GUI_Theme.h"
//---------------------------------------------------------------------------
#include <memory>
//---------------------------------------------------------------------------
#include <System.IniFiles.hpp>
#include <System.IOUtils.hpp>
#include <System.SysUtils.hpp>
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_Layout.h"
#include "ASWUnitTestsBCB_GUI_Theme.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Inifiles;
using namespace System::Ioutils;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

TMemIniFile* NewMemIniFile();

//---------------------------------------------------------------------------
/*
    NewMemIniFile

    An empty, in-memory INI file; nothing is read from or written to disk. The caller owns it.
*/
TMemIniFile* NewMemIniFile()
{
    return new TMemIniFile(UnicodeString());
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_Theme
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_GUI_Theme::TTest_ASWUnitTestsBCB_GUI_Theme()
    : inherited("ASWUnitTestsBCB_GUI_Theme_Tests"),
      m_TempFolder(TPath::Combine(TPath::GetTempPath(), UnicodeString("ASWUnitTestsBCB_GUI_Theme_Tests_") +
          UIntToStr(static_cast<unsigned int>(::GetCurrentProcessId()))))
{
}
//---------------------------------------------------------------------------
void TTest_ASWUnitTestsBCB_GUI_Theme::SetUp_Group()
{
    Log("Setting up temp group folder: " + ToUTF8(m_TempFolder));
    TDirectory::CreateDirectory(m_TempFolder);
}
//---------------------------------------------------------------------------
void TTest_ASWUnitTestsBCB_GUI_Theme::TearDown_Group()
{
    Log("Cleaning up temp group folder: " + ToUTF8(m_TempFolder));

    if (TDirectory::Exists(m_TempFolder))
        TDirectory::Delete(m_TempFolder, true);
}
//---------------------------------------------------------------------------

// /////// Begin tests after this line ///////////////////////

//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_GUIThemeIsDark_ChosenThemeIgnoresWindows()
{
    // Act and Assert: every combination of the app mode and high contrast.
    for (int mode = 0; mode < 4; ++mode)
    {
        bool const appsUseDark = (mode & 1) != 0;
        bool const highContrast = (mode & 2) != 0;
        std::string const context = "app mode dark " + IntToString(appsUseDark ? 1 : 0) + ", high contrast " +
            IntToString(highContrast ? 1 : 0);

        CheckTrue(GUIThemeIsDark(EGUITheme::Dark, appsUseDark, highContrast), ASWBCB_HERE, "Dark, " + context);
        CheckFalse(GUIThemeIsDark(EGUITheme::Light, appsUseDark, highContrast), ASWBCB_HERE, "Light, " + context);
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_GUIThemeIsDark_WindowsFollowsAppModeUnlessHighContrast()
{
    // Act and Assert
    CheckTrue(GUIThemeIsDark(EGUITheme::Windows, true, false), ASWBCB_HERE, "dark app mode");
    CheckFalse(GUIThemeIsDark(EGUITheme::Windows, false, false), ASWBCB_HERE, "light app mode");
    CheckFalse(GUIThemeIsDark(EGUITheme::Windows, true, true), ASWBCB_HERE,
        "dark app mode under high contrast, which shows its own colors");
    CheckFalse(GUIThemeIsDark(EGUITheme::Windows, false, true), ASWBCB_HERE, "light app mode under high contrast");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_LoadGUITheme_IgnoresCase()
{
    // Arrange
    std::unique_ptr<TMemIniFile> const ini(NewMemIniFile());

    // Act and Assert
    ini->WriteString("View", "Theme", "dARK");
    CheckEquals(EGUITheme::Dark, LoadGUITheme(*ini), ASWBCB_HERE, "dARK");

    ini->WriteString("View", "Theme", "LIGHT");
    CheckEquals(EGUITheme::Light, LoadGUITheme(*ini), ASWBCB_HERE, "LIGHT");

    ini->WriteString("View", "Theme", "windows");
    CheckEquals(EGUITheme::Windows, LoadGUITheme(*ini), ASWBCB_HERE, "windows");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_LoadGUITheme_WindowsWhenMissingOrUnknown()
{
    // Arrange
    std::unique_ptr<TMemIniFile> const ini(NewMemIniFile());

    // Act and Assert
    CheckEquals(EGUITheme::Windows, LoadGUITheme(*ini), ASWBCB_HERE, "no [View] section");

    ini->WriteString("View", "Other", "Dark");
    CheckEquals(EGUITheme::Windows, LoadGUITheme(*ini), ASWBCB_HERE, "no Theme key");

    ini->WriteString("View", "Theme", "Carbon");
    CheckEquals(EGUITheme::Windows, LoadGUITheme(*ini), ASWBCB_HERE,
        "a value that isn't a theme");

    ini->WriteString("View", "Theme", " Dark");
    CheckEquals(EGUITheme::Windows, LoadGUITheme(*ini), ASWBCB_HERE,
        "a theme's name with more around it");

    ini->WriteString("View", "Theme", "");
    CheckEquals(EGUITheme::Windows, LoadGUITheme(*ini), ASWBCB_HERE, "an empty value");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_LoadGUIThemeFromLayoutFile_WindowsWithoutAFile()
{
    // Arrange
    UnicodeString const missingPath = TPath::Combine(TPath::Combine(m_TempFolder, "Missing"), "layout.ini");
    UnicodeString const folderPath = m_TempFolder; // A folder, which can't be read as a file

    // Act and Assert
    CheckEquals(EGUITheme::Windows, LoadGUIThemeFromLayoutFile(missingPath), ASWBCB_HERE,
        "a file that doesn't exist");
    CheckEquals(EGUITheme::Windows, LoadGUIThemeFromLayoutFile(folderPath), ASWBCB_HERE,
        "a folder's path");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_SaveGUITheme_RoundTrips()
{
    // Arrange
    EGUITheme const themes[] = { EGUITheme::Windows, EGUITheme::Light, EGUITheme::Dark };
    char const* const names[] = { "Windows", "Light", "Dark" };

    for (int i = 0; i < 3; ++i)
    {
        std::unique_ptr<TMemIniFile> const ini(NewMemIniFile());

        // Act
        SaveGUITheme(*ini, themes[i]);

        // Assert
        CheckEquals(names[i], ini->ReadString("View", "Theme", UnicodeString()), ASWBCB_HERE,
            std::string("written by name: ") + names[i]);
        CheckEquals(themes[i], LoadGUITheme(*ini), ASWBCB_HERE,
            std::string("read back: ") + names[i]);
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_SaveGUIThemeToLayoutFile_KeepsTheLayout()
{
    // Arrange: a layout file, in a folder that doesn't exist yet, saved as the window saves it.
    UnicodeString const path = TPath::Combine(TPath::Combine(m_TempFolder, "Layout"), "layout.ini");
    TGUILayout layout;
    layout.TestTreeWidth = 321;
    ForceDirectories(ExtractFileDir(path));
    {
        std::unique_ptr<TMemIniFile> const ini(new TMemIniFile(path));
        SaveGUILayout(*ini, layout);
        ini->UpdateFile();
    }

    // Act
    SaveGUIThemeToLayoutFile(path, EGUITheme::Dark);

    // Assert
    std::unique_ptr<TMemIniFile> const ini(new TMemIniFile(path));
    CheckEquals(EGUITheme::Dark, LoadGUIThemeFromLayoutFile(path), ASWBCB_HERE,
        "the theme is saved");
    TGUILayout const loaded = LoadGUILayout(*ini);
    AssertTrue(loaded.TestTreeWidth.HasValue(), ASWBCB_HERE, "the layout is kept");
    CheckEquals(321, loaded.TestTreeWidth.Value(), ASWBCB_HERE, "with its value");

    // Act and Assert: a file and folder that don't exist yet are created.
    UnicodeString const newPath = TPath::Combine(TPath::Combine(m_TempFolder, "New"), "layout.ini");
    SaveGUIThemeToLayoutFile(newPath, EGUITheme::Light);
    CheckEquals(EGUITheme::Light, LoadGUIThemeFromLayoutFile(newPath), ASWBCB_HERE,
        "saved to a new file in a new folder");
}
//---------------------------------------------------------------------------
/*
    TTest_ASWUnitTestsBCB_GUI_Theme::Test_WindowsAppsUseDarkTheme_MatchesTheRegistry

    Read here through the Windows API itself, rather than TRegistry, which WindowsAppsUseDarkTheme() uses. Passes on
    either setting, and on a Windows version without it.
*/
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_WindowsAppsUseDarkTheme_MatchesTheRegistry()
{
    // Arrange
    bool expected = false;
    HKEY key = nullptr;
    LONG const opened = ::RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &key);

    if (opened == ERROR_SUCCESS)
    {
        DWORD type = 0;
        DWORD value = 1;
        DWORD size = sizeof(value);
        LONG const queried = ::RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, &type,
            reinterpret_cast<BYTE*>(&value), &size);

        if ((queried == ERROR_SUCCESS) && (type == REG_DWORD))
            expected = (value == 0);

        ::RegCloseKey(key);
    }

    Log(std::string("Windows' app mode is ") + (expected ? "dark" : "light (or not set)"));

    // Act and Assert
    CheckEquals(expected, WindowsAppsUseDarkTheme(), ASWBCB_HERE);
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_GUI_Theme, "SelfTests.GUI")
