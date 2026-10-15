/* **************************************************************************
ASWExample_WordCounter.cpp
Author: Anthony S. West - ASW Software

Example code under test: counts the words in a text.

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
#include "ASWExample_WordCounter.h"
//---------------------------------------------------------------------------
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace ASWExample
{

/////////////////////////////////////////////////////////////////////////////
// TWordCounter
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TWordCounter::TWordCounter()
    : m_Limit(0)
{
}
//---------------------------------------------------------------------------
int TWordCounter::Count() const
{
    int count = 0;
    bool inWord = false;

    // A UnicodeString's characters are numbered from 1.
    for (int i = 1; i <= m_Text.Length(); ++i)
    {
        WideChar const character = m_Text[i];
        bool const separator = (character == L' ') || (character == L'\t') || (character == L'\r') ||
            (character == L'\n');

        if (!separator && !inWord)
            ++count;

        inWord = !separator;
    }

    return ((m_Limit != 0) && (count > m_Limit)) ? m_Limit : count;
}
//---------------------------------------------------------------------------
int TWordCounter::Limit() const
{
    return m_Limit;
}
//---------------------------------------------------------------------------
void TWordCounter::SetLimit(UnicodeString const& text)
{
    int const limit = StrToInt(text);

    if (limit < 0)
        throw ERangeError(UnicodeString(L"The word limit can't be negative: ") + text);

    m_Limit = limit;
}
//---------------------------------------------------------------------------
void TWordCounter::SetText(UnicodeString const& text)
{
    m_Text = text;
}
//---------------------------------------------------------------------------
UnicodeString TWordCounter::Text() const
{
    return m_Text;
}
//---------------------------------------------------------------------------

} // namespace ASWExample
