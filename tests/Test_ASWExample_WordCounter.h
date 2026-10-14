/* **************************************************************************
Test_ASWExample_WordCounter.h
Author: Anthony S. West - ASW Software

Example test group for the example code in toTest/.

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
#ifndef Test_ASWExample_WordCounterH
#define Test_ASWExample_WordCounterH
//---------------------------------------------------------------------------
#include "ASWExample_WordCounter.h"
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWExample_WordCounter
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWExample_WordCounter : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

private:
    ASWExample::TWordCounter* m_Counter;

protected:
    virtual void SetUp_Test(ITestCase& testCase);
    virtual void TearDown_Test(ITestCase& testCase);

public:
    __fastcall TTest_ASWExample_WordCounter();
    __fastcall virtual ~TTest_ASWExample_WordCounter();

__published: // Tests: every "void __fastcall Test_<Name>()" here runs as the test <Name>
    void __fastcall Test_Count_Empty();
    void __fastcall Test_Count_Limit();
    void __fastcall Test_Count_NonASCII();
    void __fastcall Test_Count_Separators();
    void __fastcall Test_SetLimit_Invalid();
    void __fastcall Test_SetLimit_Negative();
    void __fastcall Test_SetLimit_Valid();
    void __fastcall Test_Text_RoundTrip();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWExample_WordCounterH
