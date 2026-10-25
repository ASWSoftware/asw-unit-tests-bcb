/* **************************************************************************
Test_ASWUnitTestsBCB_Values.cpp
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_Values.h: how assertions compare and describe values.

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
#include "Test_ASWUnitTestsBCB_Values.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <cstring>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Values.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Classes;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace TestShapes
{

struct TShape
{
    virtual ~TShape()
    {
    }
};

struct TCircle : public TShape
{
};

struct TSquare : public TShape
{
};

} // namespace TestShapes

//---------------------------------------------------------------------------

namespace
{

enum EColor
{
    Red,
    Green,
    Blue
};

enum ESize
{
    Small = 1,
    Large = 2
};

enum ESigned
{
    MinusFive = -5,
    PlusFive = 5
};

enum class EScopedColor
{
    Red,
    Green,
    Blue,
};

enum class EScopedSigned : short
{
    MinusThree = -3,
    PlusThree = 3,
};

struct TOuter
{
    enum EInner
    {
        InnerTwo = 2
    };

    enum class EScopedInner
    {
        Two = 2,
    };
};

struct TBase
{
    int BaseValue;
};

struct TDerived : public TBase
{
    int DerivedValue;
};

//---------------------------------------------------------------------------
/*
    Compare

    Returns "equal" when ValuesEqual() finds 'expected' and 'actual' equal, otherwise how DescribeValue() describes
    them: "\"expected\" vs \"actual\"". The calls are qualified, since bcc32 doesn't find a function of an enclosing
    namespace from an anonymous one when an argument has a class type.
*/
template <typename TExpected, typename TActual>
std::string Compare(TExpected const& expected, TActual const& actual)
{
    if (ASWUnitTestsBCB::ValuesEqual(expected, actual))
        return "equal";

    return "\"" + ASWUnitTestsBCB::DescribeValue(expected) + "\" vs \"" + ASWUnitTestsBCB::DescribeValue(actual) +
        "\"";
}
//---------------------------------------------------------------------------
/*
    Compare

    "equal" when TextsEqualIgnoringASCIICase() finds 'expected' and 'actual' equal, otherwise "different".
*/
template <typename TExpected, typename TActual>
std::string CompareIC(TExpected const& expected, TActual const& actual)
{
    return ASWUnitTestsBCB::TextsEqualIgnoringASCIICase(expected, actual) ? "equal" : "different";
}
//---------------------------------------------------------------------------
/*
    EmptinessOf

    "empty" or "null" when ValueEmptiness() finds 'value' empty or a null C string, otherwise how DescribeContents()
    describes it.
*/
template <typename T>
std::string EmptinessOf(T const& value)
{
    switch (ASWUnitTestsBCB::ValueEmptiness(value))
    {
        case ASWUnitTestsBCB::TEmptiness::Empty:
            return "empty";

        case ASWUnitTestsBCB::TEmptiness::NullString:
            return "null";

        case ASWUnitTestsBCB::TEmptiness::NotEmpty:
            break;
    }

    return ASWUnitTestsBCB::DescribeContents(value);
}
//---------------------------------------------------------------------------
/*
    MatchOf

    "match" when TextMatchFailure() finds that 'text' matches 'pattern', otherwise why not.
*/
template <typename TText, typename TPattern>
std::string MatchOf(TText const& text, TPattern const& pattern)
{
    std::string const failure = ASWUnitTestsBCB::TextMatchFailure(text, pattern, true);
    return failure.empty() ? std::string("match") : failure;
}
//---------------------------------------------------------------------------
bool StartsWithText(std::string const& text, std::string const& prefix)
{
    return text.compare(0, prefix.size(), prefix) == 0;
}
//---------------------------------------------------------------------------
/*
    OrderOf

    How CompareOrdered() finds 'value' compares with 'bound': "<", "=", ">" or "unordered".
*/
template <typename TValue, typename TBound>
std::string OrderOf(TValue const& value, TBound const& bound)
{
    switch (ASWUnitTestsBCB::CompareOrdered(value, bound))
    {
        case ASWUnitTestsBCB::TOrder::Less:
            return "<";

        case ASWUnitTestsBCB::TOrder::Equal:
            return "=";

        case ASWUnitTestsBCB::TOrder::Greater:
            return ">";

        case ASWUnitTestsBCB::TOrder::Unordered:
            return "unordered";
    }

    return "?";
}
//---------------------------------------------------------------------------
/*
    OrderedTexts

    How DescribeOrderedValues() describes 'value' and 'bound': "value, bound".
*/
template <typename TValue, typename TBound>
std::string OrderedTexts(TValue const& value, TBound const& bound)
{
    std::string valueText;
    std::string boundText;

    ASWUnitTestsBCB::DescribeOrderedValues(value, bound, valueText, boundText);
    return valueText + ", " + boundText;
}
//---------------------------------------------------------------------------
/*
    PartsOf

    Where TextHasPart() finds 'part' in 'text': "contains", "starts" and "ends", comma-separated, or "" for none.
*/
template <typename TText, typename TPart>
std::string PartsOf(TText const& text, TPart const& part, bool ignoreCase)
{
    std::string parts;

    if (ASWUnitTestsBCB::TextHasPart(text, part, ASWUnitTestsBCB::TTextPart::Contains, ignoreCase))
        parts += "contains,";

    if (ASWUnitTestsBCB::TextHasPart(text, part, ASWUnitTestsBCB::TTextPart::StartsWith, ignoreCase))
        parts += "starts,";

    if (ASWUnitTestsBCB::TextHasPart(text, part, ASWUnitTestsBCB::TTextPart::EndsWith, ignoreCase))
        parts += "ends,";

    return parts.empty() ? parts : parts.substr(0, parts.size() - 1);
}
//---------------------------------------------------------------------------
/*
    SameAddressOf

    The address the Same assertions compare for 'value', described.
*/
template <typename T>
std::string SameAddressOf(T const& value)
{
    return ASWUnitTestsBCB::DescribePointer(ASWUnitTestsBCB::TSameAddress<T>::Get(value));
}
//---------------------------------------------------------------------------
/*
    IsNullValue

    Whether the Null assertions find 'value' null.
*/
template <typename T>
bool IsNullValue(T const& value)
{
    return ASWUnitTestsBCB::TNullCheck<T>::IsNull(value);
}
//---------------------------------------------------------------------------
/*
    Address

    'pointer''s address as the framework should show it, e.g. "0x12ff40".
*/
std::string Address(void const* pointer)
{
    std::ostringstream stream;
    stream << "0x" << std::hex << reinterpret_cast<std::size_t>(pointer);
    return stream.str();
}
//---------------------------------------------------------------------------
void FunctionA()
{
}
//---------------------------------------------------------------------------
void FunctionB()
{
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_Values
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_Values::TTest_ASWUnitTestsBCB_Values()
    : inherited("ASWUnitTestsBCB_Values_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Bool()
{
    int const two = 2;

    CheckEquals("equal", Compare(true, true), ASWBCB_HERE);
    CheckEquals("\"true\" vs \"false\"", Compare(true, false), ASWBCB_HERE);
    CheckEquals("\"false\" vs \"true\"", Compare(false, two == 2), ASWBCB_HERE, "an expression");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_ClassNames()
{
    std::string const circle = DescribeTypeInfo(typeid(TestShapes::TCircle));

#if defined(__clang__) && !defined(__MINGW32__) // bcc32c: the compiler's mangled name, e.g. "$TestShapes@TCircle"
    CheckTrue(circle.find("TCircle") != std::string::npos, ASWBCB_HERE, circle);
#else
    CheckEquals("TestShapes::TCircle", circle, ASWBCB_HERE, "readable with bcc32, demangled with bcc64x");
#endif
    CheckEquals(circle, DescribeClass<TestShapes::TCircle>(static_cast<TestShapes::TCircle*>(nullptr)), ASWBCB_HERE,
        "a C++ class");
    CheckEquals("TStrings", DescribeClass<TStrings>(static_cast<TStrings*>(nullptr)),
        ASWBCB_HERE, "a TObject descendant, by its ClassName() with every compiler");

    std::unique_ptr<TObject> list(new TStringList());
    TestShapes::TCircle circleObject;
    TestShapes::TShape const* const shape = &circleObject;

    CheckEquals("TStringList", DescribeObjectClass<TObject>(list.get(), list.get()), ASWBCB_HERE,
        "the object's own class");
    CheckEquals(circle, DescribeObjectClass<TestShapes::TShape>(shape, shape), ASWBCB_HERE, "the object's own class");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Emptiness()
{
    char const* const nullText = nullptr;
    wchar_t const* const nullWide = nullptr;
    char buffer[8] = "";
    std::vector<int> vector;
    std::list<std::string> list(1, "a");
    std::map<int, int> map;
    map[1] = 1;
    map[2] = 2;
    TBytes bytes;

    CheckEquals("empty", EmptinessOf(std::string()), ASWBCB_HERE);
    CheckEquals("was \"abc\"", EmptinessOf(std::string("abc")), ASWBCB_HERE);
    CheckEquals("empty", EmptinessOf(""), ASWBCB_HERE, "a string literal");
    CheckEquals("was \"ab\"", EmptinessOf("ab"), ASWBCB_HERE, "a string literal");
    CheckEquals("empty", EmptinessOf(buffer), ASWBCB_HERE, "a char array");
    CheckEquals("null", EmptinessOf(nullText), ASWBCB_HERE, "a null C string is neither empty nor not empty");
    CheckEquals("empty", EmptinessOf(L""), ASWBCB_HERE);
    CheckEquals("was \"x\xC3\xA9\"", EmptinessOf(L"x\x00E9"), ASWBCB_HERE, "wide text as UTF-8");
    CheckEquals("null", EmptinessOf(nullWide), ASWBCB_HERE, "a null wide C string");
    CheckEquals("empty", EmptinessOf(std::wstring()), ASWBCB_HERE);
    CheckEquals("was \" \"", EmptinessOf(std::wstring(L" ")), ASWBCB_HERE, "a space isn't empty");
    CheckEquals("empty", EmptinessOf(UnicodeString()), ASWBCB_HERE);
    CheckEquals("was \"abc\"", EmptinessOf(UnicodeString(L"abc")), ASWBCB_HERE);
    CheckEquals("empty", EmptinessOf(AnsiString()), ASWBCB_HERE);
    CheckEquals("was \"a\"", EmptinessOf(AnsiString("a")), ASWBCB_HERE);
    CheckEquals("empty", EmptinessOf(UTF8String()), ASWBCB_HERE);
    CheckEquals("was \"b\"", EmptinessOf(UTF8String("b")), ASWBCB_HERE);

    CheckEquals("empty", EmptinessOf(vector), ASWBCB_HERE);
    vector.push_back(7);
    CheckEquals("had 1 element", EmptinessOf(vector), ASWBCB_HERE);
    vector.push_back(8);
    vector.push_back(9);
    CheckEquals("had 3 elements", EmptinessOf(vector), ASWBCB_HERE);
    CheckEquals("had 1 element", EmptinessOf(list), ASWBCB_HERE);
    CheckEquals("had 2 elements", EmptinessOf(map), ASWBCB_HERE);
    CheckEquals("empty", EmptinessOf(std::map<int, int>()), ASWBCB_HERE);
    CheckEquals("empty", EmptinessOf(bytes), ASWBCB_HERE, "a System::DynamicArray");
    bytes.Length = 4;
    CheckEquals("had 4 elements", EmptinessOf(bytes), ASWBCB_HERE, "a System::DynamicArray");

    // Failure texts
    CheckEquals("", EmptyFailure(""), ASWBCB_HERE);
    CheckEquals("Expected empty but was \"ab\"", EmptyFailure("ab"), ASWBCB_HERE);
    CheckEquals("Expected empty but had 4 elements", EmptyFailure(bytes), ASWBCB_HERE);
    CheckEquals("Expected empty but was \"(null)\"", EmptyFailure(nullText), ASWBCB_HERE, "nullptr fails Empty");
    CheckEquals("", NotEmptyFailure("ab"), ASWBCB_HERE);
    CheckEquals("Expected not empty but was empty", NotEmptyFailure(std::wstring()), ASWBCB_HERE);
    CheckEquals("Expected not empty but was \"(null)\"", NotEmptyFailure(nullWide), ASWBCB_HERE,
        "nullptr fails NotEmpty too");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Enums()
{
    EColor const color = Blue;

    CheckEquals("equal", Compare(Blue, color), ASWBCB_HERE);
    CheckEquals("\"0\" vs \"2\"", Compare(Red, color), ASWBCB_HERE, "described as numbers");
    CheckEquals("equal", Compare(Green, 1), ASWBCB_HERE, "an enum and an int");
    CheckEquals("equal", Compare(2u, Blue), ASWBCB_HERE, "an unsigned int and an enum");
    CheckEquals("equal", Compare(Large, Blue), ASWBCB_HERE, "two enum types with the same value");
    CheckEquals("equal", Compare(TOuter::InnerTwo, Large), ASWBCB_HERE, "an enum declared in a class");
    CheckEquals("\"-5\" vs \"5\"", Compare(MinusFive, PlusFive), ASWBCB_HERE, "a negative value");
    CheckEquals("\"-5\" vs \"4294967291\"", Compare(MinusFive, 0xFFFFFFFBu), ASWBCB_HERE, "compared by value");
    CheckEquals("\"1\" vs \"0\"", Compare(Small, static_cast<__int64>(0)), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Enums_Scoped()
{
    EScopedColor const color = EScopedColor::Blue;

    CheckEquals("equal", Compare(EScopedColor::Blue, color), ASWBCB_HERE);
    CheckEquals("\"0\" vs \"2\"", Compare(EScopedColor::Red, color), ASWBCB_HERE, "described as numbers");
    CheckEquals("equal", Compare(TOuter::EScopedInner::Two, EScopedColor::Blue), ASWBCB_HERE,
        "an enum class declared in a class, with another enum type with the same value");
    CheckEquals("equal", Compare(EScopedColor::Green, 1), ASWBCB_HERE, "an enum class and an int, as any enum");
    CheckEquals("\"-3\" vs \"3\"", Compare(EScopedSigned::MinusThree, EScopedSigned::PlusThree), ASWBCB_HERE,
        "a negative value of a declared underlying type");
    CheckEquals(EScopedColor::Blue, color, ASWBCB_HERE, "CheckEquals() takes an enum class");
    CheckNotEquals(EScopedColor::Red, color, ASWBCB_HERE, "CheckNotEquals() takes an enum class");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_FloatingPoint_Fixed()
{
    CheckEquals("0.100000", DescribeFixed(0.1), ASWBCB_HERE);
    CheckEquals("-2.500000", DescribeFixed(-2.5), ASWBCB_HERE);
    CheckEquals("123456789.000000", DescribeFixed(123456789.0), ASWBCB_HERE, "no exponent");
    CheckEquals("0.000000", DescribeFixed(1e-9), ASWBCB_HERE, "six decimals, as std::to_string()");
    CheckEquals("0.333333", DescribeFixed(1.0f / 3.0f), ASWBCB_HERE, "a float");
    CheckEquals("nan", DescribeFixed(std::numeric_limits<double>::quiet_NaN()), ASWBCB_HERE);
    CheckEquals("inf", DescribeFixed(std::numeric_limits<double>::infinity()), ASWBCB_HERE);
    CheckEquals("-inf", DescribeFixed(-std::numeric_limits<double>::infinity()), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Integers_MixedTypes()
{
    int const i = 2;
    unsigned int const u = 2;
    __int64 const big = 2;
    long long const longLong = 2; // The same type as __int64 with every supported compiler
    char const c = 'a';

    CheckEquals("equal", Compare(2, u), ASWBCB_HERE, "int and unsigned int");
    CheckEquals("equal", Compare(i, big), ASWBCB_HERE, "int and __int64");
    CheckEquals("equal", Compare(longLong, big), ASWBCB_HERE, "long long and __int64");
    CheckEquals("equal", Compare(97, c), ASWBCB_HERE, "int and char");
    CheckEquals("equal", Compare(static_cast<unsigned char>(200), 200), ASWBCB_HERE, "unsigned char and int");
    CheckEquals("equal", Compare(L'a', 97), ASWBCB_HERE, "wchar_t and int");
    CheckEquals("equal", Compare(static_cast<short>(-5), -5L), ASWBCB_HERE, "short and long");
    CheckEquals("\"2\" vs \"3\"", Compare(big, 3u), ASWBCB_HERE, "described as numbers");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Integers_SameTypePairs()
{
    // bcc32 compared these as bool (so they passed) when SFINAE templates competed with a (bool, bool) overload.
    CheckEquals("\"1\" vs \"2\"", Compare(1, 2), ASWBCB_HERE);
    CheckEquals("\"97\" vs \"98\"", Compare('a', 'b'), ASWBCB_HERE, "chars are numbers");
    CheckEquals("\"1\" vs \"2\"", Compare(1u, 2u), ASWBCB_HERE);
    CheckEquals("\"1\" vs \"2\"", Compare(1L, 2L), ASWBCB_HERE);
    CheckEquals("equal", Compare(7, 7), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Integers_Signedness()
{
    CheckEquals("\"-1\" vs \"4294967295\"", Compare(-1, 0xFFFFFFFFu), ASWBCB_HERE, "-1 isn't 0xFFFFFFFF");
    CheckEquals("\"18446744073709551615\" vs \"-1\"", Compare(static_cast<unsigned __int64>(-1), -1), ASWBCB_HERE);
    CheckEquals("\"-9223372036854775808\" vs \"0\"", Compare(static_cast<__int64>(0x8000000000000000ULL), 0),
        ASWBCB_HERE, "the most negative __int64");
    CheckEquals("equal", Compare(-3, static_cast<__int64>(-3)), ASWBCB_HERE);
    CheckEquals("equal", Compare(0xFFFFFFFFu, static_cast<__int64>(0xFFFFFFFFLL)), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_MemoryFailure()
{
    unsigned char const bytes[] = { 0x01, 0x02, 0x03, 0x04 };
    unsigned char const sameBytes[] = { 0x01, 0x02, 0x03, 0x04 };
    unsigned char const thirdDiffers[] = { 0x01, 0x02, 0xFF, 0x04 };
    unsigned char const deadBeef[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    unsigned char sequence[24];
    unsigned char laterDiffers[24];
    unsigned char zeros[20];
    unsigned char ones[20];
    unsigned char fifthDiffers[20];

    for (unsigned char i = 0; i < 24; ++i)
    {
        sequence[i] = i;
        laterDiffers[i] = i;
    }

    laterDiffers[20] = 0xAA;
    std::memset(zeros, 0x00, sizeof(zeros));
    std::memset(ones, 0x11, sizeof(ones));
    std::memset(fifthDiffers, 0x00, sizeof(fifthDiffers));
    fifthDiffers[4] = 0x11;

    // EqualsMem
    CheckEquals("", MemoryFailure(bytes, sameBytes, 4, true), ASWBCB_HERE, "same bytes, different buffers");
    CheckEquals("", MemoryFailure(bytes, bytes, 4, true), ASWBCB_HERE, "the same buffer");
    CheckEquals("", MemoryFailure(nullptr, nullptr, 4, true), ASWBCB_HERE, "the same pointer, so never read");
    CheckEquals("", MemoryFailure(bytes, thirdDiffers, 0, true), ASWBCB_HERE, "zero bytes are always equal");
    CheckEquals("", MemoryFailure(bytes, nullptr, 0, true), ASWBCB_HERE, "zero bytes, even with one nullptr");
    CheckEquals("Bytes differ at offset 2 of 4: expected \"03 04\" but was \"FF 04\"",
        MemoryFailure(bytes, thirdDiffers, 4, true), ASWBCB_HERE);
    CheckEquals("Bytes differ at offset 20 of 24: expected \"14 15 16 17\" but was \"AA 15 16 17\"",
        MemoryFailure(sequence, laterDiffers, sizeof(sequence), true), ASWBCB_HERE);
    CheckEquals("Bytes differ at offset 0 of 20: expected \"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ...\" "
        "but was \"11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 ...\"", MemoryFailure(zeros, ones, 20, true),
        ASWBCB_HERE, "16 bytes, then ...");
    CheckEquals("Bytes differ at offset 4 of 20: expected \"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00\" "
        "but was \"11 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00\"", MemoryFailure(zeros, fifthDiffers, 20, true),
        ASWBCB_HERE, "exactly 16 bytes from the offset: no ...");
    CheckEquals("Cannot compare 4 bytes: actual is null", MemoryFailure(bytes, nullptr, 4, true), ASWBCB_HERE);
    CheckEquals("Cannot compare 4 bytes: expected is null", MemoryFailure(nullptr, bytes, 4, true), ASWBCB_HERE);

    // NotEqualsMem
    CheckEquals("", MemoryFailure(bytes, thirdDiffers, 4, false), ASWBCB_HERE);
    CheckEquals("", MemoryFailure(nullptr, bytes, 4, false), ASWBCB_HERE, "nullptr isn't equal to any memory");
    CheckEquals("", MemoryFailure(bytes, nullptr, 4, false), ASWBCB_HERE, "nullptr isn't equal to any memory");
    CheckEquals("Expected the 4 bytes to differ, but both are \"DE AD BE EF\"",
        MemoryFailure(deadBeef, deadBeef, 4, false), ASWBCB_HERE, "the same buffer");
    CheckEquals("Expected the 4 bytes to differ, but both are \"01 02 03 04\"",
        MemoryFailure(bytes, sameBytes, 4, false), ASWBCB_HERE);
    CheckEquals("Expected the 4 bytes to differ, but both are \"\"", MemoryFailure(nullptr, nullptr, 4, false), ASWBCB_HERE,
        "both nullptr");
    CheckEquals("Expected the 0 bytes to differ, but both are \"\"", MemoryFailure(bytes, deadBeef, 0, false),
        ASWBCB_HERE, "zero bytes are always equal");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_NarrowStrings()
{
    std::string const s = "abc";
    char buffer[4] = { 'a', 'b', 'c', 0 };
    char const* pointer = "abc";

    CheckEquals("equal", Compare("abc", "abc"), ASWBCB_HERE, "literals");
    CheckEquals("\"abc\" vs \"abd\"", Compare("abc", "abd"), ASWBCB_HERE);
    CheckEquals("\"a\" vs \"ab\"", Compare("a", "ab"), ASWBCB_HERE, "different lengths");
    CheckEquals("equal", Compare(s, "abc"), ASWBCB_HERE, "std::string and a literal");
    CheckEquals("equal", Compare("abc", s), ASWBCB_HERE, "a literal and std::string");
    CheckEquals("equal", Compare(buffer, pointer), ASWBCB_HERE, "an array and a pointer");
    CheckEquals("equal", Compare(pointer, s), ASWBCB_HERE, "a pointer and std::string");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_NarrowStrings_Null()
{
    char const* nullPointer = nullptr;
    char const* empty = "";

    CheckEquals("equal", Compare(nullPointer, nullPointer), ASWBCB_HERE);
    CheckEquals("\"(null)\" vs \"\"", Compare(nullPointer, empty), ASWBCB_HERE, "nullptr isn't empty");
    CheckEquals("\"\" vs \"(null)\"", Compare(std::string(), nullPointer), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Null()
{
    int value = 1;
    int* const nullPointer = nullptr;
    void (*nullFunction)() = nullptr;
    std::unique_ptr<int> const owned(new int(1));
    std::unique_ptr<int> const empty;
    std::unique_ptr<int[]> const ownedArray(new int[2]);
    DelphiInterface<IInterface> emptyInterface;
    DelphiInterface<IInterface> someInterface;
    TInterfacedObject* const interfaced = new TInterfacedObject(); // Freed by someInterface

    AssertTrue(interfaced->GetInterface(__uuidof(IInterface), &someInterface), ASWBCB_HERE);

    CheckFalse(IsNullValue(&value), ASWBCB_HERE);
    CheckTrue(IsNullValue(nullPointer), ASWBCB_HERE);
    CheckFalse(IsNullValue(&FunctionA), ASWBCB_HERE, "a function pointer");
    CheckTrue(IsNullValue(nullFunction), ASWBCB_HERE, "a function pointer");
    CheckFalse(IsNullValue(owned), ASWBCB_HERE, "std::unique_ptr");
    CheckTrue(IsNullValue(empty), ASWBCB_HERE, "std::unique_ptr");
    CheckFalse(IsNullValue(ownedArray), ASWBCB_HERE, "std::unique_ptr of an array");
    CheckFalse(IsNullValue(someInterface), ASWBCB_HERE, "System::DelphiInterface");
    CheckTrue(IsNullValue(emptyInterface), ASWBCB_HERE, "System::DelphiInterface");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Ordering_Compare()
{
    double const nan = std::numeric_limits<double>::quiet_NaN();
    double const infinity = std::numeric_limits<double>::infinity();

    CheckEquals("<", OrderOf(1, 2), ASWBCB_HERE);
    CheckEquals(">", OrderOf(2u, 1), ASWBCB_HERE);
    CheckEquals("=", OrderOf(7, 7L), ASWBCB_HERE);
    CheckEquals("<", OrderOf(-1, 0xFFFFFFFFu), ASWBCB_HERE, "by value, without wrapping");
    CheckEquals(">", OrderOf(0xFFFFFFFFFFFFFFFFULL, -1), ASWBCB_HERE, "by value, without wrapping");
    CheckEquals("<", OrderOf(static_cast<__int64>(-5), -3), ASWBCB_HERE, "two negative values");
    CheckEquals(">", OrderOf(static_cast<unsigned char>(200), static_cast<signed char>(-1)), ASWBCB_HERE);
    CheckEquals("=", OrderOf('a', 97), ASWBCB_HERE, "a char is a number");
    CheckEquals("<", OrderOf(1.5, 2), ASWBCB_HERE);
    CheckEquals(">", OrderOf(2, 1.5f), ASWBCB_HERE);
    CheckEquals("=", OrderOf(0.5f, 0.5), ASWBCB_HERE);
    CheckEquals(">", OrderOf(0.1f, 0.1), ASWBCB_HERE, "a float and a double compare as double");
    CheckEquals("=", OrderOf(16777217, 16777216.0f), ASWBCB_HERE, "an int and a float compare as float, as in C++");
    CheckEquals("<", OrderOf(1.5L, 2), ASWBCB_HERE, "long double");
    CheckEquals(">", OrderOf(infinity, std::numeric_limits<double>::max()), ASWBCB_HERE);
    CheckEquals("<", OrderOf(-infinity, -1), ASWBCB_HERE);
    CheckEquals("unordered", OrderOf(nan, 1.0), ASWBCB_HERE, "a NaN is in no order");
    CheckEquals("unordered", OrderOf(1, nan), ASWBCB_HERE, "a NaN is in no order");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Ordering_Describe()
{
    CheckEquals("1, 2", OrderedTexts(1, 2), ASWBCB_HERE);
    CheckEquals("-1, 4294967295", OrderedTexts(-1, 0xFFFFFFFFu), ASWBCB_HERE);
    CheckEquals("98, 97", OrderedTexts('b', 'a'), ASWBCB_HERE, "chars as numbers");
    CheckEquals("1.5, 2", OrderedTexts(1.5, 2), ASWBCB_HERE);
    CheckEquals("0.25, 3", OrderedTexts(0.25f, 3), ASWBCB_HERE, "a float");
    CheckEquals("1.5, 2", OrderedTexts(1.5L, 2), ASWBCB_HERE, "a long double");
    CheckEquals("0.100000001490116, 0.1", OrderedTexts(0.1f, 0.1), ASWBCB_HERE, "in the common type, 15 digits");
    CheckEquals("1, 1.0000000000000011", OrderedTexts(1.0, 1.0 + 1e-15), ASWBCB_HERE,
        "all 17 digits when 15 would make them look the same");
    CheckEquals("nan, 1", OrderedTexts(std::numeric_limits<double>::quiet_NaN(), 1.0), ASWBCB_HERE);
    CheckEquals("inf, -inf", OrderedTexts(std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Ordering_Holds()
{
    TOrder const orders[] = { TOrder::Less, TOrder::Equal, TOrder::Greater, TOrder::Unordered };
    TOrdering const orderings[] = {
        TOrdering::GreaterThan, TOrdering::GreaterThanOrEqual, TOrdering::LessThan, TOrdering::LessThanOrEqual
    };
    // For each ordering, whether each order (less, equal, greater, unordered) satisfies it.
    char const* const expected[] = { "0010", "0110", "1000", "1100" };

    for (std::size_t i = 0; i < sizeof(orderings) / sizeof(orderings[0]); ++i)
    {
        std::string holds;

        for (std::size_t j = 0; j < sizeof(orders) / sizeof(orders[0]); ++j)
            holds += OrderingHolds(orders[j], orderings[i]) ? "1" : "0";

        CheckEquals(expected[i], holds, ASWBCB_HERE, OrderingSymbol(orderings[i]));
    }

    CheckEquals(">", OrderingSymbol(TOrdering::GreaterThan), ASWBCB_HERE);
    CheckEquals(">=", OrderingSymbol(TOrdering::GreaterThanOrEqual), ASWBCB_HERE);
    CheckEquals("<", OrderingSymbol(TOrdering::LessThan), ASWBCB_HERE);
    CheckEquals("<=", OrderingSymbol(TOrdering::LessThanOrEqual), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Pointers()
{
    int first = 1;
    int second = 1;
    int* const firstPointer = &first;
    int const* const firstConstPointer = &first;
    void* const firstVoidPointer = &first;
    int* const nullPointer = nullptr;
    TDerived derived;
    TBase* const basePointer = &derived;
    unsigned char bytes[2] = { 'a', 0 };

    CheckEquals("equal", Compare(firstPointer, &first), ASWBCB_HERE);
    CheckEquals("\"" + Address(&first) + "\" vs \"" + Address(&second) + "\"", Compare(&first, &second),
        ASWBCB_HERE, "compared and described by address, not by what they point to");
    CheckEquals("equal", Compare(firstConstPointer, firstPointer), ASWBCB_HERE, "int const* and int*");
    CheckEquals("equal", Compare(firstVoidPointer, firstPointer), ASWBCB_HERE, "void* and int*");
    CheckEquals("equal", Compare(basePointer, &derived), ASWBCB_HERE, "a base class pointer and a derived one");
    CheckEquals("equal", Compare(nullPointer, static_cast<int*>(nullptr)), ASWBCB_HERE);
    CheckEquals("\"(null)\" vs \"" + Address(&first) + "\"", Compare(nullPointer, firstPointer), ASWBCB_HERE);
    CheckEquals("\"" + Address(&bytes[0]) + "\" vs \"" + Address(&bytes[1]) + "\"", Compare(&bytes[0], &bytes[1]),
        ASWBCB_HERE, "unsigned char* isn't a string");

    std::string const functions = Compare(&FunctionA, &FunctionB);
    CheckEquals("equal", Compare(&FunctionA, &FunctionA), ASWBCB_HERE, "function pointers");
    CheckEquals("\"0x", functions.substr(0, 3), ASWBCB_HERE, functions);
    CheckTrue(functions.find("\" vs \"0x") != std::string::npos, ASWBCB_HERE, functions);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_RegularExpressions()
{
    char const* const nullText = nullptr;
    std::string const date = "\\d{4}-\\d{2}-\\d{2}";
    std::string const invalid = "Invalid regular expression \"";

    CheckEquals("match", MatchOf("2026-10-07", date), ASWBCB_HERE);
    CheckEquals("Expected \"2026-1-7\" to match \"\\d{4}-\\d{2}-\\d{2}\"", MatchOf("2026-1-7", date), ASWBCB_HERE);
    CheckEquals("Expected \"abc123x\" to match \"[a-z]+\\d+\"", MatchOf("abc123x", "[a-z]+\\d+"), ASWBCB_HERE,
        "the whole text must match");
    CheckEquals("Expected \"xabc\" to match \"abc\"", MatchOf("xabc", "abc"), ASWBCB_HERE, "the whole text");
    CheckEquals("match", MatchOf("xabcx", ".*abc.*"), ASWBCB_HERE, "part of the text, with .*");
    CheckEquals("match", MatchOf("ab", "a|ab"), ASWBCB_HERE, "an alternation inside the anchors");
    CheckEquals("Expected \"abc\n\" to match \"abc\"", MatchOf("abc\n", "abc"), ASWBCB_HERE, "not before a newline");
    CheckEquals("match", MatchOf("ABC", "(?i)abc"), ASWBCB_HERE, "an inline option");
    CheckEquals("match", MatchOf("", ""), ASWBCB_HERE, "an empty pattern matches empty text");
    CheckEquals("Expected \"a\" to match \"\"", MatchOf("a", ""), ASWBCB_HERE, "an empty pattern");
    CheckEquals("match", MatchOf("a.b", "\\Qa.b"), ASWBCB_HERE, "a \\Q quote left open");
    CheckEquals("Expected \"axb\" to match \"\\Qa.b\"", MatchOf("axb", "\\Qa.b"), ASWBCB_HERE, "quoted");

    // Text types and Unicode
    CheckEquals("match", MatchOf(L"caf\x00E9", L"caf."), ASWBCB_HERE, "wide");
    CheckEquals("match", MatchOf(std::string("caf\xC3\xA9"), L"caf\x00E9"), ASWBCB_HERE, "UTF-8 with UTF-16");
    CheckEquals("match", MatchOf(std::string("caf\xC3\xA9"), "caf."), ASWBCB_HERE, "UTF-8: . is one character");
    CheckEquals("match", MatchOf(UnicodeString(L"x1"), "x\\d"), ASWBCB_HERE);
    CheckEquals("match", MatchOf(AnsiString("x1"), std::wstring(L"x\\d")), ASWBCB_HERE);
    CheckEquals("match", MatchOf(L"\xD83D\xDE00", L"."), ASWBCB_HERE, ". is a whole surrogate pair");
    CheckEquals("match", MatchOf("", "(?!.)"), ASWBCB_HERE, "empty text, nothing after it");
    CheckEquals("Expected \"\" to match \"a?b\"", MatchOf(L"", L"a?b"), ASWBCB_HERE, "empty text");

    // null C strings never match.
    CheckEquals("Expected \"(null)\" to match \".*\"", MatchOf(nullText, ".*"), ASWBCB_HERE);
    CheckEquals("Expected \"abc\" to match \"(null)\"", MatchOf("abc", nullText), ASWBCB_HERE);
    CheckEquals("", TextMatchFailure(nullText, ".*", false), ASWBCB_HERE, "NotMatches passes");

    // NotMatches
    CheckEquals("", TextMatchFailure("abc", "a.", false), ASWBCB_HERE);
    CheckEquals("Expected \"abc\" not to match \"a.c\"", TextMatchFailure("abc", "a.c", false), ASWBCB_HERE);

    // Invalid patterns fail both, saying why.
    std::string const unbalanced = MatchOf("abc", "a(");
    CheckTrue(StartsWithText(unbalanced, invalid + "a(\": ") && (unbalanced.size() > invalid.size() + 5),
        ASWBCB_HERE, unbalanced);
    std::string const hidden = MatchOf("ab", "a)(b");
    CheckTrue(StartsWithText(hidden, invalid + "a)(b\": "), ASWBCB_HERE, "not valid inside the anchors: " + hidden);
    CheckTrue(StartsWithText(TextMatchFailure("abc", "[a-", false), invalid + "[a-\": "), ASWBCB_HERE,
        "NotMatches fails too");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_RTLStrings_AndMixes()
{
    std::string const s = "abc";
    std::wstring const w = L"abc";
    UnicodeString const unicode = L"abc";
    AnsiString const ansi = "abc";
    UTF8String const utf8 = "abc";
    wchar_t const* wideNull = nullptr;

    CheckEquals("equal", Compare(unicode, L"abc"), ASWBCB_HERE, "UnicodeString and a wide literal");
    CheckEquals("equal", Compare(unicode, "abc"), ASWBCB_HERE, "UnicodeString and a narrow literal");
    CheckEquals("equal", Compare(s, unicode), ASWBCB_HERE, "std::string and UnicodeString");
    CheckEquals("equal", Compare(ansi, unicode), ASWBCB_HERE, "AnsiString and UnicodeString");
    CheckEquals("equal", Compare(utf8, w), ASWBCB_HERE, "UTF8String and std::wstring");
    CheckEquals("equal", Compare(s, w), ASWBCB_HERE, "std::string and std::wstring");
    CheckEquals("\"abc\" vs \"abcx\"", Compare(unicode, unicode + L"x"), ASWBCB_HERE);
    CheckEquals("\"(null)\" vs \"\"", Compare(wideNull, UnicodeString()), ASWBCB_HERE, "nullptr isn't empty");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Same()
{
    int values[3] = { 1, 2, 3 };
    char const* const text = "abc";
    int* const nullPointer = nullptr;
    std::unique_ptr<int> const owned(new int(1));
    DelphiInterface<IInterface> someInterface;
    TInterfacedObject* const interfaced = new TInterfacedObject(); // Freed by someInterface

    AssertTrue(interfaced->GetInterface(__uuidof(IInterface), &someInterface), ASWBCB_HERE);
    IInterface* const rawInterface = someInterface;

    CheckEquals(Address(&values[1]), SameAddressOf(&values[1]), ASWBCB_HERE, "a raw pointer as it is");
    CheckEquals(Address(&values[0]), SameAddressOf(values), ASWBCB_HERE, "an array's first element");
    CheckEquals(Address(text), SameAddressOf(text), ASWBCB_HERE, "a C string by address");
    CheckEquals(Address(owned.get()), SameAddressOf(owned), ASWBCB_HERE, "std::unique_ptr's get()");
    CheckEquals(Address(rawInterface), SameAddressOf(someInterface), ASWBCB_HERE, "the interface pointer");
    CheckEquals("(null)", SameAddressOf(nullPointer), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Texts_HasPart()
{
    char const* const nullString = nullptr;

    CheckEquals("contains", PartsOf("abcdef", "cd", false), ASWBCB_HERE);
    CheckEquals("contains,starts", PartsOf("abcdef", "ab", false), ASWBCB_HERE);
    CheckEquals("contains,ends", PartsOf("abcdef", "ef", false), ASWBCB_HERE);
    CheckEquals("contains,starts,ends", PartsOf("abc", "abc", false), ASWBCB_HERE, "the whole text");
    CheckEquals("contains,ends", PartsOf("aab", "ab", false), ASWBCB_HERE, "after a partial match");
    CheckEquals("", PartsOf("ab", "abc", false), ASWBCB_HERE, "a part longer than the text");
    CheckEquals("", PartsOf("abcdef", "x", false), ASWBCB_HERE);
    CheckEquals("contains,starts,ends", PartsOf("abc", "", false), ASWBCB_HERE, "every text has an empty part");
    CheckEquals("contains,starts,ends", PartsOf("", "", false), ASWBCB_HERE, "an empty text too");
    CheckEquals("", PartsOf("abcdef", "CD", false), ASWBCB_HERE, "case matters");
    CheckEquals("contains", PartsOf("abcdef", "CD", true), ASWBCB_HERE, "unless ignored");
    CheckEquals("contains,ends", PartsOf(L"aXbX", std::wstring(L"xbx"), true), ASWBCB_HERE, "wide strings");
    CheckEquals("contains,starts", PartsOf(UnicodeString(L"Hello"), "HE", true), ASWBCB_HERE);
    CheckEquals("contains", PartsOf(std::string("caf\xC3\xA9 au lait"), L"\x00E9 au", false), ASWBCB_HERE,
        "UTF-8 and UTF-16 compare as Unicode");
    CheckEquals("", PartsOf(L"\x00C9t\x00E9", L"\x00E9T\x00C9", true), ASWBCB_HERE, "non-ASCII letters don't fold");
    CheckEquals("", PartsOf(nullString, "", false), ASWBCB_HERE, "a null text has no parts");
    CheckEquals("", PartsOf("abc", nullString, false), ASWBCB_HERE, "a null part is in no text");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Texts_IgnoringASCIICase()
{
    char const* const nullString = nullptr;
    wchar_t const* const wideNull = nullptr;

    CheckEquals("equal", CompareIC("abcXYZ", "ABCxyz"), ASWBCB_HERE);
    CheckEquals("different", CompareIC("abc", "abd"), ASWBCB_HERE);
    CheckEquals("different", CompareIC("ab", "abc"), ASWBCB_HERE, "different lengths");
    CheckEquals("different", CompareIC("@[`{", "`{@["), ASWBCB_HERE, "only the letters A-Z fold, not their neighbors");
    CheckEquals("equal", CompareIC(L"abc", std::wstring(L"ABC")), ASWBCB_HERE, "wide strings");
    CheckEquals("equal", CompareIC(std::string("abc"), L"AbC"), ASWBCB_HERE, "narrow and wide");
    CheckEquals("equal", CompareIC(UnicodeString(L"abc"), "ABC"), ASWBCB_HERE, "UnicodeString and narrow");
    CheckEquals("equal", CompareIC(AnsiString("aBc"), UTF8String("AbC")), ASWBCB_HERE);
    CheckEquals("different", CompareIC(L"caf\x00E9", L"CAF\x00C9"), ASWBCB_HERE, "non-ASCII letters don't fold");
    CheckEquals("equal", CompareIC("caf\xC3\xA9", "CAF\xC3\xA9"), ASWBCB_HERE, "UTF-8 bytes stay as they are");
    CheckEquals("different", CompareIC(nullString, ""), ASWBCB_HERE, "nullptr isn't empty");
    CheckEquals("equal", CompareIC(nullString, nullString), ASWBCB_HERE);
    CheckEquals("different", CompareIC(wideNull, UnicodeString()), ASWBCB_HERE, "nullptr isn't empty");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_TypeCheckFailure()
{
    using TestShapes::TCircle;
    using TestShapes::TShape;
    using TestShapes::TSquare;

    std::string const circle = DescribeTypeInfo(typeid(TCircle));
    std::string const square = DescribeTypeInfo(typeid(TSquare));
    TCircle circleObject;
    TShape* const shape = &circleObject;
    TShape* const nullShape = nullptr;
    std::unique_ptr<TShape> const owned(new TSquare());
    std::unique_ptr<TObject> const list(new TStringList());

    CheckEquals("", TypeCheckFailure<TCircle>(shape, true), ASWBCB_HERE, "a raw pointer");
    CheckEquals("", TypeCheckFailure<TShape>(shape, true), ASWBCB_HERE, "a descendant counts");
    CheckEquals("Expected type \"" + square + "\" but was \"" + circle + "\"", TypeCheckFailure<TSquare>(shape, true),
        ASWBCB_HERE);
    CheckEquals("", TypeCheckFailure<TSquare>(shape, false), ASWBCB_HERE);
    CheckEquals("Expected not type \"" + circle + "\" but was \"" + circle + "\"",
        TypeCheckFailure<TCircle>(shape, false), ASWBCB_HERE);
    CheckEquals("Expected type \"" + circle + "\" but was null", TypeCheckFailure<TCircle>(nullShape, true),
        ASWBCB_HERE, "a null pointer is never a TType");
    CheckEquals("", TypeCheckFailure<TCircle>(nullShape, false), ASWBCB_HERE);
    CheckEquals("", TypeCheckFailure<TCircle>(circleObject, true), ASWBCB_HERE, "an object");
    CheckEquals("", TypeCheckFailure<TSquare>(owned, true), ASWBCB_HERE, "std::unique_ptr");
    CheckEquals("", TypeCheckFailure<TStrings>(list.get(), true), ASWBCB_HERE, "an RTL class");
    CheckEquals("Expected type \"Exception\" but was \"TStringList\"",
        TypeCheckFailure<Exception>(list.get(), true), ASWBCB_HERE, "RTL classes by ClassName()");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_Unicode_AcrossTypes()
{
    CheckEquals("equal", Compare(L"caf\x00E9", "caf\xC3\xA9"), ASWBCB_HERE, "UTF-16 and UTF-8");
    CheckEquals("equal", Compare(UnicodeString(L"caf\x00E9"), std::string("caf\xC3\xA9")), ASWBCB_HERE);
    CheckEquals("equal", Compare(UTF8String("caf\xC3\xA9"), L"caf\x00E9"), ASWBCB_HERE);
    CheckEquals("\"caf\xC3\xA9\" vs \"cafe\"", Compare(L"caf\x00E9", "cafe"), ASWBCB_HERE, "described as UTF-8");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Values::Test_WideStrings()
{
    std::wstring const w = L"abc";
    wchar_t const* wideNull = nullptr;

    CheckEquals("equal", Compare(L"abc", w), ASWBCB_HERE);
    CheckEquals("\"abc\" vs \"ab\"", Compare(w, L"ab"), ASWBCB_HERE);
    CheckEquals("\"(null)\" vs \"\"", Compare(wideNull, L""), ASWBCB_HERE, "nullptr isn't empty");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_Values, "SelfTests")
