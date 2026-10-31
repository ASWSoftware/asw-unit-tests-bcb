/* **************************************************************************
ASWUnitTestsBCB_GUI_Theme.h
Author: Anthony S. West - ASW Software

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

#ifndef ASWUnitTestsBCB_GUI_ThemeH
#define ASWUnitTestsBCB_GUI_ThemeH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <System.IniFiles.hpp>
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// EGUITheme
//
// The VCL GUI runner's theme, as chosen in its View > Theme menu and saved
// in its layout file. Contains no VCL UI code, so it's unit tested from the
// console runner (see tests/Test_ASWUnitTestsBCB_GUI_Theme.cpp).
/////////////////////////////////////////////////////////////////////////////
enum class EGUITheme
{
    Windows, // Follows Windows' setting for apps, light or dark; the default.
    Light,
    Dark,
};

//---------------------------------------------------------------------------

// Whether 'theme' is dark: Light and Dark as chosen, and Windows as 'windowsAppsUseDark' (Windows' app mode setting)
// says, except that it's light while 'highContrast' is on, so the window shows high contrast's own colors.
bool GUIThemeIsDark(EGUITheme theme, bool windowsAppsUseDark, bool highContrast);
// Reads the theme SaveGUITheme() wrote to 'ini', its [View] section's Theme key, ignoring (ASCII) case. Windows when
// it's missing or isn't a theme's name.
EGUITheme LoadGUITheme(System::Inifiles::TCustomIniFile& ini);
// The theme SaveGUIThemeToLayoutFile() saved in the layout file at 'layoutPath', e.g. GUILayoutFilePath(), or
// Windows if there's none or it can't be read.
EGUITheme LoadGUIThemeFromLayoutFile(System::UnicodeString const& layoutPath);
// Writes 'theme' to 'ini', as "Windows", "Light" or "Dark". For a TMemIniFile, the file itself is only written by a
// later UpdateFile().
void SaveGUITheme(System::Inifiles::TCustomIniFile& ini, EGUITheme theme);
// Saves 'theme' in the layout file at 'layoutPath', e.g. GUILayoutFilePath(), keeping its other values, and creating
// it and its folder if needed. Ignores any error, which only means the next start uses the default theme.
void SaveGUIThemeToLayoutFile(System::UnicodeString const& layoutPath, EGUITheme theme);
// Whether Windows' app mode setting is dark; false if it can't be read, as on a Windows version without it.
bool WindowsAppsUseDarkTheme();
// Whether Windows' high contrast is on.
bool WindowsHighContrast();

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_GUI_ThemeH
