/* **************************************************************************
Test_ASWUnitTestsBCB_TestBase.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_TestBase.h: discovery, the lifecycle, outcomes, exceptions and assertions, checked by
running small unregistered groups and reading their results.

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
#ifndef Test_ASWUnitTestsBCB_TestBaseH
#define Test_ASWUnitTestsBCB_TestBaseH
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_TestBase
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_TestBase : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

public:
    __fastcall TTest_ASWUnitTestsBCB_TestBase();

__published:
    void __fastcall Test_Asserts_EndTheTest();
    void __fastcall Test_Booleans_ChecksAndAsserts();
    void __fastcall Test_Checks_FailTheTestAtTheEnd();
    void __fastcall Test_Checks_LogAndRecordTheirMessages();
    void __fastcall Test_Empty_ChecksAndAsserts();
    void __fastcall Test_Equals_EnumsAndPointers();
    void __fastcall Test_EqualsIC_ChecksAndAsserts();
    void __fastcall Test_Exceptions_ExpectedAnyType();
    void __fastcall Test_Exceptions_ExpectedMessage();
    void __fastcall Test_Exceptions_ExpectedType();
    void __fastcall Test_Exceptions_Unexpected();
    void __fastcall Test_FailAndSkip();
    void __fastcall Test_FindPublishedTests_DeclarationOrderAndPrefix();
    void __fastcall Test_FindPublishedTests_DuplicateNameThrows();
    void __fastcall Test_FindPublishedTests_InheritedFirstAndOverrides();
    void __fastcall Test_FindPublishedTests_NoPublishedMethods();
    void __fastcall Test_GetTestCallbackList_RunsMostDerivedCode();
    void __fastcall Test_Matches_ChecksAndAsserts();
    void __fastcall Test_Memory_ChecksAndAsserts();
    void __fastcall Test_Near_ChecksAndAsserts();
    void __fastcall Test_NotEquals_ChecksAndAsserts();
    void __fastcall Test_Objects_ChecksAndAsserts();
    void __fastcall Test_Ordering_ChecksAndAsserts();
    void __fastcall Test_RegisterTestCases_DuplicateNameThrows();
    void __fastcall Test_RegisterTestCases_RowsRunAfterPublished();
    void __fastcall Test_Run_CatchCrashes_IllegalInstruction();
    void __fastcall Test_Run_CatchCrashes_RecordsTheSame();
    void __fastcall Test_Run_CatchCrashes_StackOverflowEndsTheRun();
    void __fastcall Test_Run_FatalErrorsEndTheRun();
    void __fastcall Test_Run_LifecycleOrder();
    void __fastcall Test_Run_LogLines();
    void __fastcall Test_Run_ObserverEventsAndStop();
    void __fastcall Test_Run_RecordsEachOutcome();
    void __fastcall Test_Run_RecordsShortDurations();
    void __fastcall Test_Run_SetUpGroupThrows_FailsEachTest();
    void __fastcall Test_Run_Timeout_AbandonsHungTest();
    void __fastcall Test_Run_Timeout_FatalErrorsEndTheRun();
    void __fastcall Test_Run_Timeout_IgnoresLateFinish();
    void __fastcall Test_Run_Timeout_KeepsTheFloatingPointEnvironment();
    void __fastcall Test_Run_Timeout_RecordsTheSame();
    void __fastcall Test_Run_Timeout_RunsOnAWorkerThread();
    void __fastcall Test_TextParts_ChecksAndAsserts();
    void __fastcall Test_Throws_ChecksAndAsserts();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_TestBaseH
