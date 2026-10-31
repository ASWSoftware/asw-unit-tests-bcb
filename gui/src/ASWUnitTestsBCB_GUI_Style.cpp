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
#include <System.SysUtils.hpp>
#include <Vcl.Styles.hpp> // Links the unit that registers the engine loading VCL style resources, e.g. Carbon.
#include <Vcl.Themes.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_Theme.h"
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

    TrySetStyle() leaves the style as it is if Carbon's resource isn't linked in (the GUI project's Custom_Styles), or
    the Vcl.Styles unit isn't, which loads it. "Windows" is the default, unstyled look's name.
*/
bool ApplyGUITheme(EGUITheme theme)
{
    bool const dark = GUIThemeIsDark(theme, WindowsAppsUseDarkTheme(), WindowsHighContrast());
    UnicodeString const styleName = dark ? UnicodeString("Carbon") : UnicodeString("Windows");

    if (SameText(TStyleManager::ActiveStyle->Name, styleName))
        return false;

    return TStyleManager::TrySetStyle(styleName, false);
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
