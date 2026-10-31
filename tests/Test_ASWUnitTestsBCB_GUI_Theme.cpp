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
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
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

std::string Joined(std::vector<std::string> const& names);
TMemIniFile* NewMemIniFile();

//---------------------------------------------------------------------------
/*
    Joined

    'names' joined with " | ", which a check can compare and show whole.
*/
std::string Joined(std::vector<std::string> const& names)
{
    std::string text;

    for (std::size_t i = 0; i < names.size(); ++i)
        text += (i == 0 ? "" : " | ") + names[i];

    return text;
}
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
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_FindGUIBuiltInDarkTheme_MatchesIgnoringCase()
{
    // Arrange
    char const* const builtIn[] = { "Carbon", "Onyx Blue" };
    std::vector<std::string> const names(builtIn, builtIn + 2);

    // Act and Assert
    CheckEquals("Onyx Blue", FindGUIBuiltInDarkTheme(names, "onyx BLUE"), ASWBCB_HERE, "as the list names it");
    CheckEquals("Carbon", FindGUIBuiltInDarkTheme(names, "Carbon"), ASWBCB_HERE, "the same case");
    CheckEquals("", FindGUIBuiltInDarkTheme(names, "Tablet Dark"), ASWBCB_HERE, "not in the list");
    CheckEquals("", FindGUIBuiltInDarkTheme(names, "Onyx"), ASWBCB_HERE, "only part of a name");
    CheckEquals("", FindGUIBuiltInDarkTheme(std::vector<std::string>(), "Carbon"), ASWBCB_HERE, "an empty list");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_GUIBuiltInDarkThemeNames_CarbonThenLinkedStyles()
{
    // Arrange: the styles a project links in, in no particular order or case, with others among them.
    char const* const linked[] = { "Windows", "tablet dark", "Carbon", "Amethyst Kamri", "Charcoal Dark Slate" };
    std::vector<std::string> const styleNames(linked, linked + 5);

    // Act and Assert
    CheckEquals("Carbon | Charcoal Dark Slate | Tablet Dark", Joined(GUIBuiltInDarkThemeNames(styleNames)), ASWBCB_HERE,
        "Carbon first, then the linked dark styles in their order, named as the runner names them");
    CheckEquals("Carbon", Joined(GUIBuiltInDarkThemeNames(std::vector<std::string>())), ASWBCB_HERE,
        "Carbon even when no style is linked in, since it's the default");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_GUIMenuItemText_DoublesAmpersands()
{
    // Act and Assert
    CheckEquals("Onyx Blue", GUIMenuItemText("Onyx Blue"), ASWBCB_HERE, "no '&'");
    CheckEquals("Salt && Pepper", GUIMenuItemText("Salt & Pepper"), ASWBCB_HERE, "one");
    CheckEquals("&&&&A&&", GUIMenuItemText("&&A&"), ASWBCB_HERE, "several, at the ends too");
    CheckEquals("", GUIMenuItemText(""), ASWBCB_HERE, "empty");
}
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
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_GUIThemeProblemLine_NamesTheTheme()
{
    // Act and Assert
    CheckEquals("Dark theme \"Onyx Blue\": It isn't linked in.",
        GUIThemeProblemLine("Onyx Blue", "It isn't linked in."), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_GUIVCLStyleDarkThemeNames_SortedWithoutCarbon()
{
    // Act and Assert
    CheckEquals("Charcoal Dark Slate | Cobalt XEMedia | Onyx Blue | Tablet Dark", Joined(GUIVCLStyleDarkThemeNames()),
        ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_LoadGUIDarkTheme_CarbonWhenMissingEmptyOrCarbon()
{
    // Arrange
    std::unique_ptr<TMemIniFile> const ini(NewMemIniFile());

    // Act and Assert
    CheckEquals("Carbon", LoadGUIDarkTheme(*ini), ASWBCB_HERE, "no [View] section");

    ini->WriteString("View", "Theme", "Dark");
    CheckEquals("Carbon", LoadGUIDarkTheme(*ini), ASWBCB_HERE, "no DarkTheme key");

    ini->WriteString("View", "DarkTheme", "");
    CheckEquals("Carbon", LoadGUIDarkTheme(*ini), ASWBCB_HERE, "an empty value");

    ini->WriteString("View", "DarkTheme", "cARBON");
    CheckEquals("Carbon", LoadGUIDarkTheme(*ini), ASWBCB_HERE, "Carbon in another case, named as the runner names it");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_LoadGUIDarkTheme_KeepsOtherNames()
{
    // Arrange
    std::unique_ptr<TMemIniFile> const ini(NewMemIniFile());

    // Act and Assert: kept as written, since a name the runner doesn't know may be a style a later version adds.
    ini->WriteString("View", "DarkTheme", "onyx blue");
    CheckEquals("onyx blue", LoadGUIDarkTheme(*ini), ASWBCB_HERE, "a built-in theme, in its case as saved");

    ini->WriteString("View", "DarkTheme", "Not A Theme");
    CheckEquals("Not A Theme", LoadGUIDarkTheme(*ini), ASWBCB_HERE, "a name that isn't a theme's");

    SaveGUIDarkTheme(*ini, "Tablet Dark");
    CheckEquals("Tablet Dark", ini->ReadString("View", "DarkTheme", UnicodeString()), ASWBCB_HERE,
        "SaveGUIDarkTheme() writes the name to the DarkTheme key");
    CheckEquals("Tablet Dark", LoadGUIDarkTheme(*ini), ASWBCB_HERE, "which reads back");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_LoadGUIDarkThemeFromLayoutFile_CarbonWithoutAFile()
{
    // Arrange
    UnicodeString const missingPath = TPath::Combine(TPath::Combine(m_TempFolder, "Missing"), "layout.ini");
    UnicodeString const folderPath = m_TempFolder; // A folder, which can't be read as a file

    // Act and Assert
    CheckEquals("Carbon", LoadGUIDarkThemeFromLayoutFile(missingPath), ASWBCB_HERE, "a file that doesn't exist");
    CheckEquals("Carbon", LoadGUIDarkThemeFromLayoutFile(folderPath), ASWBCB_HERE, "a folder's path");
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
void __fastcall TTest_ASWUnitTestsBCB_GUI_Theme::Test_SaveGUIDarkThemeToLayoutFile_KeepsTheThemeAndLayout()
{
    // Arrange: a layout file with a theme, in a folder that doesn't exist yet.
    UnicodeString const path = TPath::Combine(TPath::Combine(m_TempFolder, "DarkLayout"), "layout.ini");
    TGUILayout layout;
    layout.DetailHeight = 123;
    ForceDirectories(ExtractFileDir(path));
    {
        std::unique_ptr<TMemIniFile> const ini(new TMemIniFile(path));
        SaveGUILayout(*ini, layout);
        ini->UpdateFile();
    }
    SaveGUIThemeToLayoutFile(path, EGUITheme::Dark);

    // Act
    SaveGUIDarkThemeToLayoutFile(path, "Onyx Blue");

    // Assert
    CheckEquals("Onyx Blue", LoadGUIDarkThemeFromLayoutFile(path), ASWBCB_HERE, "the dark theme is saved");
    CheckEquals(EGUITheme::Dark, LoadGUIThemeFromLayoutFile(path), ASWBCB_HERE, "the theme is kept");
    std::unique_ptr<TMemIniFile> const ini(new TMemIniFile(path));
    TGUILayout const loaded = LoadGUILayout(*ini);
    AssertTrue(loaded.DetailHeight.HasValue(), ASWBCB_HERE, "the layout is kept");
    CheckEquals(123, loaded.DetailHeight.Value(), ASWBCB_HERE, "with its value");

    // Act and Assert: a file and folder that don't exist yet are created.
    UnicodeString const newPath = TPath::Combine(TPath::Combine(m_TempFolder, "NewDark"), "layout.ini");
    SaveGUIDarkThemeToLayoutFile(newPath, "Tablet Dark");
    CheckEquals("Tablet Dark", LoadGUIDarkThemeFromLayoutFile(newPath), ASWBCB_HERE,
        "saved to a new file in a new folder");
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
