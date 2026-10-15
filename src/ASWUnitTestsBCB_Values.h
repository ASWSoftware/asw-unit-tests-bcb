/* **************************************************************************
ASWUnitTestsBCB_Values.h
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

#ifndef ASWUnitTestsBCB_ValuesH
#define ASWUnitTestsBCB_ValuesH
//---------------------------------------------------------------------------
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <typeinfo>
//---------------------------------------------------------------------------
#include <System.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

// Each assertion family is one function template that classifies both of its values' types into a kind
// (TValueKind) and calls the overload for that kind (tag dispatch). bcc32 mis-resolves a set of competing SFINAE
// templates beside non-template overloads (it called CheckEquals(bool, bool) for CheckEquals(1, 2)), so this file
// never offers one. To support a new kind of value, add its kind struct, its TValueKind specializations (and any
// TValuePairKind ones), and its ValuesEqual() and DescribeValue() overloads.

/////////////////////////////////////////////////////////////////////////////
// Value kinds
/////////////////////////////////////////////////////////////////////////////
struct TValueKindBool
{
};

// A class with empty() and size() members, e.g. a std::vector or a System::DynamicArray; only the Empty assertions
// take one (see TEmptinessKind).
struct TValueKindContainer
{
};

// An enum type, compared (with another enum or an integer) as its integer value, as an integer kind.
struct TValueKindEnum
{
};

// float, double or long double; only the ordering assertions take one (see TOrderingKind).
struct TValueKindFloatingPoint
{
};

// Any integer type but bool, compared by value whatever its size and signedness (-1 never equals 0xFFFFFFFF).
struct TValueKindInteger
{
};

// Any pointer but a C string (char or wchar_t), compared by address.
struct TValueKindPointer
{
};

// char*, char const*, char arrays and std::string, read as UTF-8.
struct TValueKindNarrowString
{
};

// wchar_t*, wchar_t const*, wchar_t arrays and std::wstring (UTF-16).
struct TValueKindWideString
{
};

// System::UnicodeString, System::AnsiString and System::UTF8String; also any mix of string kinds, compared as Unicode.
struct TValueKindRTLString
{
};

struct TValueKindUnsupported
{
};

// True when T is an enum type.
template <typename T>
struct TIsEnum
{
    static const bool Value = __is_enum(T);
};

// The kind of a value of type T that no specialization of TValueKind names: an enum, or unsupported.
template <typename T, bool IsEnum>
struct TDefaultValueKind
{
    typedef TValueKindUnsupported Type;
};

template <typename T>
struct TDefaultValueKind<T, true>
{
    typedef TValueKindEnum Type;
};

// The kind of a value of type T.
template <typename T>
struct TValueKind
{
    typedef typename TDefaultValueKind<T, TIsEnum<T>::Value>::Type Type;
};

template <typename T>
struct TValueKind<T*>
{
    typedef TValueKindPointer Type;
};

template <>
struct TValueKind<bool>
{
    typedef TValueKindBool Type;
};

// The integer types, with their signedness.
template <typename T>
struct TIntegerTraits;

#define ASWUnitTestsBCB_INTEGER_TYPE(IntegerType, isSigned) \
    template <> \
    struct TIntegerTraits<IntegerType> \
    { \
        static const bool IsSigned = isSigned; \
    }; \
    template <> \
    struct TValueKind<IntegerType> \
    { \
        typedef TValueKindInteger Type; \
    };

ASWUnitTestsBCB_INTEGER_TYPE(char, (static_cast<char>(-1) < static_cast<char>(0)))
ASWUnitTestsBCB_INTEGER_TYPE(signed char, true)
ASWUnitTestsBCB_INTEGER_TYPE(unsigned char, false)
ASWUnitTestsBCB_INTEGER_TYPE(wchar_t, (static_cast<wchar_t>(-1) < static_cast<wchar_t>(0)))
ASWUnitTestsBCB_INTEGER_TYPE(short, true)
ASWUnitTestsBCB_INTEGER_TYPE(unsigned short, false)
ASWUnitTestsBCB_INTEGER_TYPE(int, true)
ASWUnitTestsBCB_INTEGER_TYPE(unsigned int, false)
ASWUnitTestsBCB_INTEGER_TYPE(long, true)
ASWUnitTestsBCB_INTEGER_TYPE(unsigned long, false)
ASWUnitTestsBCB_INTEGER_TYPE(__int64, true) // Also long long, the same type on every supported compiler
ASWUnitTestsBCB_INTEGER_TYPE(unsigned __int64, false)

#undef ASWUnitTestsBCB_INTEGER_TYPE

template <>
struct TValueKind<char*>
{
    typedef TValueKindNarrowString Type;
};

template <>
struct TValueKind<char const*>
{
    typedef TValueKindNarrowString Type;
};

template <std::size_t Length>
struct TValueKind<char[Length]>
{
    typedef TValueKindNarrowString Type;
};

template <std::size_t Length>
struct TValueKind<char const[Length]>
{
    typedef TValueKindNarrowString Type;
};

template <>
struct TValueKind<std::string>
{
    typedef TValueKindNarrowString Type;
};

template <>
struct TValueKind<wchar_t*>
{
    typedef TValueKindWideString Type;
};

template <>
struct TValueKind<wchar_t const*>
{
    typedef TValueKindWideString Type;
};

template <std::size_t Length>
struct TValueKind<wchar_t[Length]>
{
    typedef TValueKindWideString Type;
};

template <std::size_t Length>
struct TValueKind<wchar_t const[Length]>
{
    typedef TValueKindWideString Type;
};

template <>
struct TValueKind<std::wstring>
{
    typedef TValueKindWideString Type;
};

template <>
struct TValueKind<System::AnsiString>
{
    typedef TValueKindRTLString Type;
};

template <>
struct TValueKind<System::UnicodeString>
{
    typedef TValueKindRTLString Type;
};

template <>
struct TValueKind<System::UTF8String>
{
    typedef TValueKindRTLString Type;
};

// The kind two values are compared as: their common kind, an integer for enums (with each other or with integers),
// or, for two different string kinds, an RTL string.
template <typename TExpectedKind, typename TActualKind>
struct TValuePairKind
{
    typedef TValueKindUnsupported Type;
};

template <typename TKind>
struct TValuePairKind<TKind, TKind>
{
    typedef TKind Type;
};

template <>
struct TValuePairKind<TValueKindEnum, TValueKindEnum>
{
    typedef TValueKindInteger Type;
};

template <>
struct TValuePairKind<TValueKindEnum, TValueKindInteger>
{
    typedef TValueKindInteger Type;
};

template <>
struct TValuePairKind<TValueKindInteger, TValueKindEnum>
{
    typedef TValueKindInteger Type;
};

template <>
struct TValuePairKind<TValueKindNarrowString, TValueKindWideString>
{
    typedef TValueKindRTLString Type;
};

template <>
struct TValuePairKind<TValueKindNarrowString, TValueKindRTLString>
{
    typedef TValueKindRTLString Type;
};

template <>
struct TValuePairKind<TValueKindWideString, TValueKindNarrowString>
{
    typedef TValueKindRTLString Type;
};

template <>
struct TValuePairKind<TValueKindWideString, TValueKindRTLString>
{
    typedef TValueKindRTLString Type;
};

template <>
struct TValuePairKind<TValueKindRTLString, TValueKindNarrowString>
{
    typedef TValueKindRTLString Type;
};

template <>
struct TValuePairKind<TValueKindRTLString, TValueKindWideString>
{
    typedef TValueKindRTLString Type;
};

// Never defined: an assertion given two values it can't compare fails to compile with this name in the error.
template <typename TExpected, typename TActual>
struct TValuesCannotBeCompared;

// The kind two values are compared as by the assertions that take only strings (EqualsIC, Contains, ...): their
// string kind (see TValuePairKind), or unsupported when they aren't both strings.
template <typename TPairKind>
struct TTextPairKind
{
    typedef TValueKindUnsupported Type;
};

template <>
struct TTextPairKind<TValueKindNarrowString>
{
    typedef TValueKindNarrowString Type;
};

template <>
struct TTextPairKind<TValueKindRTLString>
{
    typedef TValueKindRTLString Type;
};

template <>
struct TTextPairKind<TValueKindWideString>
{
    typedef TValueKindWideString Type;
};

// Never defined: an assertion that takes only strings, given two values that aren't both strings, fails to compile
// with this name in the error.
template <typename TExpected, typename TActual>
struct TValuesAreNotTexts;

template <bool Value>
struct TBoolTag
{
};


// Whether a value the Empty assertions take is empty (see ValueEmptiness()).
struct TEmptiness
{
    enum Type
    {
        Empty,
        NotEmpty,
        NullString, // A null C string, which is neither: it isn't a text, not even an empty one
    };
};

// How a value compares with a bound (see CompareOrdered()).
struct TOrder
{
    enum Type
    {
        Less,
        Equal,
        Greater,
        Unordered, // A NaN is in no order
    };
};

// The relation an ordering assertion (e.g. CheckGreaterThan()) checks a value has to a bound.
struct TOrdering
{
    enum Type
    {
        GreaterThan,
        GreaterThanOrEqual,
        LessThan,
        LessThanOrEqual,
    };
};

// Where a text's part is looked for (see TextHasPart()).
struct TTextPart
{
    enum Type
    {
        Contains,  // Anywhere
        EndsWith,  // At the end
        StartsWith, // At the start
    };
};


/////////////////////////////////////////////////////////////////////////////
// Describing and converting values (ASWUnitTestsBCB_Values.cpp)
/////////////////////////////////////////////////////////////////////////////

// An address in hexadecimal for a failure message, e.g. "0x12ff40".
std::string DescribeAddress(unsigned __int64 address);
std::string DescribeBool(bool value);
// A container's size for an Empty failure, e.g. "had 3 elements" or "had 1 element".
std::string DescribeElementCount(unsigned __int64 count);
// A floating-point value with six decimals, as std::to_string() shows it (e.g. "0.100000"), and "nan", "inf" or
// "-inf" with every compiler.
std::string DescribeFixed(double value);
// Describes a value and a bound of an ordering failure in their common floating-point type: with as many significant
// digits as the type reliably holds (digits10), or with all of them (max_digits10) when that would make two
// different values look the same; "nan", "inf" and "-inf" with every compiler.
void DescribeFloatingPointPair(float value, float bound, std::string& valueText, std::string& boundText);
void DescribeFloatingPointPair(double value, double bound, std::string& valueText, std::string& boundText);
void DescribeFloatingPointPair(long double value, long double bound, std::string& valueText,
    std::string& boundText);
// An RTL class's name, e.g. "TStringList".
std::string DescribeRTLClass(System::TClass cls);
std::string DescribeSignedInteger(__int64 value);

// A string value as UTF-8 for a failure message: "(null)" for a null pointer, and an AnsiString converted from the
// system code page.
std::string DescribeString(char const* text);
std::string DescribeString(std::string const& text);
std::string DescribeString(wchar_t const* text);
std::string DescribeString(std::wstring const& text);
std::string DescribeString(System::AnsiString const& text);
std::string DescribeString(System::UnicodeString const& text);
std::string DescribeString(System::UTF8String const& text);

// A C++ type's name, e.g. "NS::TCircle": demangled with bcc64x, and as the compiler gives it otherwise (readable with
// bcc32, Borland-mangled with bcc32c, e.g. "$NS@TCircle").
std::string DescribeTypeInfo(std::type_info const& type);

std::string DescribeUnsignedInteger(unsigned __int64 value);

// True when 'a' and 'b' are the same text, ignoring the case of the ASCII letters A-Z only, so the result doesn't
// depend on the locale, and no byte of a multi-byte UTF-8 sequence is changed.
bool EqualsIgnoringASCIICase(std::string const& a, std::string const& b);
bool EqualsIgnoringASCIICase(std::wstring const& a, std::wstring const& b);
bool EqualsIgnoringASCIICase(System::UnicodeString const& a, System::UnicodeString const& b);

// True when 'text' has 'part' where 'where' says, optionally ignoring the case of the ASCII letters A-Z only (see
// EqualsIgnoringASCIICase()). Every text has an empty part anywhere.
bool HasPart(std::string const& text, std::string const& part, TTextPart::Type where, bool ignoreCase);
bool HasPart(std::wstring const& text, std::wstring const& part, TTextPart::Type where, bool ignoreCase);

// True for a null C string pointer; a null C string only equals another null one.
bool IsNullString(char const* text);
bool IsNullString(std::string const& text);
bool IsNullString(wchar_t const* text);
bool IsNullString(std::wstring const& text);
bool IsNullString(System::AnsiString const& text);
bool IsNullString(System::UnicodeString const& text);
bool IsNullString(System::UTF8String const& text);

// True when the whole of 'text' matches the regular expression 'pattern' (PCRE syntax, as the RTL's TRegEx; an
// empty pattern matches only empty text). 'valid' is false, with the RTL's description in 'error', when 'pattern'
// isn't a valid regular expression; then the result is false.
bool MatchesRegularExpression(System::UnicodeString const& text, System::UnicodeString const& pattern, bool& valid,
    std::string& error);

// Why the 'size' bytes at 'actual' fail an EqualsMem check against those at 'expected' (with 'expectEqual') or a
// NotEqualsMem check, or "" when they pass. Zero bytes, or the same pointer twice (even nullptr), are equal without
// reading either; exactly one null pointer is never equal, and nothing is read. A failure shows the offset of the
// first differing byte and up to 16 bytes from there in hexadecimal (e.g. "01 02 FF ...").
std::string MemoryFailure(void const* expected, void const* actual, std::size_t size, bool expectEqual);

// A narrow string's text (empty for nullptr).
std::string NarrowText(char const* text);
std::string NarrowText(std::string const& text);

// True when 'order' (see CompareOrdered()) satisfies 'ordering'; never for TOrder::Unordered.
bool OrderingHolds(TOrder::Type order, TOrdering::Type ordering);
// The operator for 'ordering' in a failure message, e.g. ">=".
char const* OrderingSymbol(TOrdering::Type ordering);

// A string's text as Unicode (empty for nullptr): narrow text is read as UTF-8, and an AnsiString in the system code
// page.
System::UnicodeString UnicodeText(char const* text);
System::UnicodeString UnicodeText(std::string const& text);
System::UnicodeString UnicodeText(wchar_t const* text);
System::UnicodeString UnicodeText(std::wstring const& text);
System::UnicodeString UnicodeText(System::AnsiString const& text);
System::UnicodeString UnicodeText(System::UnicodeString const& text);
System::UnicodeString UnicodeText(System::UTF8String const& text);

// A wide string's text (empty for nullptr), or a UnicodeString's.
std::wstring WideText(wchar_t const* text);
std::wstring WideText(std::wstring const& text);
std::wstring WideText(System::UnicodeString const& text);


/////////////////////////////////////////////////////////////////////////////
// Integers
/////////////////////////////////////////////////////////////////////////////

template <typename TInteger>
bool IsNegativeInteger(TInteger value, TBoolTag<true> /*isSigned*/)
{
    return value < 0;
}

