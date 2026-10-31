/* **************************************************************************
ASWUnitTestsBCB_GUI_Style.h
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

#ifndef ASWUnitTestsBCB_GUI_StyleH
#define ASWUnitTestsBCB_GUI_StyleH
//---------------------------------------------------------------------------
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_Theme.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

// Sets the VCL style for 'theme' (see GUIThemeIsDark()): 'darkStyle' (see LoadGUIDarkStyle()) when it's dark,
// otherwise the default unstyled look. Returns whether the active style changed. Call it on the main thread; before any
// form exists, each form is created in that style, and afterward it restyles the open ones.
bool ApplyGUITheme(EGUITheme theme, System::UnicodeString const& darkStyle);
// The dark themes this executable has built in, for the View > Theme > Dark Theme menu: GUIBuiltInDarkThemeNames() for
// the VCL styles linked in.
std::vector<std::string> BuiltInGUIDarkThemeNames();
// The VCL style the dark theme 'darkTheme' is, for ApplyGUITheme(): the built-in one of that name, ignoring case (see
// BuiltInGUIDarkThemeNames()); otherwise the style in "<darkTheme>.vsf" in 'themesFolder' (see GUIThemesFolderPath()),
// loaded the first time it's needed. Adds a line for the log to 'problems' (see GUIThemeProblemLine()), and gives
// Carbon, when the file is missing, isn't a VCL style, has the name of a style already loaded from elsewhere, or can't
// be loaded.
System::UnicodeString LoadGUIDarkStyle(System::UnicodeString const& themesFolder, std::string const& darkTheme,
    std::vector<std::string>& problems);

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_GUI_StyleH
