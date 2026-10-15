/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_Layout.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_GUI_Layout.h.

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
#ifndef Test_ASWUnitTestsBCB_GUI_LayoutH
#define Test_ASWUnitTestsBCB_GUI_LayoutH
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_Layout
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_GUI_Layout : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

public:
    __fastcall TTest_ASWUnitTestsBCB_GUI_Layout();

__published:
    void __fastcall Test_GUILayoutFilePath_IsPerExeUnderAppData();
    void __fastcall Test_IsWindowReachable_FalseOffScreen();
    void __fastcall Test_IsWindowReachable_FalseWhenTitleBarIsHidden();
    void __fastcall Test_IsWindowReachable_TrueOnAWorkArea();
    void __fastcall Test_LoadGUILayout_EmptyIniHasNoValues();
    void __fastcall Test_LoadGUILayout_IgnoresInvalidValues();
    void __fastcall Test_SaveGUILayout_RemovesMissingValues();
    void __fastcall Test_SaveGUILayout_RoundTrips();
    void __fastcall Test_ScaleFrom96_ScalesAndRounds();
    void __fastcall Test_ScaleTo96_ReversesScaleFrom96();
    void __fastcall Test_ScreenToWorkspace_ReversesWorkspaceToScreen();
    void __fastcall Test_WorkspaceToScreen_OffsetsByPrimaryWorkArea();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_GUI_LayoutH
