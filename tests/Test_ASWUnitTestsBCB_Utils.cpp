/* **************************************************************************
Test_ASWUnitTestsBCB_Utils.cpp
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

#include <System.hpp>
#pragma hdrstop

//---------------------------------------------------------------------------
// Module header
#include "Test_ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Exception.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

std::string Joined(std::vector<std::size_t> const& indices);

//---------------------------------------------------------------------------
/*
    Joined

    Returns 'indices' as text, e.g. "1,3,0", for readable failure messages.
*/
std::string Joined(std::vector<std::size_t> const& indices)
{
    std::string text;

    for (std::size_t i = 0; i < indices.size(); ++i)
        text += ((i > 0) ? "," : "") + ASWUnitTestsBCB::UIntToString(static_cast<unsigned int>(indices[i]));

    return text;
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_Utils
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_Utils::TTest_ASWUnitTestsBCB_Utils()
    : inherited("ASWUnitTestsBCB_Utils_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_BareMethodName_DropsQualification()
{
    CheckEquals("Test_X", BareMethodName("Test_X"), ASWBCB_HERE, "already bare");
    CheckEquals("Test_X", BareMethodName("TGroup::Test_X"), ASWBCB_HERE, "bcc32's __FUNC__");
    CheckEquals("Test_X", BareMethodName("NS::TGroup::Test_X"), ASWBCB_HERE, "with a namespace");
    CheckEquals("Tmpl<A::B>", BareMethodName("TGroup::Tmpl<A::B>"), ASWBCB_HERE, "\"::\" in template arguments");
    CheckEquals("", BareMethodName(""), ASWBCB_HERE, "empty");

    // __FUNC__ here is "TTest_ASWUnitTestsBCB_Utils::Test_..." on bcc32 and bare on the Clang compilers.
    CheckEquals("Test_BareMethodName_DropsQualification", BareMethodName(__FUNC__), ASWBCB_HERE, __FUNC__);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_DescribeException_RTLAndStd()
{
    std::string rtlDescription;
    std::string rtlMessage;

    try
    {
        StrToInt(L"x");
    }
    catch (Exception& rtlEx)
    {
        rtlDescription = DescribeException(rtlEx);
        rtlMessage = RTLExceptionMessage(rtlEx);
    }

    CheckEquals("EConvertError: 'x' is not a valid integer value", rtlDescription, ASWBCB_HERE);
    CheckEquals("'x' is not a valid integer value", rtlMessage, ASWBCB_HERE);
    CheckEquals("boom", DescribeException(std::runtime_error("boom")), ASWBCB_HERE, "a std::exception's what()");
    CheckEquals("an object that is neither a std::exception nor an RTL Exception", DescribeUnknownException(),
        ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_ExceptionMessages_MatchASWUnitTests()
{
    CheckEquals("Values not equal: Test_X (42): Expected: \"1\" but was \"2\". why",
        TExceptEquals("TGroup::Test_X", 42, "1", "2", "why").what(), ASWBCB_HERE);
    CheckEquals("Expected true but was false: Test_X (7): m", TExceptTrue("Test_X", 7, "m").what(), ASWBCB_HERE);
    CheckEquals("Expected false but was true: Test_X (7): m", TExceptFalse("Test_X", 7, "m").what(), ASWBCB_HERE);
    CheckEquals("Failed: Test_X (7): m", TExceptFail("Test_X", 7, "m").what(), ASWBCB_HERE);
    CheckEquals("Test skipped: Test_X (7): m", TExceptSkipped("Test_X", 7, "m").what(), ASWBCB_HERE);
    CheckEquals("Exception expected: m", TExceptExpected("m").what(), ASWBCB_HERE);
    CheckEquals("m", TExceptAbortRun("m").what(), ASWBCB_HERE);
    CheckEquals("Test group class \"TG\" publishes more than one method named \"Test_X\"",
        TExceptDuplicateTestName("TG", "Test_X").what(), ASWBCB_HERE);

    TExceptFatalError const knownFatal("f", false);
    TExceptFatalError const unknownFatal("f", true);
    CheckFalse(knownFatal.IsUnknownException(), ASWBCB_HERE);
    CheckTrue(unknownFatal.IsUnknownException(), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_FNV1aHash_ReferenceValues()
{
    // The published 32-bit FNV-1a test vectors, as asw-unit-tests checks them.
    CheckEquals(0x811C9DC5u, FNV1aHash(""), ASWBCB_HERE, "empty text");
    CheckEquals(0xE40C292Cu, FNV1aHash("a"), ASWBCB_HERE, "\"a\"");
    CheckEquals(0xBF9CF968u, FNV1aHash("foobar"), ASWBCB_HERE, "\"foobar\"");
    CheckEquals(0x7A0B824Eu, FNV1aHash("\xFF"), ASWBCB_HERE, "a byte above 0x7F, taken as unsigned");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_FormatDurationMs_ThreeDecimals()
{
    CheckEquals("0.713 ms", FormatDurationMs(0.7134), ASWBCB_HERE);
    CheckEquals("0.000 ms", FormatDurationMs(0.0), ASWBCB_HERE);
    CheckEquals("1234.500 ms", FormatDurationMs(1234.5), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_FormatElapsedTime_Fields()
{
    CheckEquals("00h:00m:00s.000ms", FormatElapsedTime(0), ASWBCB_HERE);
    CheckEquals("00h:01m:02s.345ms", FormatElapsedTime(62345), ASWBCB_HERE);
    CheckEquals("26h:03m:04s.005ms", FormatElapsedTime(((26 * 60 + 3) * 60 + 4) * static_cast<__int64>(1000) + 5),
        ASWBCB_HERE, "hours past a day");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_FormatLocation_BareMethodAndLine()
{
    CheckEquals("Test_X (12)", FormatLocation("TGroup::Test_X", 12), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_FromUTF8_NonASCII()
{
    UnicodeString const text = FromUTF8("caf\xC3\xA9 \xE2\x82\xAC");

    CheckEquals(6, text.Length(), ASWBCB_HERE);
    CheckTrue(text == UnicodeString(L"caf\x00E9 \x20AC"), ASWBCB_HERE);
    CheckEquals("caf\xC3\xA9 \xE2\x82\xAC", ToUTF8(text), ASWBCB_HERE, "the round trip");
    CheckEquals(0, FromUTF8("").Length(), ASWBCB_HERE);
    CheckEquals(3, FromUTF8(std::string("a\0b", 3)).Length(), ASWBCB_HERE, "an embedded null character is kept");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_GetUTCTimeISO8601_Format()
{
    // e.g. "2026-10-11T22:45:12.224Z"
    std::string const time = GetUTCTimeISO8601();
    std::string const shape = "dddd-dd-ddTdd:dd:dd.dddZ";
    bool matches = (time.size() == shape.size());

    for (std::size_t i = 0; matches && (i < shape.size()); ++i)
        matches = (shape[i] == 'd') ? ((time[i] >= '0') && (time[i] <= '9')) : (time[i] == shape[i]);

    CheckTrue(matches, ASWBCB_HERE, time);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_MersenneTwister_IsMT19937()
{
    // The C++ standard requires std::mt19937's 10000th value from the default seed (5489) to be 4123659995.
    TMersenneTwister defaultSeed(5489u);
    unsigned int const first = defaultSeed.Next();
    unsigned int value = first;

    for (int i = 2; i <= 10000; ++i)
        value = defaultSeed.Next();

    CheckEquals(3499211612u, first, ASWBCB_HERE, "the first value from 5489");
    CheckEquals(4123659995u, value, ASWBCB_HERE, "the 10000th value from 5489");

    TMersenneTwister zeroSeed(0u);
    CheckEquals(2357136044u, zeroSeed.Next(), ASWBCB_HERE, "the first value from 0");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_NewShuffleSeed_Varies()
{
    // From the performance counter, which moves on between calls, so ten seeds in a row are never all the same.
    unsigned int const first = NewShuffleSeed();
    bool varied = false;

    for (int i = 0; (i < 10) && !varied; ++i)
        varied = (NewShuffleSeed() != first);

    CheckTrue(varied, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_NumbersToString()
{
    CheckEquals("-42", IntToString(-42), ASWBCB_HERE);
    CheckEquals("0", IntToString(0), ASWBCB_HERE);
    CheckEquals("4294967295", UIntToString(4294967295u), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_ShuffledIndices_EachIndexOnce()
{
    CheckEquals("", Joined(ShuffledIndices(0, 1u)), ASWBCB_HERE, "no indices");
    CheckEquals("0", Joined(ShuffledIndices(1, 1u)), ASWBCB_HERE, "a single index");

    std::vector<std::size_t> const shuffled = ShuffledIndices(100, 7u);
    std::vector<std::size_t> sorted = shuffled;
    std::sort(sorted.begin(), sorted.end());
    std::vector<std::size_t> identity(100);

    for (std::size_t i = 0; i < identity.size(); ++i)
        identity[i] = i;

    CheckEquals(Joined(identity), Joined(sorted), ASWBCB_HERE, "each of 0 to 99 appears exactly once");
    CheckNotEquals(Joined(identity), Joined(shuffled), ASWBCB_HERE, "and the order is actually shuffled");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_ShuffledIndices_SameAsASWUnitTests()
{
    // asw-unit-tests' Test_ShuffledIndices_GivesSameOrderEverywhere expects these orders, so a --shuffle-seed gives
    // the same order in either framework. A change here also changes the order every existing seed gives.
    CheckEquals("1,3,9,7,6,0,8,4,5,2", Joined(ShuffledIndices(10, 42u)), ASWBCB_HERE, "seed 42");
    CheckEquals("6,9,7,2,5,0,8,4,3,1", Joined(ShuffledIndices(10, 4294967295u)), ASWBCB_HERE, "the largest seed");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_TCheckMessage_AcceptsEveryStringType()
{
    CheckEquals("", TCheckMessage().Text, ASWBCB_HERE, "default");
    CheckEquals("a", TCheckMessage("a").Text, ASWBCB_HERE, "char const*");
    CheckEquals("", TCheckMessage(static_cast<char const*>(nullptr)).Text, ASWBCB_HERE, "nullptr char const*");
    CheckEquals("b", TCheckMessage(std::string("b")).Text, ASWBCB_HERE, "std::string");
    CheckEquals("caf\xC3\xA9", TCheckMessage(L"caf\x00E9").Text, ASWBCB_HERE, "wchar_t const*");
    CheckEquals("", TCheckMessage(static_cast<wchar_t const*>(nullptr)).Text, ASWBCB_HERE, "nullptr wchar_t const*");
    CheckEquals("\xE2\x82\xAC", TCheckMessage(std::wstring(L"\x20AC")).Text, ASWBCB_HERE, "std::wstring");
    CheckEquals("caf\xC3\xA9", TCheckMessage(UnicodeString(L"caf\x00E9")).Text, ASWBCB_HERE,
        "UnicodeString");
    CheckEquals("ansi", TCheckMessage(AnsiString("ansi")).Text, ASWBCB_HERE, "AnsiString");
    CheckEquals("caf\xC3\xA9", TCheckMessage(UTF8String("caf\xC3\xA9")).Text, ASWBCB_HERE, "UTF8String");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_TOptional()
{
    TOptional<unsigned int> const none;
    CheckFalse(none.HasValue(), ASWBCB_HERE, "default: no value");
    CheckEquals(0u, none.Value(), ASWBCB_HERE, "T() without a value");

    TOptional<unsigned int> zero(0u);
    CheckTrue(zero.HasValue(), ASWBCB_HERE, "zero is a value");
    CheckEquals(0u, zero.Value(), ASWBCB_HERE);

    zero = 42u;
    CheckTrue(zero.HasValue(), ASWBCB_HERE, "assigned from a T");
    CheckEquals(42u, zero.Value(), ASWBCB_HERE);

    zero = TOptional<unsigned int>();
    CheckFalse(zero.HasValue(), ASWBCB_HERE, "cleared");

    TOptional<std::string> const text("x");
    CheckEquals("x", text.Value(), ASWBCB_HERE, "a class type");
    CheckEquals("", TOptional<std::string>().Value(), ASWBCB_HERE, "a class type, T() without a value");

    TOptional<TTestOutcome> const noEnum;
    CheckEquals(0, static_cast<int>(noEnum.Value()), ASWBCB_HERE, "an enum: zero without a value");
    CheckEquals(TTestOutcome::Fail, TOptional<TTestOutcome>(TTestOutcome::Fail).Value(), ASWBCB_HERE, "an enum");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Utils::Test_ToUTF8_NonASCII()
{
    CheckEquals("caf\xC3\xA9 \xE2\x82\xAC", ToUTF8(UnicodeString(L"caf\x00E9 \x20AC")), ASWBCB_HERE);
    CheckEquals("", ToUTF8(UnicodeString()), ASWBCB_HERE);
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_Utils, "SelfTests")
