/* **************************************************************************
ASWUnitTestsBCB_Utils.cpp
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

#include <System.hpp>
#pragma hdrstop

//---------------------------------------------------------------------------
// Module header
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
#include <algorithm>
#include <iomanip>
#include <locale>
#include <sstream>
//---------------------------------------------------------------------------
#include <windows.h>
//---------------------------------------------------------------------------
using namespace System;
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TMersenneTwister
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TMersenneTwister::TMersenneTwister(unsigned int seed)
    : m_Index(StateSize)
{
    m_State[0] = seed;

    for (std::size_t i = 1; i < StateSize; ++i)
        m_State[i] = 1812433253u * (m_State[i - 1] ^ (m_State[i - 1] >> 30)) + static_cast<unsigned int>(i);
}
//---------------------------------------------------------------------------
unsigned int TMersenneTwister::Next()
{
    if (m_Index >= StateSize)
        Twist();

    unsigned int value = m_State[m_Index++];

    value ^= value >> 11;
    value ^= (value << 7) & 0x9D2C5680u;
    value ^= (value << 15) & 0xEFC60000u;
    value ^= value >> 18;
    return value;
}
//---------------------------------------------------------------------------
void TMersenneTwister::Twist()
{
    for (std::size_t i = 0; i < StateSize; ++i)
    {
        unsigned int const bits = (m_State[i] & 0x80000000u) | (m_State[(i + 1) % StateSize] & 0x7FFFFFFFu);

        m_State[i] = m_State[(i + 397) % StateSize] ^ (bits >> 1) ^ (((bits & 1u) != 0) ? 0x9908B0DFu : 0u);
    }

    m_Index = 0;
}
//---------------------------------------------------------------------------


// /////// Functions ///////////////////////

//---------------------------------------------------------------------------
std::string BareMethodName(std::string const& method)
{
    std::string::size_type const templateStart = method.find('<');
    std::string const qualifiedName = method.substr(0, templateStart);
    std::string::size_type const lastColons = qualifiedName.rfind("::");

    if (lastColons == std::string::npos)
        return method;

    return method.substr(lastColons + 2);
}
//---------------------------------------------------------------------------
unsigned int FNV1aHash(std::string const& text)
{
    unsigned int hash = 0x811C9DC5u; // FNV offset basis

    for (std::string::size_type i = 0; i < text.size(); ++i)
    {
        hash ^= static_cast<unsigned char>(text[i]);
        hash *= 0x01000193u; // FNV prime
    }

    return hash;
}
//---------------------------------------------------------------------------
std::string FormatDurationMs(double milliseconds)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(3) << milliseconds << " ms";
    return stream.str();
}
//---------------------------------------------------------------------------
std::string FormatElapsedTime(__int64 milliseconds)
{
    __int64 const hours = milliseconds / (1000 * 60 * 60);
    milliseconds %= (1000 * 60 * 60);
    __int64 const minutes = milliseconds / (1000 * 60);
    milliseconds %= (1000 * 60);
    __int64 const seconds = milliseconds / 1000;
    milliseconds %= 1000;

    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(2) << hours << "h:" << std::setw(2) << minutes << "m:" << std::setw(2)
    << seconds << "s." << std::setw(3) << milliseconds << "ms";
    return stream.str();
}
//---------------------------------------------------------------------------
std::string FormatLocation(std::string const& method, int line)
{
    return BareMethodName(method) + " (" + IntToString(line) + ")";
}
//---------------------------------------------------------------------------
UnicodeString FromUTF8(std::string const& text)
{
    return UnicodeString(UTF8String(text.c_str(), static_cast<int>(text.size())));
}
//---------------------------------------------------------------------------
std::string GetUTCTimeISO8601()
{
    SYSTEMTIME utc;
    ::GetSystemTime(&utc);

    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(4) << utc.wYear << '-' << std::setw(2) << utc.wMonth << '-'
    << std::setw(2) << utc.wDay << 'T' << std::setw(2) << utc.wHour << ':' << std::setw(2) << utc.wMinute
    << ':' << std::setw(2) << utc.wSecond << '.' << std::setw(3) << utc.wMilliseconds << 'Z';
    return stream.str();
}
//---------------------------------------------------------------------------
std::string IntToString(int value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}
//---------------------------------------------------------------------------
unsigned int NewShuffleSeed()
{
    LARGE_INTEGER counter;
    ::QueryPerformanceCounter(&counter);

    unsigned __int64 const ticks = static_cast<unsigned __int64>(counter.QuadPart);
    unsigned int const processId = static_cast<unsigned int>(::GetCurrentProcessId());

    return static_cast<unsigned int>(ticks) ^ static_cast<unsigned int>(ticks >> 32) ^ (processId * 2654435761u) ^
        static_cast<unsigned int>(::GetTickCount());
}
//---------------------------------------------------------------------------
/*
    ShuffledIndices

    A Fisher-Yates shuffle driven by the Mersenne Twister (see TMersenneTwister), drawing each position by rejection
    sampling, which keeps it unbiased, exactly as asw-unit-tests does it. (std::shuffle and
    std::uniform_int_distribution aren't used there, since how they turn the generator's output into positions is
    left to each standard library.)
*/
std::vector<std::size_t> ShuffledIndices(std::size_t count, unsigned int seed)
{
    std::vector<std::size_t> indices(count);

    for (std::size_t i = 0; i < count; ++i)
        indices[i] = i;

    TMersenneTwister generator(seed);

    for (std::size_t i = count; i > 1; --i)
    {
        // Draws a position in [0, i). A draw below 2^32 mod i is rejected, so each position is equally likely.
        unsigned int const bound = static_cast<unsigned int>(i);
        unsigned int const rejectBelow = (0u - bound) % bound;
        unsigned int draw = generator.Next();

        while (draw < rejectBelow)
            draw = generator.Next();

        std::swap(indices[i - 1], indices[draw % bound]);
    }

    return indices;
}
//---------------------------------------------------------------------------
std::string ToUTF8(UnicodeString const& text)
{
    UTF8String const utf8(text);
    return std::string(utf8.c_str(), static_cast<std::string::size_type>(utf8.Length()));
}
//---------------------------------------------------------------------------
std::string UIntToString(unsigned int value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