template <typename TInteger>
bool IsNegativeInteger(TInteger /*value*/, TBoolTag<false> /*isSigned*/)
{
    return false;
}

template <typename TInteger>
bool IsNegativeInteger(TInteger value)
{
    return IsNegativeInteger(value, TBoolTag<TIntegerTraits<TInteger>::IsSigned>());
}

template <typename TInteger>
std::string DescribeInteger(TInteger value)
{
    if (IsNegativeInteger(value))
        return DescribeSignedInteger(static_cast<__int64>(value));

    return DescribeUnsignedInteger(static_cast<unsigned __int64>(value));
}

// True when two integers of any types have the same value.
template <typename TExpected, typename TActual>
bool IntegersEqual(TExpected expected, TActual actual)
{
    bool const expectedNegative = IsNegativeInteger(expected);

    if (expectedNegative != IsNegativeInteger(actual))
        return false;

    if (expectedNegative)
        return static_cast<__int64>(expected) == static_cast<__int64>(actual);

    return static_cast<unsigned __int64>(expected) == static_cast<unsigned __int64>(actual);
}

// A value of an integer kind (TValueKindInteger or TValueKindEnum) as an integer type: an integer as it is, and an
// enum as an __int64.
template <typename T, typename TKind = typename TValueKind<T>::Type>
struct TIntegerValue
{
    typedef T Type;

