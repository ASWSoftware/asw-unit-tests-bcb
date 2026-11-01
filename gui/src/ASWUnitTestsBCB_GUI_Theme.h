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
#include <string>
#include <vector>
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

// The names of the files (not folders) in the folder, without the folder, converted to UTF-8, in no particular order;
// empty if the folder doesn't exist or can't be listed.
std::vector<std::string> FileNamesInFolder(System::UnicodeString const& folderPath);
// The dark theme 'darkTheme' is among 'builtInNames' (see GUIBuiltInDarkThemeNames()), as that list names it, matching
// it ignoring (ASCII) case; empty if it isn't one of them.
std::string FindGUIBuiltInDarkTheme(std::vector<std::string> const& builtInNames, std::string const& darkTheme);
// The dark themes the VCL GUI runner has built in, for its View > Theme > Dark Theme menu: Carbon (see
// GUIDefaultDarkThemeName()), then each of GUIVCLStyleDarkThemeNames() that's among 'styleNames', the VCL styles linked
// in (the GUI project's Custom_Styles), matching them ignoring (ASCII) case.
std::vector<std::string> GUIBuiltInDarkThemeNames(std::vector<std::string> const& styleNames);
// The name of the default dark theme, "Carbon": RAD Studio's Carbon VCL style, which the GUI project links in.
std::string GUIDefaultDarkThemeName();
// 'text', e.g. a dark theme's name, as a menu item's caption: each '&' doubled, so it shows rather than making the next
// letter the item's accelerator.
std::string GUIMenuItemText(std::string const& text);
// Whether 'theme' is dark: Light and Dark as chosen, and Windows as 'windowsAppsUseDark' (Windows' app mode setting)
// says, except that it's light while 'highContrast' is on, so the window shows high contrast's own colors.
bool GUIThemeIsDark(EGUITheme theme, bool windowsAppsUseDark, bool highContrast);
// The names of the themes among 'fileNames', the files in the Themes folder (see GUIThemesFolderPath()): each one that
// ends in 'extension' (e.g. ".vsf"), ignoring (ASCII) case, without it, sorted ignoring case, and each name once. Leaves
// out 'builtInNames' (see GUIBuiltInDarkThemeNames()), ignoring case, which a file can't replace, and a file named just
// 'extension'.
std::vector<std::string> GUIThemeNames(std::vector<std::string> const& fileNames, std::string const& extension,
    std::vector<std::string> const& builtInNames);
// A line for the log about one of the dark theme 'themeName''s problems: Dark theme "<name>": <problem>
std::string GUIThemeProblemLine(std::string const& themeName, std::string const& problem);
// The folder of the VCL style files, "<name>.vsf", that View > Theme > Dark Theme lists, whatever executable runs the
// GUI: Themes, beside the layout file (see GUILayoutFilePath()), so %APPDATA%\ASWUnitTestsBCB\Themes. It may not exist
// yet.
System::UnicodeString GUIThemesFolderPath();
// The dark themes besides Carbon that the VCL GUI runner has built in when their VCL styles are linked in, sorted:
// RAD Studio's dark VCL styles, by their style names.
std::vector<std::string> GUIVCLStyleDarkThemeNames();
// Reads the dark theme SaveGUIDarkTheme() wrote to 'ini', its [View] section's DarkTheme key: GUIDefaultDarkThemeName()
// when it's missing or empty, or is that name in another case.
std::string LoadGUIDarkTheme(System::Inifiles::TCustomIniFile& ini);
// The dark theme SaveGUIDarkThemeToLayoutFile() saved in the layout file at 'layoutPath', e.g. GUILayoutFilePath(), or
// GUIDefaultDarkThemeName() if there's none or it can't be read.
std::string LoadGUIDarkThemeFromLayoutFile(System::UnicodeString const& layoutPath);
// Reads the theme SaveGUITheme() wrote to 'ini', its [View] section's Theme key, ignoring (ASCII) case. Windows when
// it's missing or isn't a theme's name.
EGUITheme LoadGUITheme(System::Inifiles::TCustomIniFile& ini);
// The theme SaveGUIThemeToLayoutFile() saved in the layout file at 'layoutPath', e.g. GUILayoutFilePath(), or
// Windows if there's none or it can't be read.
EGUITheme LoadGUIThemeFromLayoutFile(System::UnicodeString const& layoutPath);
// Writes 'name', a dark theme's (see LoadGUIDarkTheme()), to 'ini'. For a TMemIniFile, the file itself is only written
// by a later UpdateFile().
void SaveGUIDarkTheme(System::Inifiles::TCustomIniFile& ini, std::string const& name);
// Saves 'name', the dark theme's, in the layout file at 'layoutPath', e.g. GUILayoutFilePath(), keeping its other
// values, and creating it and its folder if needed. Ignores any error, which only means the next start uses the
// default dark theme.
void SaveGUIDarkThemeToLayoutFile(System::UnicodeString const& layoutPath, std::string const& name);
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
