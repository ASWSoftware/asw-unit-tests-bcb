/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_Selection.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_GUI_Selection.h.

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
#ifndef Test_ASWUnitTestsBCB_GUI_SelectionH
#define Test_ASWUnitTestsBCB_GUI_SelectionH
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_Selection
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_GUI_Selection : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

public:
    __fastcall TTest_ASWUnitTestsBCB_GUI_Selection();

__published:
    void __fastcall Test_CaptureGUISelection_SavesWhatRunSelectedWouldRun();
    void __fastcall Test_DescribeGUISelectionRestore_NotesMissingAndNewTests();
    void __fastcall Test_DescribeGUISelectionRestore_NotesWhenEveryTestWasChecked();
    void __fastcall Test_FormatGUISelection_LeavesOutNamesItCannotStore();
    void __fastcall Test_FormatGUISelection_RoundTripsThroughParse();
    void __fastcall Test_GUISelectionFilePath_IsBesideTheLayoutFile();
    void __fastcall Test_ParseGUISelection_SkipsLinesItCannotRead();
    void __fastcall Test_RestoreGUISelection_ChecksEveryTestIfNoSavedCheckedTestExists();
    void __fastcall Test_RestoreGUISelection_ChecksNewGroupsOnlyIfEveryTestWasChecked();
    void __fastcall Test_RestoreGUISelection_ChecksNewTestsLikeTheirGroup();
    void __fastcall Test_RestoreGUISelection_DoesNothingWithoutASavedSelection();
    void __fastcall Test_RestoreGUISelection_KeepsASavedSelectionWithNothingChecked();
    void __fastcall Test_RestoreGUISelection_RestoresSavedCheckStates();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_GUI_SelectionH