    static T Get(T value)
    {
        return value;
    }
};

template <typename T>
struct TIntegerValue<T, TValueKindEnum>
{
    typedef __int64 Type;

    static __int64 Get(T value)
    {
        return static_cast<__int64>(value);
    }
};


/////////////////////////////////////////////////////////////////////////////
// Pointers
/////////////////////////////////////////////////////////////////////////////

// A pointer for a failure message: its address (see DescribeAddress()), or "(null)".
template <typename T>
std::string DescribePointer(T* pointer)
{
    if (pointer == nullptr)
        return "(null)";

    // The pointer's bytes are copied, since a function pointer can't portably be cast to an integer.
    unsigned __int64 address = 0;
    std::memcpy(&address, &pointer, (sizeof(pointer) < sizeof(address)) ? sizeof(pointer) : sizeof(address));
    return DescribeAddress(address);
}

// Never defined: a Null assertion given a value that can't be null fails to compile with this name in the error.
template <typename T>
struct TValueCannotBeNull;

// Whether a value is null, for the Null assertions: a raw pointer, a std::unique_ptr or a System::DelphiInterface
// (an interface reference, e.g. _di_IXMLNode). Other values (an array, an integer, nullptr itself) don't compile.
template <typename T>
struct TNullCheck
{
    static bool IsNull(T const& /*value*/)
    {
        return sizeof(TValueCannotBeNull<T>) == 0;
    }
};

