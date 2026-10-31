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
#include <memory>
//---------------------------------------------------------------------------
#include <System.IniFiles.hpp>
#include <System.SysUtils.hpp>
#include <System.Win.Registry.hpp>
#include <windows.h>
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Inifiles;
using namespace System::Sysutils;
using namespace System::Win::Registry;
//---------------------------------------------------------------------------

namespace
{

char const ThemeKey[] = "Theme";
char const ViewSection[] = "View";

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

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

        std::unique_ptr<TMemIniFile> const ini(new TMemIniFile(layoutPath));
        return LoadGUITheme(*ini);
    }
    catch (...)
    {
        return EGUITheme::Windows;
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
        std::unique_ptr<TMemIniFile> const ini(new TMemIniFile(layoutPath));
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
