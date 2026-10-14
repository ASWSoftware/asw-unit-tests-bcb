/* **************************************************************************
ASWUnitTestsBCB_StdOutRedirect.h
Author: Anthony S. West - ASW Software

Captures what's written to std::cout, for tests of code that writes to the
console.

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

#ifndef ASWUnitTestsBCB_StdOutRedirectH
#define ASWUnitTestsBCB_StdOutRedirectH
//---------------------------------------------------------------------------
#include <sstream>
#include <streambuf>
#include <string>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TStdOutRedirect
//
// Redirects std::cout into an in-memory buffer, for testing code that
// writes to the console (including this framework's own log, when no
// ITestRunObserver is set). Starts capturing on construction. Restore()
// hands std::cout back before this object is destroyed (its destructor
// calls it if that hasn't happened), and Capture() starts a fresh capture
// again afterwards, discarding what the previous one collected. Both are
// safe to call in the state they'd produce (e.g. Restore() while not
// capturing does nothing), so one object can be switched back and forth as
// often as a test needs. Only std::cout is captured: printf() and other C
// stdio output, and std::wcout, still reach the console.
/////////////////////////////////////////////////////////////////////////////
class TStdOutRedirect
{
private:
    std::ostringstream m_Captured;
    std::streambuf* m_OriginalBuffer; // Not nullptr while capturing

    TStdOutRedirect(TStdOutRedirect const&);            // Not copyable
    TStdOutRedirect& operator=(TStdOutRedirect const&); // Not copyable

public:
    TStdOutRedirect();
    ~TStdOutRedirect();

    // Redirects std::cout into this object's buffer, discarding what a previous Capture()/Restore() cycle collected.
    // Does nothing if already capturing.
    void Capture();

    // Restores std::cout to the stream buffer it had before capturing began. Does nothing if not capturing.
    void Restore();

    // Everything written to std::cout during the current capture, or, after Restore(), the capture that just ended.
    std::string Str() const;
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_StdOutRedirectH
