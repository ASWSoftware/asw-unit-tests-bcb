/* **************************************************************************
Test_ASWExample_WordCounter.cpp
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

#include <System.hpp>
#pragma hdrstop

//---------------------------------------------------------------------------
// Module header
#include "Test_ASWExample_WordCounter.h"
//---------------------------------------------------------------------------
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWExample_WordCounter
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWExample_WordCounter::TTest_ASWExample_WordCounter()
    : inherited(TSuitePath("ASWExamples"), "ASWExample_WordCounter_Tests"),
      m_Counter(nullptr)
{
    // Nothing to register: the published Test_ methods are found when the group runs. The group is in suite
    // "ASWExamples", so --filter "ASWExamples.*" runs it.
}
//---------------------------------------------------------------------------
__fastcall TTest_ASWExample_WordCounter::~TTest_ASWExample_WordCounter()
{
    delete m_Counter;
}
//---------------------------------------------------------------------------
void TTest_ASWExample_WordCounter::SetUp_Test(ITestCase& /*testCase*/)
{
    delete m_Counter;
    m_Counter = new ASWExample::TWordCounter();
}
//---------------------------------------------------------------------------
void TTest_ASWExample_WordCounter::TearDown_Test(ITestCase& /*testCase*/)
{
    delete m_Counter;
    m_Counter = nullptr;
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWExample_WordCounter::Test_Count_Empty()
{
    CheckEquals(0, m_Counter->Count(), ASWBCB_HERE, "no text yet");

    m_Counter->SetText(L" \t\r\n ");
    CheckEquals(0, m_Counter->Count(), ASWBCB_HERE, "only separators");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWExample_WordCounter::Test_Count_Limit()
{
    m_Counter->SetText(L"one two three four");
    m_Counter->SetLimit(L"3");

    CheckEquals(3, m_Counter->Count(), ASWBCB_HERE, "capped at the limit");

    m_Counter->SetLimit(L"0");
    CheckEquals(4, m_Counter->Count(), ASWBCB_HERE, "0 means no limit");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWExample_WordCounter::Test_Count_NonASCII()
{
    m_Counter->SetText(L"caf\x00E9 cr\x00E8me br\x00FBl\x00E9" L"e");

    CheckEquals(3, m_Counter->Count(), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWExample_WordCounter::Test_Count_Separators()
{
    m_Counter->SetText(L"  one\ttwo\r\nthree  four ");

    CheckEquals(4, m_Counter->Count(), ASWBCB_HERE, "spaces, tabs and line breaks separate words");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWExample_WordCounter::Test_SetLimit_Invalid()
{
    SetExceptionExpected<EConvertError>(ASWBCB_HERE, "not a number", "not a valid integer value");

    m_Counter->SetLimit(L"ten");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWExample_WordCounter::Test_SetLimit_Negative()
{
    SetExceptionExpected<ERangeError>(ASWBCB_HERE, "a negative limit", "can't be negative");

    m_Counter->SetLimit(L"-1");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWExample_WordCounter::Test_SetLimit_Valid()
{
    m_Counter->SetLimit(L"10");

    AssertEquals(10, m_Counter->Limit(), ASWBCB_HERE); // Stops the test on failure
    CheckTrue(m_Counter->Limit() > 0, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWExample_WordCounter::Test_Text_RoundTrip()
{
    m_Counter->SetText(L"caf\x00E9");

    CheckEquals(L"caf\x00E9", m_Counter->Text(), ASWBCB_HERE, "UnicodeString and a wide literal");
    CheckEquals("caf\xC3\xA9", m_Counter->Text(), ASWBCB_HERE, "UnicodeString and UTF-8 text");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP(ASWUnitTestsBCB::TTest_ASWExample_WordCounter)
