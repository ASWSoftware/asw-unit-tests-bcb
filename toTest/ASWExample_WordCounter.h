/* **************************************************************************
ASWExample_WordCounter.h
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

#ifndef ASWExample_WordCounterH
#define ASWExample_WordCounterH
//---------------------------------------------------------------------------
#include <System.hpp>
//---------------------------------------------------------------------------

namespace ASWExample
{

/////////////////////////////////////////////////////////////////////////////
// TWordCounter
//
// Counts the words in a text: the runs of characters between spaces, tabs
// and line breaks.
/////////////////////////////////////////////////////////////////////////////
class TWordCounter
{
private:
    System::UnicodeString m_Text;
    int m_Limit; // The most words Count() reports; 0 for no limit

public:
    TWordCounter();

    // The number of words in Text(), up to Limit() when that isn't 0.
    int Count() const;
    int Limit() const;
    // Sets Limit() from its decimal text, e.g. "10". Raises EConvertError when 'text' isn't a whole number, and
    // ERangeError when it's negative.
    void SetLimit(System::UnicodeString const& text);
    void SetText(System::UnicodeString const& text);
    System::UnicodeString Text() const;
};

} // namespace ASWExample

//---------------------------------------------------------------------------
#endif // #ifndef ASWExample_WordCounterH
