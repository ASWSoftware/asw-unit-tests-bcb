/* **************************************************************************
ASWUnitTestsBCB_CrashGuard.h
Author: Anthony S. West - ASW Software

Native-crash protection for a test, for --catch-crashes.

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

#ifndef ASWUnitTestsBCB_CrashGuardH
#define ASWUnitTestsBCB_CrashGuardH
//---------------------------------------------------------------------------
#include <string>
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TCrashGuardResult
//
// What TCrashGuard::Run() found. 'Crashed' is false if the body ended
// normally, an exception included, which leaves Run() unaffected; only a
// fault Run() catches sets it. 'ShouldAbortRun' is set for a fault after
// which running more tests in this process isn't safe.
/////////////////////////////////////////////////////////////////////////////
struct TCrashGuardResult
{
    bool Crashed;
    bool ShouldAbortRun;
    std::string Description; // The fault, e.g. "stack overflow"

    TCrashGuardResult();
};


/////////////////////////////////////////////////////////////////////////////
// TCrashGuard
//
// Runs a closure, catching the native faults that would otherwise end the
// process, for TTestGroupBase::RunCatchingCrashes(). See Run() for which
// faults those are, and the caveats.
/////////////////////////////////////////////////////////////////////////////
class TCrashGuard
{
public:
    typedef void __fastcall (__closure* TBody)();

    // Calls 'body' and returns a result with Crashed set if it caused a stack overflow or an illegal instruction on
    // this thread.
    static TCrashGuardResult Run(TBody body);
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_CrashGuardH