template <typename T>
struct TNullCheck<T*>
{
    static bool IsNull(T* value)
    {
        return value == nullptr;
    }
};

template <typename T, typename TDeleter>
struct TNullCheck<std::unique_ptr<T, TDeleter> >
{
    static bool IsNull(std::unique_ptr<T, TDeleter> const& value)
    {
        return value.get() == nullptr;
    }
};

template <typename T>
struct TNullCheck<System::DelphiInterface<T> >
{
    static bool IsNull(System::DelphiInterface<T> const& value)
    {
        return static_cast<T*>(value) == nullptr;
    }
};

// Never defined: a Same assertion given a value without an address fails to compile with this name in the error.
template <typename T>
struct TValueHasNoAddress;

// The address the Same assertions compare: a raw pointer as it is, an array's first element, a std::unique_ptr's
// get(), or a System::DelphiInterface's interface pointer. Other values don't compile.
template <typename T>
struct TSameAddress
{
    typedef void const* Type;

    static Type Get(T const& /*value*/)
    {
        return (sizeof(TValueHasNoAddress<T>) == 0) ? nullptr : nullptr;
    }
};

template <typename T>
struct TSameAddress<T*>
{
    typedef T* Type;

    static Type Get(T* value)
    {
        return value;
    }
};

template <typename T, std::size_t Length>
struct TSameAddress<T[Length]>
{
    typedef T const* Type;

    static Type Get(T const (&value)[Length])
    {
        return &value[0];
    }
};

template <typename T, typename TDeleter>
struct TSameAddress<std::unique_ptr<T, TDeleter> >
{
    typedef typename std::unique_ptr<T, TDeleter>::pointer Type;

    static Type Get(std::unique_ptr<T, TDeleter> const& value)
    {
        return value.get();
    }
};

template <typename T>
struct TSameAddress<System::DelphiInterface<T> >
{
    typedef T* Type;

    static Type Get(System::DelphiInterface<T> const& value)
    {
        return static_cast<T*>(value);
    }
};

// The object the IsType assertions check: a raw pointer as it is, a std::unique_ptr's get(), or an object's
// address. Object is the object's (static) type.
template <typename T>
struct TTypeCheckPointer
{
    typedef T Object;
    typedef T const* Type;

    static Type Get(T const& value)
    {
        // Not &value, which a class may overload.
        return reinterpret_cast<T const*>(&reinterpret_cast<char const&>(value));
    }
};

template <typename T>
struct TTypeCheckPointer<T*>
{
    typedef T Object;
    typedef T* Type;

    static Type Get(T* value)
    {
        return value;
    }
};

template <typename T, typename TDeleter>
struct TTypeCheckPointer<std::unique_ptr<T, TDeleter> >
{
    typedef T Object;
    typedef typename std::unique_ptr<T, TDeleter>::pointer Type;

    static Type Get(std::unique_ptr<T, TDeleter> const& value)
    {
        return value.get();
    }
};


/////////////////////////////////////////////////////////////////////////////
// Class names
//
// A class's name for a failure message: a TObject descendant's
// ClassName() (e.g. "TStringList"), the same with every compiler, and any
// other class's C++ name (see DescribeTypeInfo()).
/////////////////////////////////////////////////////////////////////////////

// Call as DescribeClass<T>(static_cast<T*>(nullptr)): the pointer argument picks the overload by what T derives from.
template <class T>
std::string DescribeClass(System::TObject const* /*typeTag*/)
{
    return DescribeRTLClass(__classid(T));
}

template <class T>
std::string DescribeClass(...)
{
    return DescribeTypeInfo(typeid(T));
}

// The class of the object 'object' points to (its dynamic type). Call as DescribeObjectClass<T>(object, object):
// the second argument picks the overload by what T derives from. (bcc32 doesn't deduce T for this pair.)
template <class T>
std::string DescribeObjectClass(T const* object, System::TObject const* /*typeTag*/)
{
    return DescribeRTLClass(const_cast<T*>(object)->ClassType());
}

template <class T>
std::string DescribeObjectClass(T const* object, ...)
{
    return DescribeTypeInfo(typeid(*object));
}

// Why 'value' fails a check that it is (with 'expectType') or isn't a TType, or "" when it passes. The check uses
// dynamic_cast, so a TType descendant is a TType, and a null pointer never is one.
template <class TType, typename TValue>
std::string TypeCheckFailure(TValue const& value, bool expectType)
{
    typedef typename TTypeCheckPointer<TValue>::Object TObjectType;

    typename TTypeCheckPointer<TValue>::Type const pointer = TTypeCheckPointer<TValue>::Get(value);
    bool const isType = (pointer != nullptr) && (dynamic_cast<TType const*>(pointer) != nullptr);

    if (isType == expectType)
        return std::string();

    std::string const actual = (pointer == nullptr) ? std::string("null") :
            ("\"" + DescribeObjectClass<TObjectType>(pointer, pointer) + "\"");
    return std::string(expectType ? "Expected type \"" : "Expected not type \"") +
        DescribeClass<TType>(static_cast<TType*>(nullptr)) + "\" but was " + actual;
}


