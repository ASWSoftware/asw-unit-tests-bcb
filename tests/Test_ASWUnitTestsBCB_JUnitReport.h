/* **************************************************************************
Test_ASWUnitTestsBCB_JUnitReport.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_JUnitReport.h.

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
#ifndef Test_ASWUnitTestsBCB_JUnitReportH
#define Test_ASWUnitTestsBCB_JUnitReportH
//---------------------------------------------------------------------------
#include <System.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_JUnitReport
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_JUnitReport : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

private:
    System::UnicodeString const m_TempFolder; // Per process, so parallel partitions don't share it

public:
    __fastcall TTest_ASWUnitTestsBCB_JUnitReport();

    virtual void SetUp_Group();
    virtual void TearDown_Group();

__published:
    void __fastcall Test_BuildXML_CountsAndEscapes();
    void __fastcall Test_BuildXML_NoTestCases();
    void __fastcall Test_EscapeXML_PlainTextUnchanged();
    void __fastcall Test_EscapeXML_SpecialCharacters();
    void __fastcall Test_Write_FailsWithoutFolder();
    void __fastcall Test_Write_ProducesReadableReport();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_JUnitReportH
