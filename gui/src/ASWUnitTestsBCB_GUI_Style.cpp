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
#include <utility>
#include <vector>
//---------------------------------------------------------------------------
#include <System.IOUtils.hpp>
#include <System.SysUtils.hpp>
#include <Vcl.Styles.hpp> // Links the unit that registers the engine loading VCL style resources, e.g. Carbon.
#include <Vcl.Themes.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_Theme.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Ioutils;
using namespace System::Sysutils;
using namespace Vcl::Themes;
//---------------------------------------------------------------------------

namespace
{

// Each VCL style file LoadGUIDarkStyle() has loaded, by its path in lowercase, and the name of its style. Not a
// function-local static, which bcc32 warns about (W8104) since it has a constructor.
std::vector<std::pair<UnicodeString, UnicodeString> > LoadedStyleFiles;

} // namespace

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
/*
    LoadGUIDarkStyle

    A style, once loaded, stays registered for the process, under the name the file gives it, and can't be loaded
    again, so a file loaded before gives the style it loaded, and a change to it shows after a restart. A file whose
    style has the name of one already registered, e.g. Carbon's own file renamed, can't be loaded either. A built-in
    style that isn't linked in is looked for as a file.
*/
UnicodeString LoadGUIDarkStyle(UnicodeString const& themesFolder, std::string const& darkTheme,
    std::vector<std::string>& problems)
{
    std::string const builtIn = FindGUIBuiltInDarkTheme(BuiltInGUIDarkThemeNames(), darkTheme);

    if (!builtIn.empty())
        return FromUTF8(builtIn);

    std::string const carbon = GUIDefaultDarkThemeName();
    std::string problem;

    try
    {
        UnicodeString const path = TPath::Combine(themesFolder, FromUTF8(darkTheme) + ".vsf");
        UnicodeString const key = AnsiLowerCase(path);

        for (std::vector<std::pair<UnicodeString, UnicodeString> >::const_iterator it = LoadedStyleFiles.begin();
             it != LoadedStyleFiles.end(); ++it)
        {
            if (it->first == key)
                return it->second;
        }

        TStyleInfo info;

        if (!FileExists(path))
        {
            problem = "There's no style file " + ToUTF8(path) + ", so " + carbon + " is used instead.";
        }
        else if (!TStyleManager::IsValidStyle(path, info))
        {
            problem = "Its file isn't a VCL style file, so " + carbon + " is used instead.";
        }
        else
        {
            DynamicArray<UnicodeString> const names = TStyleManager::StyleNames;

            for (int i = 0; (i < names.Length) && problem.empty(); ++i)
            {
                if (SameText(names[i], info.Name))
                {
                    problem = "Its style is named " + ToUTF8(info.Name) + ", as a style already loaded is, so " +
                        carbon + " is used instead. Give it a name of its own in RAD Studio's Bitmap Style Designer.";
                }
            }

            if (problem.empty())
            {
                TStyleManager::LoadFromFile(path);
                LoadedStyleFiles.push_back(std::make_pair(key, info.Name));
                return info.Name;
            }
        }
    }
    catch (Exception const& exception)
    {
        problem = "Its style file couldn't be loaded, so " + carbon + " is used instead: " + ToUTF8(exception.Message);
    }

    problems.push_back(GUIThemeProblemLine(darkTheme, problem));
    return FromUTF8(carbon);
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
