/* **************************************************************************
ASWUnitTestsBCB_GUI_Theme.cpp
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
#include "ASWUnitTestsBCB_GUI_Theme.h"
//---------------------------------------------------------------------------
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.IniFiles.hpp>
#include <System.IOUtils.hpp>
#include <System.SysUtils.hpp>
#include <System.Win.Registry.hpp>
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_Layout.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Inifiles;
using namespace System::Ioutils;
using namespace System::Sysutils;
using namespace System::Win::Registry;
//---------------------------------------------------------------------------

namespace
{

char const DarkThemeKey[] = "DarkTheme";
char const ThemeKey[] = "Theme";
char const ViewSection[] = "View";

//---------------------------------------------------------------------------

bool ContainsName(std::vector<std::string> const& names, std::string const& name);
bool LessName(std::string const& a, std::string const& b);
bool SameName(std::string const& a, std::string const& b);

//---------------------------------------------------------------------------
/*
    ContainsName

    Whether 'names' has 'name', ignoring case as SameName() does.
*/
bool ContainsName(std::vector<std::string> const& names, std::string const& name)
{
    for (std::vector<std::string>::const_iterator it = names.begin(); it != names.end(); ++it)
    {
        if (SameName(*it, name))
            return true;
    }

    return false;
}
//---------------------------------------------------------------------------
/*
    LessName

    Whether theme name 'a' sorts before 'b', both UTF-8, ignoring the case of ASCII letters only, as CompareText() does.
*/
bool LessName(std::string const& a, std::string const& b)
{
    return CompareText(ASWUnitTestsBCB::FromUTF8(a), ASWUnitTestsBCB::FromUTF8(b)) < 0;
}
//---------------------------------------------------------------------------
/*
    SameName

    Whether two theme names, UTF-8, are the same, ignoring the case of ASCII letters only, as SameText() does.
*/
bool SameName(std::string const& a, std::string const& b)
{
    return SameText(ASWUnitTestsBCB::FromUTF8(a), ASWUnitTestsBCB::FromUTF8(b));
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

//---------------------------------------------------------------------------
std::vector<std::string> FileNamesInFolder(UnicodeString const& folderPath)
{
    std::vector<std::string> names;

    try
    {
        if (!DirectoryExists(folderPath))
            return names;

        DynamicArray<UnicodeString> const files = TDirectory::GetFiles(folderPath);

        for (int i = 0; i < files.Length; ++i)
            names.push_back(ToUTF8(ExtractFileName(files[i])));
    }
    catch (...)
    {
        names.clear();
    }

    return names;
}
//---------------------------------------------------------------------------
std::string FindGUIBuiltInDarkTheme(std::vector<std::string> const& builtInNames, std::string const& darkTheme)
{
    for (std::vector<std::string>::const_iterator it = builtInNames.begin(); it != builtInNames.end(); ++it)
    {
        if (SameName(*it, darkTheme))
            return *it;
    }

    return std::string();
}
//---------------------------------------------------------------------------
/*
    GUIBuiltInDarkThemeNames

    Carbon is listed even if it isn't linked in, since it's the default; choosing it then shows the light look, as Dark
    does.
*/
std::vector<std::string> GUIBuiltInDarkThemeNames(std::vector<std::string> const& styleNames)
{
    std::vector<std::string> names(1, GUIDefaultDarkThemeName());
    std::vector<std::string> const candidates = GUIVCLStyleDarkThemeNames();

    for (std::vector<std::string>::const_iterator it = candidates.begin(); it != candidates.end(); ++it)
    {
        if (!FindGUIBuiltInDarkTheme(styleNames, *it).empty())
            names.push_back(*it);
    }

    return names;
}
//---------------------------------------------------------------------------
std::string GUIDefaultDarkThemeName()
{
    return "Carbon";
}
//---------------------------------------------------------------------------
std::string GUIMenuItemText(std::string const& text)
{
    std::string caption;

    for (std::string::const_iterator it = text.begin(); it != text.end(); ++it)
    {
        if (*it == '&')
            caption += '&';

        caption += *it;
    }

    return caption;
}
//---------------------------------------------------------------------------
bool GUIThemeIsDark(EGUITheme theme, bool windowsAppsUseDark, bool highContrast)
{
    switch (theme)
    {
        case EGUITheme::Dark:
            return true;

        case EGUITheme::Light:
            return false;

        case EGUITheme::Windows:
            break;
    }

    return windowsAppsUseDark && !highContrast;
}
//---------------------------------------------------------------------------
std::vector<std::string> GUIThemeNames(std::vector<std::string> const& fileNames, std::string const& extension,
    std::vector<std::string> const& builtInNames)
{
    std::vector<std::string> names;

    for (std::vector<std::string>::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it)
    {
        if ((it->size() <= extension.size()) || !SameName(it->substr(it->size() - extension.size()), extension))
            continue;

        std::string const name = it->substr(0, it->size() - extension.size());

        if (!ContainsName(names, name) && !ContainsName(builtInNames, name))
            names.push_back(name);
    }

    // No two names are the same ignoring case, so no two sort equal, and std::sort's order is the only one (bcc32 warns
    // about std::stable_sort).
    std::sort(names.begin(), names.end(), &LessName);
    return names;
}
//---------------------------------------------------------------------------
std::string GUIThemeProblemLine(std::string const& themeName, std::string const& problem)
{
    return "Dark theme \"" + themeName + "\": " + problem;
}
//---------------------------------------------------------------------------
UnicodeString GUIThemesFolderPath()
{
    return TPath::Combine(ExtractFileDir(GUILayoutFilePath()), "Themes");
}
//---------------------------------------------------------------------------
std::vector<std::string> GUIVCLStyleDarkThemeNames()
{
    std::vector<std::string> names;
    names.push_back("Charcoal Dark Slate");
    names.push_back("Cobalt XEMedia");
    names.push_back("Onyx Blue");
    names.push_back("Tablet Dark");
    return names;
}
//---------------------------------------------------------------------------
std::string LoadGUIDarkTheme(TCustomIniFile& ini)
{
    std::string const name = ToUTF8(ini.ReadString(ViewSection, DarkThemeKey, UnicodeString()));

    if (name.empty() || SameName(name, GUIDefaultDarkThemeName()))
        return GUIDefaultDarkThemeName();

    return name;
}
//---------------------------------------------------------------------------
std::string LoadGUIDarkThemeFromLayoutFile(UnicodeString const& layoutPath)
{
    try
    {
        if (!FileExists(layoutPath))
            return GUIDefaultDarkThemeName();

        std::unique_ptr<TMemIniFile> const ini(NewGUILayoutFile(layoutPath));
        return LoadGUIDarkTheme(*ini);
    }
    catch (...)
    {
        return GUIDefaultDarkThemeName();
    }
}
//---------------------------------------------------------------------------
EGUITheme LoadGUITheme(TCustomIniFile& ini)
{
    UnicodeString const name = ini.ReadString(ViewSection, ThemeKey, UnicodeString());

    // SameText() ignores the case of ASCII letters only.
    if (SameText(name, "Dark"))
        return EGUITheme::Dark;

    if (SameText(name, "Light"))
        return EGUITheme::Light;

    return EGUITheme::Windows;
}
//---------------------------------------------------------------------------
EGUITheme LoadGUIThemeFromLayoutFile(UnicodeString const& layoutPath)
{
    try
    {
        if (!FileExists(layoutPath))
            return EGUITheme::Windows;

        std::unique_ptr<TMemIniFile> const ini(NewGUILayoutFile(layoutPath));
        return LoadGUITheme(*ini);
    }
    catch (...)
    {
        return EGUITheme::Windows;
    }
}
//---------------------------------------------------------------------------
void SaveGUIDarkTheme(TCustomIniFile& ini, std::string const& name)
{
    ini.WriteString(ViewSection, DarkThemeKey, FromUTF8(name));
}
//---------------------------------------------------------------------------
void SaveGUIDarkThemeToLayoutFile(UnicodeString const& layoutPath, std::string const& name)
{
    try
    {
        ForceDirectories(ExtractFileDir(layoutPath));

        // Reading the file's current values first keeps the layout and any other values in it.
        std::unique_ptr<TMemIniFile> const ini(NewGUILayoutFile(layoutPath));
        SaveGUIDarkTheme(*ini, name);
        ini->UpdateFile();
    }
    catch (...)
    {
        // Not saving it (e.g. a read-only profile) only means the next start uses the default dark theme.
    }
}
//---------------------------------------------------------------------------
void SaveGUITheme(TCustomIniFile& ini, EGUITheme theme)
{
    char const* name = "Windows";

    if (theme == EGUITheme::Dark)
        name = "Dark";
    else if (theme == EGUITheme::Light)
        name = "Light";

    ini.WriteString(ViewSection, ThemeKey, name);
}
//---------------------------------------------------------------------------
void SaveGUIThemeToLayoutFile(UnicodeString const& layoutPath, EGUITheme theme)
{
    try
    {
        ForceDirectories(ExtractFileDir(layoutPath));

        // Reading the file's current values first keeps the layout and any other values in it.
        std::unique_ptr<TMemIniFile> const ini(NewGUILayoutFile(layoutPath));
        SaveGUITheme(*ini, theme);
        ini->UpdateFile();
    }
    catch (...)
    {
        // Not saving it (e.g. a read-only profile) only means the next start uses the default theme.
    }
}
//---------------------------------------------------------------------------
bool WindowsAppsUseDarkTheme()
{
    try
    {
        std::unique_ptr<TRegistry> const registry(new TRegistry(KEY_READ));
        registry->RootKey = HKEY_CURRENT_USER;

        if (registry->OpenKeyReadOnly("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize") &&
            registry->ValueExists("AppsUseLightTheme"))
        {
            return registry->ReadInteger("AppsUseLightTheme") == 0;
        }
    }
    catch (Exception&)
    {
        // Not readable, e.g. not a DWORD: light, as when it's missing.
    }

    return false;
}
//---------------------------------------------------------------------------
bool WindowsHighContrast()
{
    HIGHCONTRASTW highContrast;
    ::ZeroMemory(&highContrast, sizeof(highContrast));
    highContrast.cbSize = sizeof(highContrast);

    return (::SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, 0) != FALSE) &&
        ((highContrast.dwFlags & HCF_HIGHCONTRASTON) != 0);
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
