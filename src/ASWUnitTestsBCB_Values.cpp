/* **************************************************************************
ASWUnitTestsBCB_Values.cpp
Author: Anthony S. West - ASW Software

Compares and describes the values that assertions take.

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
#include "ASWUnitTestsBCB_Values.h"
//---------------------------------------------------------------------------
#include <cfloat>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <locale>
#include <memory>
#include <sstream>
#if defined(__clang__) && defined(__MINGW32__) // bcc64x; bcc32c has no <cxxabi.h>
#include <cxxabi.h>
#endif
//---------------------------------------------------------------------------
#include <System.RegularExpressionsCore.hpp>
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Regularexpressionscore;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

// True when the 'length' characters at 'a' and at 'b' are the same, optionally ignoring the case of the ASCII
// letters A-Z only.
template <typename TChar>
bool CharsMatch(TChar const* a, TChar const* b, std::size_t length, bool ignoreCase);

// 'value' with 'precision' significant digits, in the classic locale, and "nan", "inf" or "-inf" with every compiler.
template <typename TFloat>
std::string FormatFloatingPoint(TFloat value, int precision);

// See ASWUnitTestsBCB::DescribeFloatingPointPair().
template <typename TFloat>
void FormatFloatingPointPair(TFloat value, TFloat bound, std::string& valueText, std::string& boundText);

// The bytes at 'bytes' from 'offset' up to 'size' in uppercase hexadecimal, separated by spaces (e.g. "01 FF"): at
// most 16 of them, followed by " ..." when there are more.
std::string HexBytes(unsigned char const* bytes, std::size_t offset, std::size_t size);

// See ASWUnitTestsBCB::HasPart().
template <typename TString>
bool StringHasPart(TString const& text, TString const& part, ASWUnitTestsBCB::TTextPart::Type where,
    bool ignoreCase);

// 'character' with an ASCII letter A-Z made lowercase, and anything else unchanged.
template <typename TChar>
TChar ToLowerASCII(TChar character);

//---------------------------------------------------------------------------
template <typename TChar>
bool CharsMatch(TChar const* a, TChar const* b, std::size_t length, bool ignoreCase)
{
    for (std::size_t i = 0; i < length; ++i)
    {
        if (ignoreCase ? (ToLowerASCII(a[i]) != ToLowerASCII(b[i])) : (a[i] != b[i]))
            return false;
    }

    return true;
}
//---------------------------------------------------------------------------
template <typename TFloat>
std::string FormatFloatingPoint(TFloat value, int precision)
{
    // The compilers' runtime libraries spell these differently (e.g. "+NAN").
    if (value != value)
        return "nan";

    if (value > std::numeric_limits<TFloat>::max())
        return "inf";

    if (value < -std::numeric_limits<TFloat>::max())
        return "-inf";

    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(precision) << value;
    return stream.str();
}
//---------------------------------------------------------------------------
template <typename TFloat>
void FormatFloatingPointPair(TFloat value, TFloat bound, std::string& valueText, std::string& boundText)
{
    valueText = FormatFloatingPoint(value, std::numeric_limits<TFloat>::digits10);
    boundText = FormatFloatingPoint(bound, std::numeric_limits<TFloat>::digits10);

    if ((valueText == boundText) && (value != bound))
    {
        // C++11's max_digits10: the digits that tell every two values apart.
        int const allDigits = 2 + (std::numeric_limits<TFloat>::digits * 30103) / 100000;

        valueText = FormatFloatingPoint(value, allDigits);
        boundText = FormatFloatingPoint(bound, allDigits);
    }
}
//---------------------------------------------------------------------------
std::string HexBytes(unsigned char const* bytes, std::size_t offset, std::size_t size)
{
    std::size_t const maxShown = 16;
    std::size_t const end = ((size - offset) > maxShown) ? (offset + maxShown) : size;

    std::ostringstream stream;
    stream << std::uppercase << std::hex << std::setfill('0');

    for (std::size_t i = offset; i < end; ++i)
        stream << ((i == offset) ? "" : " ") << std::setw(2) << static_cast<unsigned int>(bytes[i]);

    if (end < size)
        stream << " ...";

    return stream.str();
}
//---------------------------------------------------------------------------
template <typename TString>
bool StringHasPart(TString const& text, TString const& part, ASWUnitTestsBCB::TTextPart::Type where,
    bool ignoreCase)
{
    if (part.size() > text.size())
        return false;

    std::size_t const lastStart = text.size() - part.size();

    switch (where)
    {
        case ASWUnitTestsBCB::TTextPart::Contains:
            for (std::size_t start = 0; start <= lastStart; ++start)
            {
                if (CharsMatch(text.data() + start, part.data(), part.size(), ignoreCase))
                    return true;
            }
            return false;

        case ASWUnitTestsBCB::TTextPart::EndsWith:
            return CharsMatch(text.data() + lastStart, part.data(), part.size(), ignoreCase);

        case ASWUnitTestsBCB::TTextPart::StartsWith:
            return CharsMatch(text.data(), part.data(), part.size(), ignoreCase);
    }

    return false;
}
//---------------------------------------------------------------------------
template <typename TChar>
TChar ToLowerASCII(TChar character)
{
    if ((character >= 'A') && (character <= 'Z'))
        return static_cast<TChar>(character - 'A' + 'a');

    return character;
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

//---------------------------------------------------------------------------
std::string DescribeAddress(unsigned __int64 address)
{
    std::ostringstream stream;
    stream << "0x" << std::hex << address;
    return stream.str();
}
//---------------------------------------------------------------------------
std::string DescribeBool(bool value)
{
    return value ? "true" : "false";
}
//---------------------------------------------------------------------------
std::string DescribeElementCount(unsigned __int64 count)
{
    return "had " + DescribeUnsignedInteger(count) + ((count == 1) ? " element" : " elements");
}
//---------------------------------------------------------------------------
std::string DescribeFixed(double value)
{
    // The compilers' runtime libraries spell these differently (e.g. "+INF").
    if (value != value)
        return "nan";

    if (value > DBL_MAX)
        return "inf";

    if (value < -DBL_MAX)
        return "-inf";

    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(6) << value;
    return stream.str();
}
//---------------------------------------------------------------------------
void DescribeFloatingPointPair(float value, float bound, std::string& valueText, std::string& boundText)
{
    FormatFloatingPointPair(value, bound, valueText, boundText);
}
//---------------------------------------------------------------------------
void DescribeFloatingPointPair(double value, double bound, std::string& valueText, std::string& boundText)
{
    FormatFloatingPointPair(value, bound, valueText, boundText);
}
//---------------------------------------------------------------------------
void DescribeFloatingPointPair(long double value, long double bound, std::string& valueText,
    std::string& boundText)
{
    FormatFloatingPointPair(value, bound, valueText, boundText);
}
//---------------------------------------------------------------------------
std::string DescribeRTLClass(TClass cls)
{
    return ToUTF8(cls->ClassName());
}
//---------------------------------------------------------------------------
std::string DescribeSignedInteger(__int64 value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}
//---------------------------------------------------------------------------
std::string DescribeString(char const* text)
{
    return (text != nullptr) ? std::string(text) : std::string("(null)");
}
//---------------------------------------------------------------------------
std::string DescribeString(std::string const& text)
{
    return text;
}
//---------------------------------------------------------------------------
std::string DescribeString(wchar_t const* text)
{
    return (text != nullptr) ? ToUTF8(UnicodeString(text)) : std::string("(null)");
}
//---------------------------------------------------------------------------
std::string DescribeString(std::wstring const& text)
{
    return ToUTF8(UnicodeText(text));
}
//---------------------------------------------------------------------------
std::string DescribeString(AnsiString const& text)
{
    return ToUTF8(UnicodeText(text));
}
//---------------------------------------------------------------------------
std::string DescribeString(UnicodeString const& text)
{
    return ToUTF8(text);
}
//---------------------------------------------------------------------------
std::string DescribeString(UTF8String const& text)
{
    return std::string(text.c_str(), static_cast<std::string::size_type>(text.Length()));
}
//---------------------------------------------------------------------------
std::string DescribeTypeInfo(std::type_info const& type)
{
#if defined(__clang__) && defined(__MINGW32__) // bcc64x: name() is mangled
    int status = 0;
    char* const demangled = abi::__cxa_demangle(type.name(), nullptr, nullptr, &status);

    if (demangled != nullptr)
    {
        std::string const name(demangled);
        std::free(demangled);
        return name;
    }
#endif

    return type.name();
}
//---------------------------------------------------------------------------
std::string DescribeUnsignedInteger(unsigned __int64 value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}
//---------------------------------------------------------------------------
bool EqualsIgnoringASCIICase(std::string const& a, std::string const& b)
{
    return (a.size() == b.size()) && CharsMatch(a.data(), b.data(), a.size(), true);
}
//---------------------------------------------------------------------------
bool EqualsIgnoringASCIICase(std::wstring const& a, std::wstring const& b)
{
    return (a.size() == b.size()) && CharsMatch(a.data(), b.data(), a.size(), true);
}
//---------------------------------------------------------------------------
bool EqualsIgnoringASCIICase(UnicodeString const& a, UnicodeString const& b)
{
    return (a.Length() == b.Length()) && CharsMatch(a.c_str(), b.c_str(), static_cast<std::size_t>(a.Length()),
        true);
}
//---------------------------------------------------------------------------
bool HasPart(std::string const& text, std::string const& part, TTextPart::Type where, bool ignoreCase)
{
    return StringHasPart(text, part, where, ignoreCase);
}
//---------------------------------------------------------------------------
bool HasPart(std::wstring const& text, std::wstring const& part, TTextPart::Type where, bool ignoreCase)
{
    return StringHasPart(text, part, where, ignoreCase);
}
//---------------------------------------------------------------------------
bool IsNullString(char const* text)
{
    return text == nullptr;
}
//---------------------------------------------------------------------------
bool IsNullString(std::string const& /*text*/)
{
    return false;
}
//---------------------------------------------------------------------------
bool IsNullString(wchar_t const* text)
{
    return text == nullptr;
}
//---------------------------------------------------------------------------
bool IsNullString(std::wstring const& /*text*/)
{
    return false;
}
//---------------------------------------------------------------------------
bool IsNullString(AnsiString const& /*text*/)
{
    return false;
}
//---------------------------------------------------------------------------
bool IsNullString(UnicodeString const& /*text*/)
{
    return false;
}
//---------------------------------------------------------------------------
bool IsNullString(UTF8String const& /*text*/)
{
    return false;
}
//---------------------------------------------------------------------------
bool MatchesRegularExpression(UnicodeString const& text, UnicodeString const& pattern, bool& valid,
    std::string& error)
{
    valid = true;

    try
    {
        // TPerlRegEx, which TRegEx uses with these same default options, since TRegEx can't match empty text (see
        // below).
        std::unique_ptr<TPerlRegEx> regex(
            new TPerlRegEx());

        // The pattern is compiled on its own first, so a malformed one can't turn valid inside the anchors (e.g.
        // "a)(b"). The RTL rejects an empty pattern, which needs no check.
        if (!pattern.IsEmpty())
        {
            regex->RegEx = pattern;
            regex->Compile();
        }

        // Anchored at both ends, so the whole text must match, as std::regex_match() does. The \E, which PCRE
        // ignores on its own, ends a \Q quote that the pattern leaves open.
        regex->RegEx = UnicodeString(L"\\A(?:") + pattern + L"\\E)\\z";

        // An empty UnicodeString is a null pointer, which PCRE rejects, so empty text is passed as one character
        // with a stop of 0. The stop is set after the subject, which resets it.
        regex->Subject = text.IsEmpty() ? UnicodeString(L" ") : text;
        regex->Stop = text.Length();
        return regex->Match();
    }
    catch (Exception& regexEx)
    {
        valid = false;
        error = ToUTF8(regexEx.Message);
    }

    return false;
}
//---------------------------------------------------------------------------
std::string MemoryFailure(void const* expected, void const* actual, std::size_t size, bool expectEqual)
{
    unsigned char const* const expectedBytes = static_cast<unsigned char const*>(expected);
    unsigned char const* const actualBytes = static_cast<unsigned char const*>(actual);
    std::string const sizeText = DescribeUnsignedInteger(size);

    if ((size > 0) && ((expectedBytes == nullptr) != (actualBytes == nullptr)))
    {
        if (!expectEqual)
            return std::string();

        return "Cannot compare " + sizeText + " bytes: " + ((expectedBytes == nullptr) ? "expected" : "actual") +
            " is null";
    }

    std::size_t offset = size;

    if (expectedBytes != actualBytes)
    {
        offset = 0;

        while ((offset < size) && (expectedBytes[offset] == actualBytes[offset]))
            ++offset;
    }

    bool const equal = (offset == size);

    if (equal == expectEqual)
        return std::string();

    if (expectEqual)
    {
        return "Bytes differ at offset " + DescribeUnsignedInteger(offset) + " of " + sizeText + ": expected \"" +
            HexBytes(expectedBytes, offset, size) + "\" but was \"" + HexBytes(actualBytes, offset, size) + "\"";
    }

    std::string const bytes = (expectedBytes == nullptr) ? std::string() : HexBytes(expectedBytes, 0, size);

    return "Expected the " + sizeText + " bytes to differ, but both are \"" + bytes + "\"";
}
//---------------------------------------------------------------------------
std::string NarrowText(char const* text)
{
    return (text != nullptr) ? std::string(text) : std::string();
}
//---------------------------------------------------------------------------
std::string NarrowText(std::string const& text)
{
    return text;
}
//---------------------------------------------------------------------------
bool OrderingHolds(TOrder::Type order, TOrdering::Type ordering)
{
    if (order == TOrder::Unordered)
        return false;

    switch (ordering)
    {
        case TOrdering::GreaterThan:
            return order == TOrder::Greater;

        case TOrdering::GreaterThanOrEqual:
            return order != TOrder::Less;

        case TOrdering::LessThan:
            return order == TOrder::Less;

        case TOrdering::LessThanOrEqual:
            return order != TOrder::Greater;
    }

    return false;
}
//---------------------------------------------------------------------------
char const* OrderingSymbol(TOrdering::Type ordering)
{
    switch (ordering)
    {
        case TOrdering::GreaterThan:
            return ">";

        case TOrdering::GreaterThanOrEqual:
            return ">=";

        case TOrdering::LessThan:
            return "<";

        case TOrdering::LessThanOrEqual:
            return "<=";
    }

    return "?";
}
//---------------------------------------------------------------------------
UnicodeString UnicodeText(char const* text)
{
    return (text != nullptr) ? UnicodeString(UTF8String(text)) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString UnicodeText(std::string const& text)
{
    return UnicodeString(UTF8String(text.c_str(), static_cast<int>(text.size())));
}
//---------------------------------------------------------------------------
UnicodeString UnicodeText(wchar_t const* text)
{
    return (text != nullptr) ? UnicodeString(text) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString UnicodeText(std::wstring const& text)
{
    return UnicodeString(text.c_str(), static_cast<int>(text.size()));
}
//---------------------------------------------------------------------------
UnicodeString UnicodeText(AnsiString const& text)
{
    return UnicodeString(text);
}
//---------------------------------------------------------------------------
UnicodeString UnicodeText(UnicodeString const& text)
{
    return text;
}
//---------------------------------------------------------------------------
UnicodeString UnicodeText(UTF8String const& text)
{
    return UnicodeString(text);
}
//---------------------------------------------------------------------------
std::wstring WideText(wchar_t const* text)
{
    return (text != nullptr) ? std::wstring(text) : std::wstring();
}
//---------------------------------------------------------------------------
std::wstring WideText(std::wstring const& text)
{
    return text;
}
//---------------------------------------------------------------------------
std::wstring WideText(UnicodeString const& text)
{
    return std::wstring(text.c_str(), static_cast<std::wstring::size_type>(text.Length()));
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
