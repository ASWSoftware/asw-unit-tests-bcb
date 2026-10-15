/* **************************************************************************
Test_ASWUnitTestsBCB_Handler.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_Handler.h and ASWUnitTestsBCB_Registry.h, run on lists of unregistered groups.

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
#ifndef Test_ASWUnitTestsBCB_HandlerH
#define Test_ASWUnitTestsBCB_HandlerH
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_Handler
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_Handler : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

public:
    __fastcall TTest_ASWUnitTestsBCB_Handler();

__published:
    void __fastcall Test_CanonicalGroupOrder_KeepsSuitesTogether();
    void __fastcall Test_CanonicalGroupOrder_RejectsInvalidSuitePath();
    void __fastcall Test_CanonicalGroupOrder_WithoutSuites_SortsByOrderThenName();
    void __fastcall Test_GetAllTestFullNames();
    void __fastcall Test_GetTests_NamesApart();
    void __fastcall Test_Initialize_ConstructorThrows_FailsItsTests();
    void __fastcall Test_Initialize_DuplicateTestNameThrows();
    void __fastcall Test_Initialize_LogLines();
    void __fastcall Test_Initialize_NoGroups();
    void __fastcall Test_Initialize_OrderThenName();
    void __fastcall Test_ListTests_Filtered();
    void __fastcall Test_ListTests_Unfiltered();
    void __fastcall Test_PatternTestFilter();
    void __fastcall Test_Registry_HasThisProgramsGroups();
    void __fastcall Test_Run_CatchCrashes_StackOverflowEndsTheRun();
    void __fastcall Test_Run_CombinesResults();
    void __fastcall Test_Run_FatalErrorEndsTheRun();
    void __fastcall Test_Run_Filter_MatchesNothing();
    void __fastcall Test_Run_Filter_OnlyMatchingTests();
    void __fastcall Test_Run_Filter_UncreatableGroup();
    void __fastcall Test_Run_LogLines();
    void __fastcall Test_Run_Shuffle_KeepsOrderWhenFiltered();
    void __fastcall Test_Run_Shuffle_LogsTheSeed();
    void __fastcall Test_Run_Shuffle_OrderFromTheSeed();
    void __fastcall Test_Run_Timeout_EndsTheRun();
    void __fastcall Test_SetRunObserver_StopsBetweenGroups();
    void __fastcall Test_ShuffledGroupOrder_KeepsSuitesTogether();
    void __fastcall Test_ShuffledGroupOrder_WithoutSuites_MatchesShuffledIndices();
    void __fastcall Test_WildcardMatch();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_HandlerH
