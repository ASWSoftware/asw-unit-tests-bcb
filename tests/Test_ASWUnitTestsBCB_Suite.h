/* **************************************************************************
Test_ASWUnitTestsBCB_Suite.h
Author: Anthony S. West - ASW Software

Tests for test suites: TSuitePath, a group's suite path and qualified name, suites named by a group's registration,
and how suites reach records, filters, observers, the run order and the handler.

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
#ifndef Test_ASWUnitTestsBCB_SuiteH
#define Test_ASWUnitTestsBCB_SuiteH
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_Suite
//
// Registered in a nested suite, "ASWUnitTestsBCB.Suites", so the
// program's own run includes a group whose registration names its suite.
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_Suite : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

public:
    __fastcall TTest_ASWUnitTestsBCB_Suite();

__published:
    void __fastcall Test_Constructor_InvalidSuitePathKept();
    void __fastcall Test_DuplicateName_ReportsQualifiedName();
    void __fastcall Test_GroupInSuite_FilterMatchesQualifiedName();
    void __fastcall Test_GroupInSuite_ReportsQualifiedName();
    void __fastcall Test_GroupWithoutSuite_KeepsItsOwnName();
    void __fastcall Test_Handler_ListsTestsByFullName();
    void __fastcall Test_Handler_OrderKeepsSuitesTogether();
    void __fastcall Test_Handler_RunsSuiteByFilter();
    void __fastcall Test_Handler_ShuffleKeepsSuitesTogether();
    void __fastcall Test_Initialize_ConstructorInvalidSuitePathThrows();
    void __fastcall Test_Initialize_RegistrationConflictingSuiteThrows();
    void __fastcall Test_Initialize_RegistrationInvalidSuitePathThrows();
    void __fastcall Test_Registration_PutsGroupInSuite();
    void __fastcall Test_Registration_SameSuiteAsConstructor();
    void __fastcall Test_Registration_UncreatableGroupStaysInSuite();
    void __fastcall Test_ThisGroup_IsInItsRegisteredSuite();
    void __fastcall Test_TSuitePath_EmptyNamesAreInvalid();
    void __fastcall Test_TSuitePath_SplitsNames();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_SuiteH