/////////////////////////////////////////////////////////////////////////////
// ValuesEqual
//
// True when 'expected' equals 'actual'. Two values of kinds that can't be
// compared (see TValuePairKind) don't compile.
/////////////////////////////////////////////////////////////////////////////

template <typename TExpected, typename TActual>
bool ValuesEqual(TExpected const& expected, TActual const& actual, TValueKindBool /*kind*/)
{
    return expected == actual;
}

template <typename TExpected, typename TActual>
bool ValuesEqual(TExpected const& expected, TActual const& actual, TValueKindInteger /*kind*/)
{
    return IntegersEqual(TIntegerValue<TExpected>::Get(expected), TIntegerValue<TActual>::Get(actual));
}

template <typename TExpected, typename TActual>
bool ValuesEqual(TExpected const& expected, TActual const& actual, TValueKindNarrowString /*kind*/)
{
    return (IsNullString(expected) == IsNullString(actual)) && (NarrowText(expected) == NarrowText(actual));
}

template <typename TExpected, typename TActual>
bool ValuesEqual(TExpected const& expected, TActual const& actual, TValueKindPointer /*kind*/)
{
    return expected == actual;
}

template <typename TExpected, typename TActual>
bool ValuesEqual(TExpected const& expected, TActual const& actual, TValueKindRTLString /*kind*/)
{
    return (IsNullString(expected) == IsNullString(actual)) && (UnicodeText(expected) == UnicodeText(actual));
}

template <typename TExpected, typename TActual>
bool ValuesEqual(TExpected const& expected, TActual const& actual, TValueKindWideString /*kind*/)
{
    return (IsNullString(expected) == IsNullString(actual)) && (WideText(expected) == WideText(actual));
}

template <typename TExpected, typename TActual>
bool ValuesEqual(TExpected const& /*expected*/, TActual const& /*actual*/, TValueKindUnsupported /*kind*/)
{
    return sizeof(TValuesCannotBeCompared<TExpected, TActual>) != 0;
}

template <typename TExpected, typename TActual>
bool ValuesEqual(TExpected const& expected, TActual const& actual)
{
    typedef typename TValuePairKind<typename TValueKind<TExpected>::Type, typename TValueKind<TActual>::Type>::Type
        TKind;

    return ValuesEqual(expected, actual, TKind());
}


/////////////////////////////////////////////////////////////////////////////
// DescribeValue
//
// Describes a value for a failure message, as UTF-8: a bool as "true" or
// "false", an integer or enum as a decimal number (a char too), a pointer
// by its address, and a string as its text ("(null)" for a null C string).
/////////////////////////////////////////////////////////////////////////////

template <typename T>
std::string DescribeValue(T const& value, TValueKindBool /*kind*/)
{
    return DescribeBool(value);
}

template <typename T>
std::string DescribeValue(T const& value, TValueKindEnum /*kind*/)
{
    return DescribeInteger(TIntegerValue<T>::Get(value));
}

template <typename T>
std::string DescribeValue(T const& value, TValueKindInteger /*kind*/)
{
    return DescribeInteger(value);
}

template <typename T>
std::string DescribeValue(T const& value, TValueKindNarrowString /*kind*/)
{
    return DescribeString(value);
}

template <typename T>
std::string DescribeValue(T const& value, TValueKindPointer /*kind*/)
{
    return DescribePointer(value);
}

template <typename T>
std::string DescribeValue(T const& value, TValueKindRTLString /*kind*/)
{
    return DescribeString(value);
}

template <typename T>
std::string DescribeValue(T const& value, TValueKindWideString /*kind*/)
{
    return DescribeString(value);
}

template <typename T>
std::string DescribeValue(T const& value)
{
    typedef typename TValueKind<T>::Type TKind;

    return DescribeValue(value, TKind());
}


/////////////////////////////////////////////////////////////////////////////
// Emptiness
//
// The Empty assertions take a string of any type (empty when it has no
// characters; a null C string is neither empty nor not empty, so it fails
// both Empty and NotEmpty) or a container: any class with empty() and
// size() members, e.g. a std::vector, a std::map or a System::DynamicArray
// (TBytes). Other values (a bool, an integer, an enum, a pointer) don't
// compile.
/////////////////////////////////////////////////////////////////////////////

template <typename TKind>
struct TEmptinessKindOf
{
    typedef TValueKindUnsupported Type;
};

template <>
struct TEmptinessKindOf<TValueKindNarrowString>
{
    typedef TValueKindNarrowString Type;
};

template <>
struct TEmptinessKindOf<TValueKindRTLString>
{
    typedef TValueKindRTLString Type;
};

// A value of no other kind is taken for a container, which fails to compile without empty() and size().
template <>
struct TEmptinessKindOf<TValueKindUnsupported>
{
    typedef TValueKindContainer Type;
};

template <>
struct TEmptinessKindOf<TValueKindWideString>
{
    typedef TValueKindWideString Type;
};

// The kind of a value of type T for the Empty assertions: a string kind, a container, or unsupported.
template <typename T>
struct TEmptinessKind
{
    typedef typename TEmptinessKindOf<typename TValueKind<T>::Type>::Type Type;
};

// Never defined: an Empty assertion given a value that is neither a string nor a container fails to compile with
// this name in the error.
template <typename T>
struct TValueCannotBeEmpty;

template <typename T>
TEmptiness::Type ValueEmptiness(T const& value, TValueKindContainer /*kind*/)
{
    return value.empty() ? TEmptiness::Empty : TEmptiness::NotEmpty;
}

template <typename T>
TEmptiness::Type ValueEmptiness(T const& value, TValueKindNarrowString /*kind*/)
{
    if (IsNullString(value))
        return TEmptiness::NullString;

    return NarrowText(value).empty() ? TEmptiness::Empty : TEmptiness::NotEmpty;
}

