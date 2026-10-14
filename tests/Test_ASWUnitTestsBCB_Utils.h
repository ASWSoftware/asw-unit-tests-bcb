/* **************************************************************************
Test_ASWUnitTestsBCB_Utils.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_Utils.h and ASWUnitTestsBCB_Exception.h.

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
#ifndef Test_ASWUnitTestsBCB_UtilsH
#define Test_ASWUnitTestsBCB_UtilsH
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_Utils
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_Utils : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

public:
    __fastcall TTest_ASWUnitTestsBCB_Utils();

__published:
    void __fastcall Test_BareMethodName_DropsQualification();
    void __fastcall Test_DescribeException_RTLAndStd();
    void __fastcall Test_ExceptionMessages_MatchASWUnitTests();
    void __fastcall Test_FNV1aHash_ReferenceValues();
    void __fastcall Test_FormatDurationMs_ThreeDecimals();
    void __fastcall Test_FormatElapsedTime_Fields();
    void __fastcall Test_FormatLocation_BareMethodAndLine();
    void __fastcall Test_FromUTF8_NonASCII();
    void __fastcall Test_GetUTCTimeISO8601_Format();
    void __fastcall Test_MersenneTwister_IsMT19937();
    void __fastcall Test_NewShuffleSeed_Varies();
    void __fastcall Test_NumbersToString();
    void __fastcall Test_ShuffledIndices_EachIndexOnce();
    void __fastcall Test_ShuffledIndices_SameAsASWUnitTests();
    void __fastcall Test_TCheckMessage_AcceptsEveryStringType();
    void __fastcall Test_TOptional();
    void __fastcall Test_ToUTF8_NonASCII();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_UtilsH
