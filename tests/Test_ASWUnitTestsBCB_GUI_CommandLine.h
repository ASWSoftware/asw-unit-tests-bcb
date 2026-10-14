/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_CommandLine.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_GUI_CommandLine.h.

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
#ifndef Test_ASWUnitTestsBCB_GUI_CommandLineH
#define Test_ASWUnitTestsBCB_GUI_CommandLineH
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_CommandLine
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_GUI_CommandLine : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

public:
    __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine();

__published:
    void __fastcall Test_HelpText_MatchesHelpOption();
    void __fastcall Test_Parse_ConsoleOnlyOptionsAreReportedAsIgnored();
    void __fastcall Test_Parse_ExitWithoutRunIsInvalid();
    void __fastcall Test_Parse_HelpIncludesGUIOptions();
    void __fastcall Test_Parse_LayoutOptionsAreGUIOnlyOptions();
    void __fastcall Test_Parse_NoArgumentsUsesDefaults();
    void __fastcall Test_Parse_PassesConsoleOptionsThrough();
    void __fastcall Test_Parse_RunAndExitAreGUIOnlyOptions();
    void __fastcall Test_Parse_UnrecognizedOptionIsInvalid();
    void __fastcall Test_Parse_VersionShowsTheVersion();
    void __fastcall Test_ProcessArguments_MatchesParamCount();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_GUI_CommandLineH