template <typename T>
TEmptiness::Type ValueEmptiness(T const& value, TValueKindRTLString /*kind*/)
{
    return UnicodeText(value).IsEmpty() ? TEmptiness::Empty : TEmptiness::NotEmpty;
}

template <typename T>
TEmptiness::Type ValueEmptiness(T const& /*value*/, TValueKindUnsupported /*kind*/)
{
    return (sizeof(TValueCannotBeEmpty<T>) == 0) ? TEmptiness::NotEmpty : TEmptiness::NotEmpty;
}

template <typename T>
TEmptiness::Type ValueEmptiness(T const& value, TValueKindWideString /*kind*/)
{
    if (IsNullString(value))
        return TEmptiness::NullString;

    return WideText(value).empty() ? TEmptiness::Empty : TEmptiness::NotEmpty;
}

// Whether 'value' (a string or a container) is empty, not empty, or a null C string.
template <typename T>
TEmptiness::Type ValueEmptiness(T const& value)
{
    typedef typename TEmptinessKind<T>::Type TKind;

    return ValueEmptiness(value, TKind());
}

template <typename T>
std::string DescribeContents(T const& value, TValueKindContainer /*kind*/)
{
    return DescribeElementCount(static_cast<unsigned __int64>(value.size()));
}

template <typename T>
std::string DescribeContents(T const& value, TValueKindNarrowString /*kind*/)
{
    return "was \"" + DescribeString(value) + "\"";
}

template <typename T>
std::string DescribeContents(T const& value, TValueKindRTLString /*kind*/)
{
    return "was \"" + DescribeString(value) + "\"";
}

template <typename T>
std::string DescribeContents(T const& /*value*/, TValueKindUnsupported /*kind*/)
{
    // Unreachable: ValueEmptiness() doesn't compile for this value (TValueCannotBeEmpty).
    return std::string();
}

template <typename T>
std::string DescribeContents(T const& value, TValueKindWideString /*kind*/)
{
    return "was \"" + DescribeString(value) + "\"";
}

// Describes a value that isn't empty for an Empty failure: a string's text (e.g. "was \"abc\"", "(null)" for a nullptr
// C string), or a container's size (see DescribeElementCount()).
template <typename T>
std::string DescribeContents(T const& value)
{
    typedef typename TEmptinessKind<T>::Type TKind;

    return DescribeContents(value, TKind());
}

// Why 'value' fails an Empty check, e.g. "Expected empty but had 3 elements", or "" when it passes.
template <typename T>
std::string EmptyFailure(T const& value)
{
    if (ValueEmptiness(value) == TEmptiness::Empty)
        return std::string();

    return "Expected empty but " + DescribeContents(value);
}

// Why 'value' fails a NotEmpty check, "Expected not empty but was empty" (or, for a null C string,
// "... but was \"(null)\""), or "" when it passes.
template <typename T>
std::string NotEmptyFailure(T const& value)
{
    TEmptiness::Type const emptiness = ValueEmptiness(value);

    if (emptiness == TEmptiness::NotEmpty)
        return std::string();

    return "Expected not empty but " + ((emptiness == TEmptiness::Empty) ? std::string("was empty") :
            DescribeContents(value));
}


/////////////////////////////////////////////////////////////////////////////
// Ordering
//
// The ordering assertions (e.g. CheckGreaterThan()) take integers (not
// bool or enums; char counts) and floating-point values, in any mix. Two
// integers compare by value, whatever their types (-1 is less than
// 0xFFFFFFFF). Otherwise both convert to their common floating-point type,
// as C++'s arithmetic does (an int with a float compares as float), and a
// NaN is in no order.
/////////////////////////////////////////////////////////////////////////////

template <typename TKind>
struct TOrderingKindOf
{
    typedef TValueKindUnsupported Type;
};

template <>
struct TOrderingKindOf<TValueKindInteger>
{
    typedef TValueKindInteger Type;
};

// The kind of a value of type T for the ordering assertions: integer, floating point or unsupported.
template <typename T>
struct TOrderingKind
{
    typedef typename TOrderingKindOf<typename TValueKind<T>::Type>::Type Type;
};

template <>
struct TOrderingKind<float>
{
    typedef TValueKindFloatingPoint Type;
};

template <>
struct TOrderingKind<double>
{
    typedef TValueKindFloatingPoint Type;
};

template <>
struct TOrderingKind<long double>
{
    typedef TValueKindFloatingPoint Type;
};

// The kind a value and a bound compare as: two integers as integers, and a floating-point value with an integer or
// another floating-point value as floating point.
template <typename TValueOrderingKind, typename TBoundOrderingKind>
struct TOrderingPairKind
{
    typedef TValueKindUnsupported Type;
};

template <typename TKind>
struct TOrderingPairKind<TKind, TKind>
{
    typedef TKind Type;
};

template <>
struct TOrderingPairKind<TValueKindFloatingPoint, TValueKindInteger>
{
    typedef TValueKindFloatingPoint Type;
};

template <>
struct TOrderingPairKind<TValueKindInteger, TValueKindFloatingPoint>
{
    typedef TValueKindFloatingPoint Type;
};

// Never defined: an ordering assertion given values it can't order fails to compile with this name in the error.
template <typename TValue, typename TBound>
struct TValuesCannotBeOrdered;

// A floating-point type's rank in C++'s arithmetic conversions (0 for an integer type).
template <typename T>
struct TFloatingPointRank
{
    static const int Value = 0;
};

template <>
struct TFloatingPointRank<float>
{
    static const int Value = 1;
};

template <>
struct TFloatingPointRank<double>
{
    static const int Value = 2;
};

template <>
struct TFloatingPointRank<long double>
{
    static const int Value = 3;
};

template <int Rank>
struct TFloatingPointOfRank;

