/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_TestList.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_GUI_TestList.h.

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
#ifndef Test_ASWUnitTestsBCB_GUI_TestListH
#define Test_ASWUnitTestsBCB_GUI_TestListH
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_TestList
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_GUI_TestList : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

public:
    __fastcall TTest_ASWUnitTestsBCB_GUI_TestList();

__published:
    void __fastcall Test_CheckedFilter_MatchesExactlyTheCheckedTests();
    void __fastcall Test_CheckMatching_ChecksExactlyTheMatches();
    void __fastcall Test_CheckOnlyFailed_ChecksVisibleFailuresOnly();
    void __fastcall Test_CountMatching_CountsFilterMatches();
    void __fastcall Test_DetailText_DescribesAFinishedTest();
    void __fastcall Test_DetailText_DescribesATestThatHasNotRun();
    void __fastcall Test_FailedFilter_MatchesEveryFailedTestEvenHiddenOnes();
    void __fastcall Test_GroupDetailText_CountsEachStatus();
    void __fastcall Test_GroupStatus_ShowsRunningThenWorstOutcome();
    void __fastcall Test_GroupSuitePath_GivesEachGroupsSuite();
    void __fastcall Test_IndexOf_FindsTestsByGroupAndTestName();
    void __fastcall Test_Load_ChecksEveryTestAndListsGroupsInOrder();
    void __fastcall Test_Load_ListsEachSuiteAfterTheSuitesHoldingIt();
    void __fastcall Test_NameInSuite_DropsTheSuitePath();
    void __fastcall Test_ParentSuitePath_DropsTheLastSuiteName();
    void __fastcall Test_ResetResults_SetsEveryTestBackToNotRun();
    void __fastcall Test_SetAllChecked_ChecksOrUnchecksEveryTest();
    void __fastcall Test_SetChecked_UpdatesItsGroupsCheckState();
    void __fastcall Test_SetGroupChecked_ChangesOnlyThatGroup();
    void __fastcall Test_SetResult_StoresTheResultAndSetsTheStatus();
    void __fastcall Test_SetSuiteChecked_ChangesTheSuiteAndTheSuitesInIt();
    void __fastcall Test_SetVisibleFilter_HidesNonMatchingTests();
    void __fastcall Test_SetVisibleFilter_LimitsCheckingToVisibleTests();
    void __fastcall Test_StatusCount_CountsTestsWithEachStatus();
    void __fastcall Test_StatusFromOutcome_MapsEachOutcome();
    void __fastcall Test_StatusName_NamesEachStatus();
    void __fastcall Test_SuiteDetailText_CountsEachStatus();
    void __fastcall Test_SuiteStatus_IncludesTheSuitesInIt();
    void __fastcall Test_TestsInSuite_IncludesNestedSuitesOnly();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_GUI_TestListH
