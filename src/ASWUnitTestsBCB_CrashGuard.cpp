/* **************************************************************************
ASWUnitTestsBCB_CrashGuard.cpp
Author: Anthony S. West - ASW Software

See header for info.

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
#include "ASWUnitTestsBCB_CrashGuard.h"
//---------------------------------------------------------------------------
#include <csetjmp>
//---------------------------------------------------------------------------
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------
using namespace System;
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TGuardFrame
//
// The state of one TCrashGuard::Run() call, which CrashHandler() jumps back
// to.
/////////////////////////////////////////////////////////////////////////////
struct TGuardFrame
{
    std::jmp_buf JumpBuffer;
    DWORD FaultCode;
};

/////////////////////////////////////////////////////////////////////////////
// TGuardFrameScope
//
// Makes a frame the current thread's innermost guard while it exists, and
// puts back the one before it when it ends, also when an exception leaves
// TCrashGuard::Run().
/////////////////////////////////////////////////////////////////////////////
class TGuardFrameScope
{
private:
    void* const m_Previous;

    TGuardFrameScope(TGuardFrameScope const&);            // Not copyable
    TGuardFrameScope& operator=(TGuardFrameScope const&); // Not copyable

public:
    explicit TGuardFrameScope(TGuardFrame* frame);
    ~TGuardFrameScope();
};

// The thread-local slot that holds the current thread's innermost TGuardFrame, or nullptr outside TCrashGuard::Run().
DWORD g_FrameSlot = TLS_OUT_OF_INDEXES;

// Calls 'body', returning false, unless CrashHandler() jumps back to 'frame', which returns true.
bool CallJumpingBack(ASWUnitTestsBCB::TCrashGuard::TBody body, TGuardFrame& frame);
LONG WINAPI CrashHandler(PEXCEPTION_POINTERS info);
std::string DescribeFault(DWORD code);
bool InstallCrashHandler();

//---------------------------------------------------------------------------
TGuardFrameScope::TGuardFrameScope(TGuardFrame* frame)
    : m_Previous(::TlsGetValue(g_FrameSlot))
{
    ::TlsSetValue(g_FrameSlot, frame);
}
//---------------------------------------------------------------------------
TGuardFrameScope::~TGuardFrameScope()
{
    ::TlsSetValue(g_FrameSlot, m_Previous);
}
//---------------------------------------------------------------------------
/*
    CallJumpingBack

    Keeps setjmp() in a function with nothing to destroy, so the jump back never skips an object of its own.

    On Win32, each function with a try block links a record into the thread's chain of exception handlers (the head
    is in the thread information block), and the functions the jump abandons, TTestGroupBase::Test() for one, leave
    theirs there. bcc32's longjmp() puts the head back; bcc32c's doesn't, and the next exception walked into the
    abandoned stack and ended the process. So with bcc32c the head saved here is put back after the jump. (Win64
    keeps its handlers in tables, not on the stack.)
*/
bool CallJumpingBack(ASWUnitTestsBCB::TCrashGuard::TBody body, TGuardFrame& frame)
{
#if defined(__BORLANDC__) && defined(__clang__) && !defined(_WIN64)
    NT_TIB* const threadInfo = reinterpret_cast<NT_TIB*>(::NtCurrentTeb());
    EXCEPTION_REGISTRATION_RECORD* const handlers = threadInfo->ExceptionList;
#endif

    if (setjmp(frame.JumpBuffer) != 0)
    {
#if defined(__BORLANDC__) && defined(__clang__) && !defined(_WIN64)
        threadInfo->ExceptionList = handlers;
#endif
        return true;
    }

    body();
    return false;
}
//---------------------------------------------------------------------------
/*
    CrashHandler

    A vectored exception handler, so it sees a fault before any handler on the stack does, and before the RTL turns
    it into an exception. It takes only the faults that ended the process (checked 2026-10-12, RAD Studio 13.1): a
    stack overflow, which bcc64x and bcc32c couldn't turn into EStackOverflow with the stack used up, and which none
    survives twice, since the stack's guard page is gone; and an illegal instruction, which bcc32 couldn't handle.
    Everything else, an access violation included, the RTL turns into an exception (EAccessViolation) that the test
    can expect and that fails it otherwise, so it's left alone. On a thread inside TCrashGuard::Run(), it jumps back
    there: nothing on the stack in between, destructors included, runs.
*/
LONG WINAPI CrashHandler(PEXCEPTION_POINTERS info)
{
    DWORD const code = info->ExceptionRecord->ExceptionCode;

    if ((code != EXCEPTION_STACK_OVERFLOW) && (code != EXCEPTION_ILLEGAL_INSTRUCTION))
        return EXCEPTION_CONTINUE_SEARCH;

    TGuardFrame* const frame = static_cast<TGuardFrame*>(::TlsGetValue(g_FrameSlot));

    // The jump is inside the if, so that the return follows it for bcc64x (whose longjmp() isn't declared to never
    // return), yet stays reachable for bcc32 (W8066 "Unreachable code").
    if (frame != nullptr)
    {
        frame->FaultCode = code;
        std::longjmp(frame->JumpBuffer, 1);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
//---------------------------------------------------------------------------
std::string DescribeFault(DWORD code)
{
    return (code == EXCEPTION_STACK_OVERFLOW) ? "stack overflow" : "illegal instruction";
}
//---------------------------------------------------------------------------
// Installs CrashHandler() the first time, for the life of the process, and returns whether it's installed. Tests
// run one at a time, so this is never called by two threads at once.
bool InstallCrashHandler()
{
    if (g_FrameSlot != TLS_OUT_OF_INDEXES)
        return true;

    DWORD const slot = ::TlsAlloc();

    if (slot == TLS_OUT_OF_INDEXES)
        return false;

    g_FrameSlot = slot;
    ::AddVectoredExceptionHandler(1, CrashHandler);
    return true;
}
//---------------------------------------------------------------------------

} // namespace

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TCrashGuardResult
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TCrashGuardResult::TCrashGuardResult()
    : Crashed(false),
      ShouldAbortRun(false)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TCrashGuard
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
/*
    TCrashGuard::Run

    Calls 'body' and returns a default result if it ends normally, an exception included, which passes through. If it
    causes a stack overflow or an illegal instruction on this thread, CrashHandler() jumps back here, and the result
    describes the fault, with ShouldAbortRun set for a stack overflow. Any other fault is left to the RTL, which turns
    it into an exception (see CrashHandler()).

    The jump abandons 'body' where it faulted: no destructor of an object on the stack between here and there runs,
    and no handler there sees the fault (even one in the code under test, e.g. one that tries an instruction the CPU
    may lack). After a stack overflow, the thread's stack has lost its guard page, so a second one would end the
    process however it's handled.

    SetThreadStackGuarantee() keeps room at the end of the stack for the handlers to run in once it overflows: without
    it, a bcc64x build ended with an access violation (0xC0000005) instead of jumping back (bcc32 and bcc32c didn't
    need it). It's set each time, since it belongs to the thread, and a test runs on a worker thread under a timeout.

    Guards nest: an inner Run() on the same thread takes the faults while it lasts. Setjmp() and longjmp() were
    checked on all three compilers, optimized bcc64x included, with C++ and RTL exceptions and access violations still
    caught as usual afterwards (2026-10-12), once CallJumpingBack() also put Win32's chain of exception handlers back.
*/
TCrashGuardResult TCrashGuard::Run(TBody body)
{
    TCrashGuardResult result;

    if (!InstallCrashHandler())
    {
        body();
        return result;
    }

    ULONG stackGuarantee = 64 * 1024;
    ::SetThreadStackGuarantee(&stackGuarantee);

    TGuardFrame frame;
    frame.FaultCode = 0;
    TGuardFrameScope const scope(&frame);

    if (CallJumpingBack(body, frame))
    {
        result.Crashed = true;
        result.ShouldAbortRun = (frame.FaultCode == EXCEPTION_STACK_OVERFLOW);
        result.Description = DescribeFault(frame.FaultCode);
    }

    return result;
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
