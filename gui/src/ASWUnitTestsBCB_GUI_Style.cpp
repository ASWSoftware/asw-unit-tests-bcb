/* **************************************************************************
ASWUnitTestsBCB_GUI_Style.cpp
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
#include "ASWUnitTestsBCB_GUI_Style.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.SysUtils.hpp>
#include <Vcl.Styles.hpp> // Links the unit that registers the engine loading VCL style resources, e.g. Carbon.
#include <Vcl.Themes.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_Theme.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Sysutils;
using namespace Vcl::Themes;
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

//---------------------------------------------------------------------------
/*
    ApplyGUITheme

    TrySetStyle() leaves the style as it is if the dark style's resource isn't linked in (the GUI project's
    Custom_Styles), or the Vcl.Styles unit isn't, which loads it. "Windows" is the default, unstyled look's name.
*/
bool ApplyGUITheme(EGUITheme theme, UnicodeString const& darkStyle)
{
    bool const dark = GUIThemeIsDark(theme, WindowsAppsUseDarkTheme(), WindowsHighContrast());
    UnicodeString const styleName = dark ? darkStyle : UnicodeString("Windows");

    if (SameText(TStyleManager::ActiveStyle->Name, styleName))
        return false;

    return TStyleManager::TrySetStyle(styleName, false);
}
//---------------------------------------------------------------------------
std::vector<std::string> BuiltInGUIDarkThemeNames()
{
    std::vector<std::string> styleNames;
    DynamicArray<UnicodeString> const styles = TStyleManager::StyleNames;

    for (int i = 0; i < styles.Length; ++i)
        styleNames.push_back(ToUTF8(styles[i]));

    return GUIBuiltInDarkThemeNames(styleNames);
}
//---------------------------------------------------------------------------
UnicodeString LoadGUIDarkStyle(std::string const& darkTheme, std::vector<std::string>& problems)
{
    std::string const builtIn = FindGUIBuiltInDarkTheme(BuiltInGUIDarkThemeNames(), darkTheme);

    if (!builtIn.empty())
        return FromUTF8(builtIn);

    problems.push_back(GUIThemeProblemLine(darkTheme, "There's no built-in dark theme of that name, so " +
        GUIDefaultDarkThemeName() + " is used instead."));
    return FromUTF8(GUIDefaultDarkThemeName());
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