template <>
struct TFloatingPointOfRank<1>
{
    typedef float Type;
};

template <>
struct TFloatingPointOfRank<2>
{
    typedef double Type;
};

template <>
struct TFloatingPointOfRank<3>
{
    typedef long double Type;
};

// The floating-point type C++ converts a value of type T1 and one of type T2 to, at least one of them a
// floating-point type: the wider of the two (an integer converting to the other's).
template <typename T1, typename T2>
struct TCommonFloatingPoint
{
    static const int Rank = (TFloatingPointRank<T1>::Value > TFloatingPointRank<T2>::Value) ?
            TFloatingPointRank<T1>::Value : TFloatingPointRank<T2>::Value;

    typedef typename TFloatingPointOfRank<Rank>::Type Type;
};

template <typename T>
TOrder::Type ThreeWayCompare(T a, T b)
{
    return (a < b) ? TOrder::Less : ((b < a) ? TOrder::Greater : TOrder::Equal);
}

template <typename TValue, typename TBound>
TOrder::Type CompareOrdered(TValue const& value, TBound const& bound, TValueKindFloatingPoint /*kind*/)
{
    typedef typename TCommonFloatingPoint<TValue, TBound>::Type TCommon;

    // Volatile, so each value is stored, and so rounded, as TCommon, as the casts require: optimized bcc32c (x87
    // code) kept static_cast<float>(16777217) at full precision in a register, ordering it above 16777216.0f.
    TCommon volatile const commonValue = static_cast<TCommon>(value);
    TCommon volatile const commonBound = static_cast<TCommon>(bound);

    if ((commonValue != commonValue) || (commonBound != commonBound))
        return TOrder::Unordered;

    return ThreeWayCompare(commonValue, commonBound);
}

template <typename TValue, typename TBound>
TOrder::Type CompareOrdered(TValue const& value, TBound const& bound, TValueKindInteger /*kind*/)
{
    bool const valueNegative = IsNegativeInteger(value);

    if (valueNegative != IsNegativeInteger(bound))
        return valueNegative ? TOrder::Less : TOrder::Greater;

    if (valueNegative)
        return ThreeWayCompare(static_cast<__int64>(value), static_cast<__int64>(bound));

    return ThreeWayCompare(static_cast<unsigned __int64>(value), static_cast<unsigned __int64>(bound));
}

template <typename TValue, typename TBound>
TOrder::Type CompareOrdered(TValue const& /*value*/, TBound const& /*bound*/, TValueKindUnsupported /*kind*/)
{
    return (sizeof(TValuesCannotBeOrdered<TValue, TBound>) == 0) ? TOrder::Unordered : TOrder::Unordered;
}

// How 'value' compares with 'bound'.
template <typename TValue, typename TBound>
TOrder::Type CompareOrdered(TValue const& value, TBound const& bound)
{
    typedef typename TOrderingPairKind<typename TOrderingKind<TValue>::Type, typename TOrderingKind<TBound>::Type>::Type
        TKind;

    return CompareOrdered(value, bound, TKind());
}

template <typename TValue, typename TBound>
void DescribeOrderedValues(TValue const& value, TBound const& bound, std::string& valueText, std::string& boundText,
    TValueKindFloatingPoint /*kind*/)
{
    typedef typename TCommonFloatingPoint<TValue, TBound>::Type TCommon;

    DescribeFloatingPointPair(static_cast<TCommon>(value), static_cast<TCommon>(bound), valueText, boundText);
}

template <typename TValue, typename TBound>
void DescribeOrderedValues(TValue const& value, TBound const& bound, std::string& valueText, std::string& boundText,
    TValueKindInteger /*kind*/)
{
    valueText = DescribeInteger(value);
    boundText = DescribeInteger(bound);
}

template <typename TValue, typename TBound>
void DescribeOrderedValues(TValue const& /*value*/, TBound const& /*bound*/, std::string& /*valueText*/,
    std::string& /*boundText*/, TValueKindUnsupported /*kind*/)
{
    // Unreachable: CompareOrdered() doesn't compile for these values (TValuesCannotBeOrdered).
}

// Describes 'value' and 'bound' for an ordering failure: integers as numbers, and floating-point values in their
// common type (see DescribeFloatingPointPair()).
template <typename TValue, typename TBound>
void DescribeOrderedValues(TValue const& value, TBound const& bound, std::string& valueText, std::string& boundText)
{
    typedef typename TOrderingPairKind<typename TOrderingKind<TValue>::Type, typename TOrderingKind<TBound>::Type>::Type
        TKind;

    DescribeOrderedValues(value, bound, valueText, boundText, TKind());
}


/////////////////////////////////////////////////////////////////////////////
// TextHasPart
//
// True when the string 'text' has the string 'part' where 'where' says,
// optionally ignoring the case of the ASCII letters A-Z only (see
// HasPart()). Strings of any types pair as in ValuesEqual(). A null C
// string, as either, never matches: it isn't a text, not even an empty
// one. Values that aren't both strings don't compile.
/////////////////////////////////////////////////////////////////////////////

template <typename TText, typename TPart>
bool TextHasPart(TText const& text, TPart const& part, TTextPart::Type where, bool ignoreCase,
    TValueKindNarrowString /*kind*/)
{
    return !IsNullString(text) && !IsNullString(part) && HasPart(NarrowText(text), NarrowText(part), where,
        ignoreCase);
}

template <typename TText, typename TPart>
bool TextHasPart(TText const& text, TPart const& part, TTextPart::Type where, bool ignoreCase,
    TValueKindRTLString /*kind*/)
{
    return !IsNullString(text) && !IsNullString(part) && HasPart(WideText(UnicodeText(text)),
        WideText(UnicodeText(part)), where, ignoreCase);
}

