/* **************************************************************************
ASWUnitTestsBCB_Utils.h
Author: Anthony S. West - ASW Software

Small helpers shared by the framework's units.

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

#ifndef ASWUnitTestsBCB_UtilsH
#define ASWUnitTestsBCB_UtilsH
//---------------------------------------------------------------------------
#include <cstddef>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.hpp>
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TMersenneTwister
//
// The 32-bit Mersenne Twister, giving the same numbers as std::mt19937
// (which the C++ standard fully specifies), for every compiler: bcc64x has
// no std::tr1, and bcc32's std::tr1::mt19937 draws warnings from <random>.
/////////////////////////////////////////////////////////////////////////////
class TMersenneTwister
{
private:
    static std::size_t const StateSize = 624;

    unsigned int m_State[StateSize];
    std::size_t m_Index;

    void Twist();

public:
    explicit TMersenneTwister(unsigned int seed);

    // Returns the next number, from 0 to 4294967295.
    unsigned int Next();
};


// The value a TOptional<T> without one holds: T(), or zero for an enum, since bcc32 warns (W8006) about
// initializing an enum with T().
template <typename T, bool IsEnum>
struct TOptionalEmptyValue
{
    static T Get()
    {
        return T();
    }
};

template <typename T>
struct TOptionalEmptyValue<T, true>
{
    static T Get()
    {
        return static_cast<T>(0);
    }
};


/////////////////////////////////////////////////////////////////////////////
// TOptional
//
// A value that may be missing, in place of C++17's std::optional: e.g. a
// --shuffle-seed that wasn't given.
/////////////////////////////////////////////////////////////////////////////
template <typename T>
class TOptional
{
private:
    static const bool IsEnum = __is_enum(T);

    bool m_HasValue;
    T m_Value;

public:
    // Without a value.
    TOptional()
        : m_HasValue(false),
          m_Value(TOptionalEmptyValue<T, IsEnum>::Get())
    {
    }

    // With 'value'. Not explicit, so a T can be passed where a TOptional<T> is expected, as with std::optional.
    TOptional(T const& value)
        : m_HasValue(true),
          m_Value(value)
    {
    }

    bool HasValue() const
    {
        return m_HasValue;
    }

    // The value; T() (zero for an enum) without one.
    T const& Value() const
    {
        return m_Value;
    }
};

//---------------------------------------------------------------------------

// Returns 'method' without any class or namespace qualification, e.g. "Test_Add" for "TTest_TMyClass::Test_Add".
// bcc32's __FUNC__ names the class too, while bcc32c's and bcc64x's give the bare name, so failure messages pass
// the method through this to read the same with every compiler. Template arguments (after the first '<') are
// kept, and a "::" inside them doesn't count.
std::string BareMethodName(std::string const& method);

// Returns the 32-bit FNV-1a hash of 'text''s bytes, the same with every compiler.
unsigned int FNV1aHash(std::string const& text);

// Returns a test's duration for the log, with three decimals, e.g. "0.713 ms".
std::string FormatDurationMs(double milliseconds);

// Returns a run's elapsed time for the log, e.g. "00h:01m:02s.345ms".
std::string FormatElapsedTime(__int64 milliseconds);

// Returns "Method (Line)", the location part of a failure message, with the bare method name (see BareMethodName()).
std::string FormatLocation(std::string const& method, int line);

// Returns UTF-8 'text' as a UnicodeString, the RTL's and the VCL's string type; the reverse of ToUTF8().
System::UnicodeString FromUTF8(std::string const& text);

// Returns the current UTC time in ISO 8601 form with milliseconds, e.g. "2026-10-11T22:45:12.224Z".
std::string GetUTCTimeISO8601();

// Returns 'value' in decimal, e.g. "-42" (C++03 has no std::to_string).
std::string IntToString(int value);

// Returns a seed for a shuffled run that wasn't given one, different from one call (and one run) to the next, in place
// of asw-unit-tests' std::random_device, which bcc32 lacks.
unsigned int NewShuffleSeed();

// Returns the indices 0 to 'count' - 1 in a random order determined entirely by 'seed'. A seed gives the same order
// with every compiler, and the same as asw-unit-tests' ShuffledIndices() gives, so a --shuffle-seed reproduces a
// run's order with either framework.
std::vector<std::size_t> ShuffledIndices(std::size_t count, unsigned int seed);

// Returns 'text' as UTF-8, the framework's internal encoding.
std::string ToUTF8(System::UnicodeString const& text);

// Returns 'value' in decimal.
std::string UIntToString(unsigned int value);

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_UtilsH
