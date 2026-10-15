/* **************************************************************************
Test_ASWUnitTestsBCB_CLI.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_CLI.h.

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
#ifndef Test_ASWUnitTestsBCB_CLIH
#define Test_ASWUnitTestsBCB_CLIH
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_CLI
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_CLI : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

public:
    __fastcall TTest_ASWUnitTestsBCB_CLI();

__published:
    void __fastcall Test_BuildTestFilter_Partition();
    void __fastcall Test_BuildTestFilter_Pattern();
    void __fastcall Test_ExitCodeForResults_Priority();
    void __fastcall Test_ParseArguments_CatchCrashes();
    void __fastcall Test_ParseArguments_Color();
    void __fastcall Test_ParseArguments_Defaults();
    void __fastcall Test_ParseArguments_Filter();
    void __fastcall Test_ParseArguments_HelpAndVersion();
    void __fastcall Test_ParseArguments_InvalidOption();
    void __fastcall Test_ParseArguments_List();
    void __fastcall Test_ParseArguments_Partition();
    void __fastcall Test_ParseArguments_Pause();
    void __fastcall Test_ParseArguments_ProjectName();
    void __fastcall Test_ParseArguments_ReportJUnit();
    void __fastcall Test_ParseArguments_Shuffle();
    void __fastcall Test_ParseArguments_TestTimeout();
    void __fastcall Test_ParseColorMode();
    void __fastcall Test_ParseUnsignedInt();
    void __fastcall Test_SplitCommandLine_CRuntimeRules();
    void __fastcall Test_ToJUnitTestCases_CopiesEachRecord();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_CLIH