template <typename TText, typename TPart>
bool TextHasPart(TText const& text, TPart const& part, TTextPart::Type where, bool ignoreCase,
    TValueKindWideString /*kind*/)
{
    return !IsNullString(text) && !IsNullString(part) && HasPart(WideText(text), WideText(part), where, ignoreCase);
}

template <typename TText, typename TPart>
bool TextHasPart(TText const& /*text*/, TPart const& /*part*/, TTextPart::Type /*where*/, bool /*ignoreCase*/,
    TValueKindUnsupported /*kind*/)
{
    return sizeof(TValuesAreNotTexts<TText, TPart>) != 0;
}

template <typename TText, typename TPart>
bool TextHasPart(TText const& text, TPart const& part, TTextPart::Type where, bool ignoreCase)
{
    typedef typename TValuePairKind<typename TValueKind<TText>::Type, typename TValueKind<TPart>::Type>::Type
        TPairKind;
    typedef typename TTextPairKind<TPairKind>::Type TKind;

    return TextHasPart(text, part, where, ignoreCase, TKind());
}


/////////////////////////////////////////////////////////////////////////////
// TextMatchFailure
//
// Why the string 'text' fails a Matches check of the regular expression
// 'pattern' (with 'expectMatch') or a NotMatches check, or "" when it
// passes (see MatchesRegularExpression()). Strings of any types pair as in
// ValuesEqual(). An invalid pattern always fails, saying why. A null C
// string, as either, never matches. Values that aren't both strings don't
// compile.
/////////////////////////////////////////////////////////////////////////////

template <typename TText, typename TPattern>
bool TextMatches(TText const& text, TPattern const& pattern, bool& valid, std::string& error,
    TValueKindNarrowString /*kind*/)
{
    return !IsNullString(text) && !IsNullString(pattern) && MatchesRegularExpression(UnicodeText(text),
        UnicodeText(pattern), valid, error);
}

template <typename TText, typename TPattern>
bool TextMatches(TText const& text, TPattern const& pattern, bool& valid, std::string& error,
    TValueKindRTLString /*kind*/)
{
    return !IsNullString(text) && !IsNullString(pattern) && MatchesRegularExpression(UnicodeText(text),
        UnicodeText(pattern), valid, error);
}

template <typename TText, typename TPattern>
bool TextMatches(TText const& /*text*/, TPattern const& /*pattern*/, bool& /*valid*/, std::string& /*error*/,
    TValueKindUnsupported /*kind*/)
{
    return sizeof(TValuesAreNotTexts<TText, TPattern>) == 0;
}

template <typename TText, typename TPattern>
bool TextMatches(TText const& text, TPattern const& pattern, bool& valid, std::string& error,
    TValueKindWideString /*kind*/)
{
    return !IsNullString(text) && !IsNullString(pattern) && MatchesRegularExpression(UnicodeText(text),
        UnicodeText(pattern), valid, error);
}

template <typename TText, typename TPattern>
std::string TextMatchFailure(TText const& text, TPattern const& pattern, bool expectMatch)
{
    typedef typename TValuePairKind<typename TValueKind<TText>::Type, typename TValueKind<TPattern>::Type>::Type
        TPairKind;
    typedef typename TTextPairKind<TPairKind>::Type TKind;

    bool valid = true;
    std::string error;
    bool const matches = TextMatches(text, pattern, valid, error, TKind());

    if (!valid)
        return "Invalid regular expression \"" + DescribeValue(pattern) + "\": " + error;

    if (matches == expectMatch)
        return std::string();

    return "Expected \"" + DescribeValue(text) + "\" " + (expectMatch ? "to match" : "not to match") + " \"" +
        DescribeValue(pattern) + "\"";
}


/////////////////////////////////////////////////////////////////////////////
// TextsEqualIgnoringASCIICase
//
// True when two strings are the same text, ignoring the case of the ASCII
// letters A-Z only (see EqualsIgnoringASCIICase()). Strings of any types
// pair as in ValuesEqual(), and a null C string equals only another null
// one. Values that aren't both strings don't compile.
/////////////////////////////////////////////////////////////////////////////

template <typename TExpected, typename TActual>
bool TextsEqualIgnoringASCIICase(TExpected const& expected, TActual const& actual, TValueKindNarrowString /*kind*/)
{
    return (IsNullString(expected) == IsNullString(actual)) &&
        EqualsIgnoringASCIICase(NarrowText(expected), NarrowText(actual));
}

template <typename TExpected, typename TActual>
bool TextsEqualIgnoringASCIICase(TExpected const& expected, TActual const& actual, TValueKindRTLString /*kind*/)
{
    return (IsNullString(expected) == IsNullString(actual)) &&
        EqualsIgnoringASCIICase(UnicodeText(expected), UnicodeText(actual));
}

template <typename TExpected, typename TActual>
bool TextsEqualIgnoringASCIICase(TExpected const& expected, TActual const& actual, TValueKindWideString /*kind*/)
{
    return (IsNullString(expected) == IsNullString(actual)) &&
        EqualsIgnoringASCIICase(WideText(expected), WideText(actual));
}

template <typename TExpected, typename TActual>
bool TextsEqualIgnoringASCIICase(TExpected const& /*expected*/, TActual const& /*actual*/,
    TValueKindUnsupported /*kind*/)
{
    return sizeof(TValuesAreNotTexts<TExpected, TActual>) != 0;
}

template <typename TExpected, typename TActual>
bool TextsEqualIgnoringASCIICase(TExpected const& expected, TActual const& actual)
{
    typedef typename TValuePairKind<typename TValueKind<TExpected>::Type, typename TValueKind<TActual>::Type>::Type
        TPairKind;
    typedef typename TTextPairKind<TPairKind>::Type TKind;

    return TextsEqualIgnoringASCIICase(expected, actual, TKind());
}

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_ValuesH
