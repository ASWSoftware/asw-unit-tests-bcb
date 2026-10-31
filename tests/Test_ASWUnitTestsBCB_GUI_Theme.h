/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_Theme.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_GUI_Theme.h.

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

//---------------------------------------------------------------------------
#ifndef Test_ASWUnitTestsBCB_GUI_ThemeH
#define Test_ASWUnitTestsBCB_GUI_ThemeH
//---------------------------------------------------------------------------
#include <System.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_Theme
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_GUI_Theme : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

private:
    System::UnicodeString const m_TempFolder; // Per process, so parallel partitions don't share it

public:
    __fastcall TTest_ASWUnitTestsBCB_GUI_Theme();

    virtual void SetUp_Group();
    virtual void TearDown_Group();

__published:
    void __fastcall Test_FindGUIBuiltInDarkTheme_MatchesIgnoringCase();
    void __fastcall Test_GUIBuiltInDarkThemeNames_CarbonThenLinkedStyles();
    void __fastcall Test_GUIMenuItemText_DoublesAmpersands();
    void __fastcall Test_GUIThemeIsDark_ChosenThemeIgnoresWindows();
    void __fastcall Test_GUIThemeIsDark_WindowsFollowsAppModeUnlessHighContrast();
    void __fastcall Test_GUIThemeProblemLine_NamesTheTheme();
    void __fastcall Test_GUIVCLStyleDarkThemeNames_SortedWithoutCarbon();
    void __fastcall Test_LoadGUIDarkTheme_CarbonWhenMissingEmptyOrCarbon();
    void __fastcall Test_LoadGUIDarkTheme_KeepsOtherNames();
    void __fastcall Test_LoadGUIDarkThemeFromLayoutFile_CarbonWithoutAFile();
    void __fastcall Test_LoadGUITheme_IgnoresCase();
    void __fastcall Test_LoadGUITheme_WindowsWhenMissingOrUnknown();
    void __fastcall Test_LoadGUIThemeFromLayoutFile_WindowsWithoutAFile();
    void __fastcall Test_SaveGUIDarkThemeToLayoutFile_KeepsTheThemeAndLayout();
    void __fastcall Test_SaveGUITheme_RoundTrips();
    void __fastcall Test_SaveGUIThemeToLayoutFile_KeepsTheLayout();
    void __fastcall Test_WindowsAppsUseDarkTheme_MatchesTheRegistry();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_GUI_ThemeH
