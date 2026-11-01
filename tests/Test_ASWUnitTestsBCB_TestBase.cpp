/* **************************************************************************
Test_ASWUnitTestsBCB_TestBase.cpp
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_TestBase.h: discovery, the lifecycle, outcomes, exceptions and assertions, checked by
running small unregistered groups and reading their results.

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
#include "Test_ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>
#if defined(_WIN64)
#  include <cfenv> // RoundingMode(), SetRoundingMode()
#else
#  include <float.h> // _control87(), for RoundingMode() and SetRoundingMode()
#endif
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <System.Diagnostics.hpp>
#include <System.SysUtils.hpp>
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Classes;
using namespace System::Diagnostics;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TGroupRunThread
//
// Runs a group with the crash guard and no timeout, on a thread of its own.
/////////////////////////////////////////////////////////////////////////////
class TGroupRunThread : public TThread
{
private:
    ASWUnitTestsBCB::TTestGroupBase& m_Group;

protected:
    virtual void __fastcall Execute()
    {
        m_Group.Run(nullptr, ASWUnitTestsBCB::TOptional<unsigned int>(), ASWUnitTestsBCB::TOptional<unsigned int>(),
            true);
    }

public:
    explicit __fastcall TGroupRunThread(ASWUnitTestsBCB::TTestGroupBase& group)
        : TThread(true),
          m_Group(group)
    {
    }
};


/////////////////////////////////////////////////////////////////////////////
// TRecordingObserver
//
// Records what a group tells its observer, which also keeps the inner
// groups' output off the console.
/////////////////////////////////////////////////////////////////////////////
class TRecordingObserver : public ASWUnitTestsBCB::ITestRunObserver
{
public:
    DWORD const CreatingThread;
    bool EventFromOtherThread; // Set when a call other than OnLog() comes from another thread than CreatingThread
    std::string Events;        // "S:Group.Test;" when a test starts, "F:Group.Test=pass;" when it finishes
    std::string LogText;
    int StopAfterStarted;      // Stop once this many tests have started; -1 never
    int Started;

public:
    TRecordingObserver()
        : CreatingThread(::GetCurrentThreadId()),
          EventFromOtherThread(false),
          StopAfterStarted(-1),
          Started(0)
    {
    }

    virtual void OnLog(std::string const& text)
    {
        LogText += text;
    }

    virtual void OnTestFinished(ASWUnitTestsBCB::TTestCaseRecord const& record)
    {
        char const* const outcome = (record.Outcome == ASWUnitTestsBCB::TTestOutcome::Pass) ? "pass" :
                (record.Outcome == ASWUnitTestsBCB::TTestOutcome::Fail) ? "fail" : "skip";
        Events += "F:" + record.GroupName + "." + record.TestName + "=" + outcome + ";";
        EventFromOtherThread = EventFromOtherThread || (::GetCurrentThreadId() != CreatingThread);
    }

    virtual void OnTestStarted(std::string const& groupName, std::string const& testName)
    {
        ++Started;
        Events += "S:" + groupName + "." + testName + ";";
        EventFromOtherThread = EventFromOtherThread || (::GetCurrentThreadId() != CreatingThread);
    }

    virtual bool StopRequested()
    {
        EventFromOtherThread = EventFromOtherThread || (::GetCurrentThreadId() != CreatingThread);
        return (StopAfterStarted >= 0) && (Started >= StopAfterStarted);
    }
};

//---------------------------------------------------------------------------

void* g_IllegalInstruction = nullptr; // Executable memory holding ud2, once RunIllegalInstruction() has run
bool volatile g_KeepRecursing = true;

//---------------------------------------------------------------------------

std::string Address(void const* pointer);
bool Contains(std::string const& text, std::string const& part);
int DownwardRoundingMode();
std::string NamesOf(std::vector<ASWUnitTestsBCB::TPublishedTest> const& tests);
std::string OutcomeOf(ASWUnitTestsBCB::TTestResults const& results, std::string const& testName);
std::string MessageOf(ASWUnitTestsBCB::TTestResults const& results, std::string const& testName);
std::string RecordsOf(ASWUnitTestsBCB::TTestResults const& results);
int Recurse(int depth);
int RoundingMode();
void RunIllegalInstruction();
void SetRoundingMode(int mode);
bool StartsWith(std::string const& text, std::string const& prefix);

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
bool Contains(std::string const& text, std::string const& part)
{
    return text.find(part) != std::string::npos;
}
//---------------------------------------------------------------------------
/*
    DownwardRoundingMode

    The rounding mode that rounds toward negative infinity, as RoundingMode() gives it.
*/
int DownwardRoundingMode()
{
#if defined(_WIN64)
    return FE_DOWNWARD;
#else
    return RC_DOWN;
#endif
}
//---------------------------------------------------------------------------
/*
    MessageOf

    The record of 'testName' in 'results', or "(no record)".
*/
std::string MessageOf(ASWUnitTestsBCB::TTestResults const& results, std::string const& testName)
{
    for (std::size_t i = 0; i < results.CaseRecords.size(); ++i)
    {
        if (results.CaseRecords[i].TestName == testName)
            return results.CaseRecords[i].Message;
    }

    return "(no record)";
}
//---------------------------------------------------------------------------
/*
    NamesOf

    The test names, comma-separated, e.g. "Zeta,Virtual,Alpha".
*/
std::string NamesOf(std::vector<ASWUnitTestsBCB::TPublishedTest> const& tests)
{
    std::string names;

    for (std::size_t i = 0; i < tests.size(); ++i)
        names += ((i == 0) ? std::string() : std::string(",")) + tests[i].TestName;

    return names;
}
//---------------------------------------------------------------------------
/*
    OutcomeOf

    "pass", "fail" or "skip" for 'testName' in 'results', or "(no record)".
*/
std::string OutcomeOf(ASWUnitTestsBCB::TTestResults const& results, std::string const& testName)
{
    for (std::size_t i = 0; i < results.CaseRecords.size(); ++i)
    {
        if (results.CaseRecords[i].TestName != testName)
            continue;

        switch (results.CaseRecords[i].Outcome)
        {
            case ASWUnitTestsBCB::TTestOutcome::Pass:
                return "pass";

            case ASWUnitTestsBCB::TTestOutcome::Fail:
                return "fail";

            case ASWUnitTestsBCB::TTestOutcome::Skip:
                return "skip";
        }
    }

    return "(no record)";
}
//---------------------------------------------------------------------------
/*
    RecordsOf

    Each record's test name, outcome and message, one per line, e.g. "Fail=fail: Fail (12): failing on purpose".
*/
std::string RecordsOf(ASWUnitTestsBCB::TTestResults const& results)
{
    std::string records;

    for (std::size_t i = 0; i < results.CaseRecords.size(); ++i)
    {
        std::string const& testName = results.CaseRecords[i].TestName;
        records += testName + "=" + OutcomeOf(results, testName) + ": " + results.CaseRecords[i].Message + "\n";
    }

    return records;
}
//---------------------------------------------------------------------------
/*
    Recurse

    Recurses until the stack overflows, a kilobyte at a time. (g_KeepRecursing only keeps compilers from warning about
    endless recursion.)
*/
int Recurse(int depth)
{
    char volatile buffer[1024];
    buffer[0] = static_cast<char>(depth);
    return (g_KeepRecursing ? Recurse(depth + 1) : 0) + buffer[0];
}
//---------------------------------------------------------------------------
/*
    RoundingMode

    This thread's floating-point rounding mode: std::fegetround() on Win64, and on Win32 the x87 control word's
    rounding bits, since bcc32 has no <cfenv> and bcc32c's runtime declares std::fegetround() and std::fesetround()
    without defining them.
*/
int RoundingMode()
{
#if defined(_WIN64)
    return std::fegetround();
#else
    return static_cast<int>(_control87(0, 0) & MCW_RC);
#endif
}
//---------------------------------------------------------------------------
/*
    RunIllegalInstruction

    Executes an illegal instruction (ud2).
*/
void RunIllegalInstruction()
{
    typedef void (*TCode)();

    if (g_IllegalInstruction == nullptr)
    {
        unsigned char const ud2[] = { 0x0F, 0x0B, 0xC3 };
        void* const memory = ::VirtualAlloc(nullptr, sizeof(ud2), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        std::memcpy(memory, ud2, sizeof(ud2));
        g_IllegalInstruction = memory;
    }

    reinterpret_cast<TCode>(g_IllegalInstruction)();
}
//---------------------------------------------------------------------------
/*
    SetRoundingMode

    Sets this thread's rounding mode to 'mode', from RoundingMode() or DownwardRoundingMode().
*/
void SetRoundingMode(int mode)
{
#if defined(_WIN64)
    std::fesetround(mode);
#else
    _control87(static_cast<unsigned int>(mode), MCW_RC);
#endif
}
//---------------------------------------------------------------------------
bool StartsWith(std::string const& text, std::string const& prefix)
{
    return text.compare(0, prefix.size(), prefix) == 0;
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

// The inner groups the tests below run. None is registered, so the program's own run never includes them.

/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Base, TInnerTB_Derived: discovery and inheritance
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Base : public TTestGroupBase
{
public:
    std::string Trace;

    __fastcall TInnerTB_Base()
        : TTestGroupBase("InnerBase")
    {
    }

    __fastcall TInnerTB_Base(std::string const& name)
        : TTestGroupBase(name)
    {
    }

__published:
    void __fastcall Test_Zeta();
    virtual void __fastcall Test_Virtual();
    void __fastcall OnClick(TObject* sender); // Not a test: no Test_ prefix
    void __fastcall Test_();                          // Not a test: no name after the prefix
    void __fastcall Test_Alpha();
};

void __fastcall TInnerTB_Base::Test_Zeta()
{
    Trace += "Base.Zeta;";
}

void __fastcall TInnerTB_Base::Test_Virtual()
{
    Trace += "Base.Virtual;";
}

void __fastcall TInnerTB_Base::OnClick(TObject* /*sender*/)
{
    Trace += "OnClick;";
}

void __fastcall TInnerTB_Base::Test_()
{
    Trace += "Empty;";
}

void __fastcall TInnerTB_Base::Test_Alpha()
{
    Trace += "Base.Alpha;";
}

class TInnerTB_Derived : public TInnerTB_Base
{
public:
    __fastcall TInnerTB_Derived()
        : TInnerTB_Base("InnerDerived")
    {
    }

__published:
    void __fastcall Test_Own();
    virtual void __fastcall Test_Virtual(); // Overrides
    void __fastcall Test_Alpha();           // Hides
    void __fastcall Test_Beta();
};

void __fastcall TInnerTB_Derived::Test_Own()
{
    Trace += "Derived.Own;";
}

void __fastcall TInnerTB_Derived::Test_Virtual()
{
    Trace += "Derived.Virtual;";
}

void __fastcall TInnerTB_Derived::Test_Alpha()
{
    Trace += "Derived.Alpha;";
}

void __fastcall TInnerTB_Derived::Test_Beta()
{
    Trace += "Derived.Beta;";
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Duplicate, TInnerTB_Empty
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Duplicate : public TTestGroupBase
{
public:
    __fastcall TInnerTB_Duplicate()
        : TTestGroupBase("InnerDuplicate")
    {
    }

__published:
    void __fastcall Test_Same();
    void __fastcall Test_Same(int value);
};

void __fastcall TInnerTB_Duplicate::Test_Same()
{
}

void __fastcall TInnerTB_Duplicate::Test_Same(int /*value*/)
{
}

class TInnerTB_Empty : public TTestGroupBase
{
public:
    __fastcall TInnerTB_Empty()
        : TTestGroupBase("InnerEmpty")
    {
    }
};


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Lifecycle: one test of each outcome, tracing the lifecycle
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Lifecycle : public TTestGroupBase
{
public:
    std::string Trace;
    int FailLine;
    int SkipLine;

    __fastcall TInnerTB_Lifecycle()
        : TTestGroupBase("InnerLifecycle"),
          FailLine(0),
          SkipLine(0)
    {
    }

    virtual void SetUp_Group()
    {
        Trace += "SUG;";
    }

    virtual void TearDown_Group()
    {
        Trace += "TDG;";
    }

protected:
    virtual void SetUp_Test(ITestCase& testCase)
    {
        Trace += "SUT:" + testCase.GetName() + ";";
    }

    virtual void TearDown_Test(ITestCase& testCase)
    {
        Trace += "TDT:" + testCase.GetName() + ";";
    }

__published:
    void __fastcall Test_Pass();
    void __fastcall Test_Fail();
    void __fastcall Test_Skip();
};

void __fastcall TInnerTB_Lifecycle::Test_Pass()
{
    Trace += "Pass;";
}

void __fastcall TInnerTB_Lifecycle::Test_Fail()
{
    Trace += "Fail;";
    FailLine = __LINE__ + 1;
    Fail(ASWBCB_HERE, "failing on purpose");
}

void __fastcall TInnerTB_Lifecycle::Test_Skip()
{
    Trace += "Skip;";
    SkipLine = __LINE__ + 1;
    Skip(ASWBCB_HERE, "skipping on purpose");
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_ShortTest: a test far shorter than a millisecond
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_ShortTest : public TTestGroupBase
{
public:
    static double const BusyWaitSeconds;

    __fastcall TInnerTB_ShortTest()
        : TTestGroupBase("InnerShortTest")
    {
    }

__published:
    void __fastcall Test_BusyWaits();
};

double const TInnerTB_ShortTest::BusyWaitSeconds = 0.0002;

void __fastcall TInnerTB_ShortTest::Test_BusyWaits()
{
    // Busy-waits, timed with QueryPerformanceCounter(), rather than sleeping, since a sleep can last a whole
    // scheduler tick, which would hide a clock too coarse to see a short test.
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER now;
    ::QueryPerformanceFrequency(&frequency);
    ::QueryPerformanceCounter(&start);

    do
    {
        ::QueryPerformanceCounter(&now);
    } while (static_cast<double>(now.QuadPart - start.QuadPart) < (BusyWaitSeconds * frequency.QuadPart));
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Hangs: a test that never ends, between two others
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Hangs : public TTestGroupBase
{
public:
    std::string Trace;

    __fastcall TInnerTB_Hangs()
        : TTestGroupBase("InnerHangs")
    {
    }

    virtual void TearDown_Group()
    {
        Trace += "TDG;";
    }

__published:
    void __fastcall Test_A_Passes();
    void __fastcall Test_B_Hangs();
    void __fastcall Test_C_NeverRuns();
};

void __fastcall TInnerTB_Hangs::Test_A_Passes()
{
    Trace += "A;";
}

void __fastcall TInnerTB_Hangs::Test_B_Hangs()
{
    Trace += "B;";

    // For good: the abandoned thread never wakes up to touch the group, which is freed while it sleeps.
    ::Sleep(INFINITE);
}

void __fastcall TInnerTB_Hangs::Test_C_NeverRuns()
{
    Trace += "C;";
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Late: a test that ends a little after a one-second timeout,
// with a Check* failure on the way
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Late : public TTestGroupBase
{
public:
    HANDLE const TornDown; // Set by TearDown_Test(), once the test is over

    __fastcall TInnerTB_Late()
        : TTestGroupBase("InnerLate"),
          TornDown(::CreateEvent(nullptr, TRUE, FALSE, nullptr))
    {
    }

    __fastcall virtual ~TInnerTB_Late()
    {
        ::CloseHandle(TornDown);
    }

protected:
    virtual void TearDown_Test(ITestCase& /*testCase*/)
    {
        ::SetEvent(TornDown);
    }

__published:
    void __fastcall Test_Late();
};

void __fastcall TInnerTB_Late::Test_Late()
{
    ::Sleep(1300);
    CheckEquals(1, 2, ASWBCB_HERE);
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Threads: the threads a test, its TThread::Synchronize() call and
// its TThread::Queue() call run on
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Threads : public TTestGroupBase
{
public:
    DWORD QueuedOn; // 0 until it runs
    DWORD SynchronizedOn;
    DWORD TestRanOn;

    __fastcall TInnerTB_Threads()
        : TTestGroupBase("InnerThreads"),
          QueuedOn(0),
          SynchronizedOn(0),
          TestRanOn(0)
    {
    }

    void __fastcall OnQueued();
    void __fastcall OnSynchronized();

__published:
    void __fastcall Test_Records();
};

void __fastcall TInnerTB_Threads::OnQueued()
{
    QueuedOn = ::GetCurrentThreadId();
}

void __fastcall TInnerTB_Threads::OnSynchronized()
{
    SynchronizedOn = ::GetCurrentThreadId();
}

void __fastcall TInnerTB_Threads::Test_Records()
{
    TestRanOn = ::GetCurrentThreadId();
    TThread::Synchronize(nullptr, &OnSynchronized);
    TThread::Queue(nullptr, &OnQueued);
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Rounding: a test that records the rounding mode it runs with
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Rounding : public TTestGroupBase
{
public:
    int RoundingModeInTest; // -1 until it runs

    __fastcall TInnerTB_Rounding()
        : TTestGroupBase("InnerRounding"),
          RoundingModeInTest(-1)
    {
    }

__published:
    void __fastcall Test_Records();
};

void __fastcall TInnerTB_Rounding::Test_Records()
{
    RoundingModeInTest = RoundingMode();
}

// Runs a new TGroup plainly and another with 'testTimeoutSeconds' and 'catchCrashes', and returns in 'plain' and
// 'other' what each recorded (see RecordsOf()), traced and told its observer, for comparing.
template <class TGroup>
void RunPlainAndWith(TOptional<unsigned int> testTimeoutSeconds, bool catchCrashes, std::string& plain,
    std::string& other)
{
    TRecordingObserver plainObserver;
    TRecordingObserver otherObserver;
    std::unique_ptr<TGroup> plainGroup(new TGroup());
    std::unique_ptr<TGroup> otherGroup(new TGroup());
    plainGroup->SetRunObserver(&plainObserver);
    otherGroup->SetRunObserver(&otherObserver);

    plainGroup->Run();
    otherGroup->Run(nullptr, TOptional<unsigned int>(), testTimeoutSeconds, catchCrashes);

    plain = RecordsOf(plainGroup->Results()) + plainGroup->Trace + "\n" + plainObserver.Events;
    other = RecordsOf(otherGroup->Results()) + otherGroup->Trace + "\n" + otherObserver.Events;
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Crashes: an illegal instruction between two tests that pass
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Crashes : public TTestGroupBase
{
public:
    std::string Trace;

    __fastcall TInnerTB_Crashes()
        : TTestGroupBase("InnerCrashes")
    {
    }

protected:
    virtual void TearDown_Test(ITestCase& testCase)
    {
        Trace += "TDT:" + testCase.GetName() + ";";
    }

__published:
    void __fastcall Test_A_Passes();
    void __fastcall Test_B_IllegalInstruction();
    void __fastcall Test_C_Passes();
};

void __fastcall TInnerTB_Crashes::Test_A_Passes()
{
    Trace += "A;";
}

void __fastcall TInnerTB_Crashes::Test_B_IllegalInstruction()
{
    Trace += "B;";
    RunIllegalInstruction();
    Trace += "B after;";
}

void __fastcall TInnerTB_Crashes::Test_C_Passes()
{
    Trace += "C;";
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Overflows: a stack overflow, then a test that never runs
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Overflows : public TTestGroupBase
{
public:
    std::string Trace;

    __fastcall TInnerTB_Overflows()
        : TTestGroupBase("InnerOverflows")
    {
    }

    virtual void TearDown_Group()
    {
        Trace += "TDG;";
    }

__published:
    void __fastcall Test_A_Overflows();
    void __fastcall Test_B_NeverRuns();
};

void __fastcall TInnerTB_Overflows::Test_A_Overflows()
{
    Trace += "A;";
    Recurse(0);
}

void __fastcall TInnerTB_Overflows::Test_B_NeverRuns()
{
    Trace += "B;";
}


/////////////////////////////////////////////////////////////////////////////
// Groups whose lifecycle methods throw
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_SetUpGroupThrows : public TTestGroupBase
{
public:
    std::string Trace;

    __fastcall TInnerTB_SetUpGroupThrows()
        : TTestGroupBase("InnerSetUpGroupThrows")
    {
    }

    virtual void SetUp_Group()
    {
        throw std::runtime_error("setup boom");
    }

    virtual void TearDown_Group()
    {
        Trace += "TDG;";
    }

__published:
    void __fastcall Test_X();
    void __fastcall Test_Y();
};

void __fastcall TInnerTB_SetUpGroupThrows::Test_X()
{
    Trace += "X;";
}

void __fastcall TInnerTB_SetUpGroupThrows::Test_Y()
{
    Trace += "Y;";
}

class TInnerTB_SetUpTestThrows : public TTestGroupBase
{
public:
    std::string Trace;

    __fastcall TInnerTB_SetUpTestThrows()
        : TTestGroupBase("InnerSetUpTestThrows")
    {
    }

    virtual void TearDown_Group()
    {
        Trace += "TDG;";
    }

protected:
    virtual void SetUp_Test(ITestCase& /*testCase*/)
    {
        StrToInt(L"y");
    }

__published:
    void __fastcall Test_A();
};

void __fastcall TInnerTB_SetUpTestThrows::Test_A()
{
    Trace += "A;";
}

class TInnerTB_TearDownTestThrows : public TTestGroupBase
{
public:
    __fastcall TInnerTB_TearDownTestThrows()
        : TTestGroupBase("InnerTearDownTestThrows")
    {
    }

protected:
    virtual void TearDown_Test(ITestCase& /*testCase*/)
    {
        throw 42;
    }

__published:
    void __fastcall Test_A();
};

void __fastcall TInnerTB_TearDownTestThrows::Test_A()
{
}

class TInnerTB_TearDownGroupThrows : public TTestGroupBase
{
public:
    __fastcall TInnerTB_TearDownGroupThrows()
        : TTestGroupBase("InnerTearDownGroupThrows")
    {
    }

    virtual void TearDown_Group()
    {
        throw std::logic_error("teardown boom");
    }

__published:
    void __fastcall Test_A();
};

void __fastcall TInnerTB_TearDownGroupThrows::Test_A()
{
}

// Its Test() lets an exception out, as only a framework failure would.
class TInnerTB_TestEscapes : public TTestGroupBase
{
public:
    enum class TKind
    {
        Std,
        RTL,
        Other,
    };

    TKind Kind;

    __fastcall TInnerTB_TestEscapes()
        : TTestGroupBase("InnerTestEscapes"),
          Kind(TKind::Std)
    {
    }

protected:
    virtual void Test(ITestCase& testCase);

__published:
    void __fastcall Test_A();
};

void TInnerTB_TestEscapes::Test(ITestCase& /*testCase*/)
{
    if (Kind == TKind::Std)
        throw std::runtime_error("escaped");

    if (Kind == TKind::RTL)
        throw EConvertError("rtl escaped");

    throw 7;
}

void __fastcall TInnerTB_TestEscapes::Test_A()
{
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Exceptions: unexpected and expected exceptions
/////////////////////////////////////////////////////////////////////////////

// A C++ exception type that isn't a std::exception, and a descendant of it.
class TInnerTB_PlainError
{
public:
    virtual ~TInnerTB_PlainError()
    {
    }
};

class TInnerTB_PlainDerivedError : public TInnerTB_PlainError
{
};

class TInnerTB_Exceptions : public TTestGroupBase
{
public:
    std::string Trace;
    int NoneLine;
    int WideMessageLine;

    __fastcall TInnerTB_Exceptions()
        : TTestGroupBase("InnerExceptions"),
          NoneLine(0),
          WideMessageLine(0)
    {
    }

protected:
    virtual void TearDown_Test(ITestCase& testCase)
    {
        Trace += "TDT:" + testCase.GetName() + ";";
    }

__published:
    void __fastcall Test_Unexpected_Std();
    void __fastcall Test_Unexpected_RTL();
    void __fastcall Test_Unexpected_Other();
    void __fastcall Test_Unexpected_AccessViolation();
    void __fastcall Test_Any_Std();
    void __fastcall Test_Any_RTL();
    void __fastcall Test_Any_Other();
    void __fastcall Test_Any_None();
    void __fastcall Test_Any_Cancelled();
    void __fastcall Test_Any_NotCarriedOver();
    void __fastcall Test_RTL_Match();
    void __fastcall Test_RTL_Base();
    void __fastcall Test_RTL_Wrong();
    void __fastcall Test_RTL_GotStd();
    void __fastcall Test_Std_Match();
    void __fastcall Test_Std_Base();
    void __fastcall Test_Std_Wrong();
    void __fastcall Test_Std_GotRTL();
    void __fastcall Test_Other_Match();
    void __fastcall Test_Other_Derived();
    void __fastcall Test_Other_Wrong();
    void __fastcall Test_Other_GotStd();
    void __fastcall Test_Other_Int();
    void __fastcall Test_Message_RTL_Match();
    void __fastcall Test_Message_RTL_Wrong();
    void __fastcall Test_Message_Std_Match();
    void __fastcall Test_Message_Std_Wrong();
    void __fastcall Test_Message_Other();
    void __fastcall Test_FailWins();
    void __fastcall Test_SkipWins();
    void __fastcall Test_WideMessage();
};

void __fastcall TInnerTB_Exceptions::Test_Unexpected_Std()
{
    throw std::runtime_error("std boom");
}

void __fastcall TInnerTB_Exceptions::Test_Unexpected_RTL()
{
    StrToInt(L"x");
}

void __fastcall TInnerTB_Exceptions::Test_Unexpected_Other()
{
    throw 42;
}

void __fastcall TInnerTB_Exceptions::Test_Unexpected_AccessViolation()
{
    int* volatile pointer = nullptr;
    *pointer = 1;
}

void __fastcall TInnerTB_Exceptions::Test_Any_Std()
{
    SetExceptionExpected(true, ASWBCB_HERE, "m");
    throw std::runtime_error("e");
}

void __fastcall TInnerTB_Exceptions::Test_Any_RTL()
{
    SetExceptionExpected(true, ASWBCB_HERE);
    StrToInt(L"x");
}

void __fastcall TInnerTB_Exceptions::Test_Any_Other()
{
    SetExceptionExpected(true, ASWBCB_HERE);
    throw 1;
}

void __fastcall TInnerTB_Exceptions::Test_Any_None()
{
    NoneLine = __LINE__ + 1;
    SetExceptionExpected(true, ASWBCB_HERE, "why");
}

void __fastcall TInnerTB_Exceptions::Test_Any_Cancelled()
{
    SetExceptionExpected(true, ASWBCB_HERE);
    SetExceptionExpected(false, ASWBCB_HERE);
}

void __fastcall TInnerTB_Exceptions::Test_Any_NotCarriedOver()
{
    // Runs after tests that expected exceptions, and expects none itself.
}

void __fastcall TInnerTB_Exceptions::Test_RTL_Match()
{
    SetExceptionExpected<EConvertError>(ASWBCB_HERE);
    StrToInt(L"x");
}

void __fastcall TInnerTB_Exceptions::Test_RTL_Base()
{
    SetExceptionExpected<Exception>(ASWBCB_HERE);
    StrToInt(L"x");
}

void __fastcall TInnerTB_Exceptions::Test_RTL_Wrong()
{
    SetExceptionExpected<EAccessViolation>(ASWBCB_HERE);
    StrToInt(L"x");
}

void __fastcall TInnerTB_Exceptions::Test_RTL_GotStd()
{
    SetExceptionExpected<EConvertError>(ASWBCB_HERE);
    throw std::runtime_error("s");
}

void __fastcall TInnerTB_Exceptions::Test_Std_Match()
{
    SetExceptionExpected<std::runtime_error>(ASWBCB_HERE);
    throw std::runtime_error("s");
}

void __fastcall TInnerTB_Exceptions::Test_Std_Base()
{
    SetExceptionExpected<std::exception>(ASWBCB_HERE);
    throw std::logic_error("l");
}

void __fastcall TInnerTB_Exceptions::Test_Std_Wrong()
{
    SetExceptionExpected<std::logic_error>(ASWBCB_HERE);
    throw std::runtime_error("r");
}

void __fastcall TInnerTB_Exceptions::Test_Std_GotRTL()
{
    SetExceptionExpected<std::runtime_error>(ASWBCB_HERE);
    StrToInt(L"x");
}

void __fastcall TInnerTB_Exceptions::Test_Other_Match()
{
    SetExceptionExpected<TInnerTB_PlainError>(ASWBCB_HERE);
    throw TInnerTB_PlainError();
}

void __fastcall TInnerTB_Exceptions::Test_Other_Derived()
{
    SetExceptionExpected<TInnerTB_PlainError>(ASWBCB_HERE);
    throw TInnerTB_PlainDerivedError();
}

void __fastcall TInnerTB_Exceptions::Test_Other_Wrong()
{
    SetExceptionExpected<TInnerTB_PlainError>(ASWBCB_HERE);
    throw 42;
}

void __fastcall TInnerTB_Exceptions::Test_Other_GotStd()
{
    SetExceptionExpected<TInnerTB_PlainError>(ASWBCB_HERE);
    throw std::runtime_error("s");
}

void __fastcall TInnerTB_Exceptions::Test_Other_Int()
{
    SetExceptionExpected<int>(ASWBCB_HERE);
    throw 7;
}

void __fastcall TInnerTB_Exceptions::Test_Message_RTL_Match()
{
    SetExceptionExpected<EConvertError>(ASWBCB_HERE, "", "not a valid");
    StrToInt(L"x");
}

void __fastcall TInnerTB_Exceptions::Test_Message_RTL_Wrong()
{
    SetExceptionExpected<EConvertError>(ASWBCB_HERE, "", L"nope");
    StrToInt(L"x");
}

void __fastcall TInnerTB_Exceptions::Test_Message_Std_Match()
{
    SetExceptionExpected<std::runtime_error>(ASWBCB_HERE, "", std::string("boo"));
    throw std::runtime_error("a boom");
}

void __fastcall TInnerTB_Exceptions::Test_Message_Std_Wrong()
{
    SetExceptionExpected<std::runtime_error>(ASWBCB_HERE, "", "zzz");
    throw std::runtime_error("a boom");
}

void __fastcall TInnerTB_Exceptions::Test_Message_Other()
{
    SetExceptionExpected<TInnerTB_PlainError>(ASWBCB_HERE, "", "x");
    throw TInnerTB_PlainError();
}

void __fastcall TInnerTB_Exceptions::Test_FailWins()
{
    SetExceptionExpected(true, ASWBCB_HERE);
    Fail(ASWBCB_HERE, "f");
}

void __fastcall TInnerTB_Exceptions::Test_SkipWins()
{
    SetExceptionExpected<std::exception>(ASWBCB_HERE);
    Skip(ASWBCB_HERE, "s");
}

void __fastcall TInnerTB_Exceptions::Test_WideMessage()
{
    WideMessageLine = __LINE__ + 1;
    SetExceptionExpected(true, ASWBCB_HERE, UnicodeString(L"caf\x00E9"));
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Booleans: True and False
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Booleans : public TTestGroupBase
{
public:
    std::string Trace;
    int FirstCheckLine;
    std::map<std::string, int> Lines; // The line of each test's failing assertion, by test name

    __fastcall TInnerTB_Booleans()
        : TTestGroupBase("InnerBooleans"),
          FirstCheckLine(0)
    {
    }

__published:
    void __fastcall Test_AssertFalse_Fails();
    void __fastcall Test_AssertTrue_Fails();
    void __fastcall Test_Checks_Fail();
};

void __fastcall TInnerTB_Booleans::Test_AssertFalse_Fails()
{
    Lines["AssertFalse_Fails"] = __LINE__ + 1;
    AssertFalse(true, ASWBCB_HERE, "m");
    Trace += "AfterAssertFalse;";
}

void __fastcall TInnerTB_Booleans::Test_AssertTrue_Fails()
{
    Lines["AssertTrue_Fails"] = __LINE__ + 1;
    AssertTrue(false, ASWBCB_HERE, "m");
    Trace += "AfterAssertTrue;";
}

void __fastcall TInnerTB_Booleans::Test_Checks_Fail()
{
    FirstCheckLine = __LINE__ + 1;
    CheckFalse(true, ASWBCB_HERE, "m");
    CheckTrue(false, ASWBCB_HERE, "m");
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Checks: Check* and Assert* failures in whole tests
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Checks : public TTestGroupBase
{
public:
    std::string Trace;
    int FirstCheckLine;
    int SecondCheckLine;

    __fastcall TInnerTB_Checks()
        : TTestGroupBase("InnerChecks"),
          FirstCheckLine(0),
          SecondCheckLine(0)
    {
    }

__published:
    void __fastcall Test_TwoChecksFail();
    void __fastcall Test_CheckThenFail();
    void __fastcall Test_CheckThenSkip();
    void __fastcall Test_CheckThenExpectedException();
    void __fastcall Test_AssertFails();
    void __fastcall Test_ChecksPass();
};

void __fastcall TInnerTB_Checks::Test_TwoChecksFail()
{
    FirstCheckLine = __LINE__ + 1;
    CheckEquals(1, 2, ASWBCB_HERE, "a");
    SecondCheckLine = __LINE__ + 1;
    CheckTrue(false, ASWBCB_HERE, "b");
    Trace += "AfterChecks;";
}

void __fastcall TInnerTB_Checks::Test_CheckThenFail()
{
    CheckEquals(1, 2, ASWBCB_HERE);
    Fail(ASWBCB_HERE, "then fail");
}

void __fastcall TInnerTB_Checks::Test_CheckThenSkip()
{
    CheckEquals(1, 2, ASWBCB_HERE);
    Skip(ASWBCB_HERE, "then skip");
}

void __fastcall TInnerTB_Checks::Test_CheckThenExpectedException()
{
    SetExceptionExpected(true, ASWBCB_HERE);
    CheckEquals(1, 2, ASWBCB_HERE);
    throw 1;
}

void __fastcall TInnerTB_Checks::Test_AssertFails()
{
    AssertEquals("a", L"b", ASWBCB_HERE);
    Trace += "AfterAssert;";
}

void __fastcall TInnerTB_Checks::Test_ChecksPass()
{
    CheckEquals(1, 1, ASWBCB_HERE);
    CheckTrue(true, ASWBCB_HERE);
    CheckFalse(false, ASWBCB_HERE);
    AssertEquals(std::string("x"), "x", ASWBCB_HERE);
    AssertTrue(true, ASWBCB_HERE);
    AssertFalse(false, ASWBCB_HERE);
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Emptiness: Empty and NotEmpty
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Emptiness : public TTestGroupBase
{
public:
    std::string Trace;
    int FirstCheckLine;
    std::map<std::string, int> Lines; // The line of each test's failing assertion, by test name

    __fastcall TInnerTB_Emptiness()
        : TTestGroupBase("InnerEmptiness"),
          FirstCheckLine(0)
    {
    }

__published:
    void __fastcall Test_AssertEmpty_Fails();
    void __fastcall Test_AssertNotEmpty_Fails();
    void __fastcall Test_Checks_Fail();
    void __fastcall Test_Passes();
};

void __fastcall TInnerTB_Emptiness::Test_AssertEmpty_Fails()
{
    Lines["AssertEmpty_Fails"] = __LINE__ + 1;
    AssertEmpty(std::string("abc"), ASWBCB_HERE, "m");
    Trace += "AfterAssertEmpty;";
}

void __fastcall TInnerTB_Emptiness::Test_AssertNotEmpty_Fails()
{
    Lines["AssertNotEmpty_Fails"] = __LINE__ + 1;
    AssertNotEmpty(std::vector<int>(), ASWBCB_HERE, "m");
    Trace += "AfterAssertNotEmpty;";
}

void __fastcall TInnerTB_Emptiness::Test_Checks_Fail()
{
    std::vector<int> const three(3, 0);
    char const* const nullText = nullptr;

    FirstCheckLine = __LINE__ + 1;
    CheckEmpty(three, ASWBCB_HERE, "m");
    CheckEmpty(nullText, ASWBCB_HERE, "m");
    CheckEmpty(UnicodeString(L"x"), ASWBCB_HERE, "m");
    CheckNotEmpty(L"", ASWBCB_HERE, "m");
    CheckNotEmpty(TBytes(), ASWBCB_HERE, "m");
    CheckNotEmpty(nullText, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Emptiness::Test_Passes()
{
    std::map<int, int> map;
    TBytes bytes;
    bytes.Length = 2;

    CheckEmpty(map, ASWBCB_HERE);
    CheckEmpty("", ASWBCB_HERE);
    CheckEmpty(std::wstring(), ASWBCB_HERE);
    CheckEmpty(AnsiString(), ASWBCB_HERE);
    CheckNotEmpty(bytes, ASWBCB_HERE);
    CheckNotEmpty(UTF8String("a"), ASWBCB_HERE);
    AssertEmpty(UnicodeString(), ASWBCB_HERE);
    AssertNotEmpty(std::vector<int>(1, 0), ASWBCB_HERE);
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Equals: Equals with enums and pointers, and NotEquals
/////////////////////////////////////////////////////////////////////////////
enum TInnerTB_Color
{
    InnerTB_Red,
    InnerTB_Blue,
};

class TInnerTB_Equals : public TTestGroupBase
{
public:
    std::string Trace;
    int First;
    int Second;
    std::map<std::string, int> Lines; // The line of each test's failing assertion, by test name

    __fastcall TInnerTB_Equals()
        : TTestGroupBase("InnerEquals"),
          First(1),
          Second(1)
    {
    }

__published:
    void __fastcall Test_AssertEquals_Enums_Fails();
    void __fastcall Test_AssertEqualsIC_Fails();
    void __fastcall Test_AssertNotEquals_Fails();
    void __fastcall Test_AssertNotEqualsIC_Fails();
    void __fastcall Test_CheckEquals_Pointers_Fails();
    void __fastcall Test_CheckEqualsIC_Fails();
    void __fastcall Test_CheckNotEquals_Chars_Fails();
    void __fastcall Test_CheckNotEquals_Enums_Fails();
    void __fastcall Test_CheckNotEquals_Integers_Fails();
    void __fastcall Test_CheckNotEquals_NullStrings_Fails();
    void __fastcall Test_CheckNotEquals_Pointers_Fails();
    void __fastcall Test_CheckNotEqualsIC_Fails();
    void __fastcall Test_EqualsIC_Passes();
    void __fastcall Test_NotEquals_Passes();
};

void __fastcall TInnerTB_Equals::Test_AssertEquals_Enums_Fails()
{
    Lines["AssertEquals_Enums_Fails"] = __LINE__ + 1;
    AssertEquals(InnerTB_Red, InnerTB_Blue, ASWBCB_HERE, "enums");
    Trace += "AfterAssertEquals;";
}

void __fastcall TInnerTB_Equals::Test_AssertEqualsIC_Fails()
{
    Lines["AssertEqualsIC_Fails"] = __LINE__ + 1;
    AssertEqualsIC("abc", L"ABD", ASWBCB_HERE, "ic");
    Trace += "AfterAssertEqualsIC;";
}

void __fastcall TInnerTB_Equals::Test_AssertNotEquals_Fails()
{
    Lines["AssertNotEquals_Fails"] = __LINE__ + 1;
    AssertNotEquals(7, 7u, ASWBCB_HERE, "seven");
    Trace += "AfterAssertNotEquals;";
}

void __fastcall TInnerTB_Equals::Test_AssertNotEqualsIC_Fails()
{
    Lines["AssertNotEqualsIC_Fails"] = __LINE__ + 1;
    AssertNotEqualsIC("abc", "ABC", ASWBCB_HERE, "ic");
    Trace += "AfterAssertNotEqualsIC;";
}

void __fastcall TInnerTB_Equals::Test_CheckEquals_Pointers_Fails()
{
    Lines["CheckEquals_Pointers_Fails"] = __LINE__ + 1;
    CheckEquals(&First, &Second, ASWBCB_HERE, "pointers");
}

void __fastcall TInnerTB_Equals::Test_CheckEqualsIC_Fails()
{
    Lines["CheckEqualsIC_Fails"] = __LINE__ + 1;
    CheckEqualsIC(std::string("abc"), "abd", ASWBCB_HERE, "ic");
    Trace += "AfterCheckEqualsIC;";
}

void __fastcall TInnerTB_Equals::Test_CheckNotEquals_Chars_Fails()
{
    Lines["CheckNotEquals_Chars_Fails"] = __LINE__ + 1;
    CheckNotEquals('a', 'a', ASWBCB_HERE, "chars");
}

void __fastcall TInnerTB_Equals::Test_CheckNotEquals_Enums_Fails()
{
    Lines["CheckNotEquals_Enums_Fails"] = __LINE__ + 1;
    CheckNotEquals(InnerTB_Blue, 1, ASWBCB_HERE, "an enum and an int");
}

void __fastcall TInnerTB_Equals::Test_CheckNotEquals_Integers_Fails()
{
    Lines["CheckNotEquals_Integers_Fails"] = __LINE__ + 1;
    CheckNotEquals(7, 7, ASWBCB_HERE, "integers");
    Trace += "AfterCheckNotEquals;";
}

void __fastcall TInnerTB_Equals::Test_CheckNotEquals_NullStrings_Fails()
{
    char const* const nullString = nullptr;

    Lines["CheckNotEquals_NullStrings_Fails"] = __LINE__ + 1;
    CheckNotEquals(nullString, nullString, ASWBCB_HERE);
}

void __fastcall TInnerTB_Equals::Test_CheckNotEquals_Pointers_Fails()
{
    Lines["CheckNotEquals_Pointers_Fails"] = __LINE__ + 1;
    CheckNotEquals(&First, &First, ASWBCB_HERE, "pointers");
}

void __fastcall TInnerTB_Equals::Test_CheckNotEqualsIC_Fails()
{
    Lines["CheckNotEqualsIC_Fails"] = __LINE__ + 1;
    CheckNotEqualsIC(L"abc", UnicodeString(L"ABC"), ASWBCB_HERE, "ic");
}

void __fastcall TInnerTB_Equals::Test_EqualsIC_Passes()
{
    CheckEqualsIC("abc", "ABC", ASWBCB_HERE);
    CheckEqualsIC(AnsiString("Abc"), L"aBC", ASWBCB_HERE);
    CheckEqualsIC(String("Abc"), String("aBC"), ASWBCB_HERE);
    CheckNotEqualsIC("abc", "abd", ASWBCB_HERE);
    CheckNotEqualsIC("abc", "abcd", ASWBCB_HERE);
    AssertEqualsIC(std::wstring(L"x"), "X", ASWBCB_HERE);
    AssertNotEqualsIC(L"x", L"y", ASWBCB_HERE);
}

void __fastcall TInnerTB_Equals::Test_NotEquals_Passes()
{
    char const* const nullString = nullptr;

    CheckNotEquals(1, 2, ASWBCB_HERE);
    CheckNotEquals(1u, 2u, ASWBCB_HERE);
    CheckNotEquals('a', 'b', ASWBCB_HERE);
    CheckNotEquals(-1, 0xFFFFFFFFu, ASWBCB_HERE);
    CheckNotEquals(true, false, ASWBCB_HERE);
    CheckNotEquals(InnerTB_Red, InnerTB_Blue, ASWBCB_HERE);
    CheckNotEquals(InnerTB_Red, 1, ASWBCB_HERE);
    CheckNotEquals(&First, &Second, ASWBCB_HERE);
    CheckNotEquals("a", "b", ASWBCB_HERE);
    CheckNotEquals(nullString, "", ASWBCB_HERE);
    CheckNotEquals(std::string("a"), L"b", ASWBCB_HERE);
    CheckNotEquals(UnicodeString(L"a"), AnsiString("b"), ASWBCB_HERE);
    AssertNotEquals(1, 2, ASWBCB_HERE);
    AssertNotEquals(&First, &Second, ASWBCB_HERE);
    AssertNotEquals("a", L"b", ASWBCB_HERE);
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Matches: Matches and NotMatches
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Matches : public TTestGroupBase
{
public:
    std::string Trace;
    int FirstCheckLine;
    std::map<std::string, int> Lines; // The line of each test's failing assertion, by test name

    __fastcall TInnerTB_Matches()
        : TTestGroupBase("InnerMatches"),
          FirstCheckLine(0)
    {
    }

__published:
    void __fastcall Test_AssertMatches_Fails();
    void __fastcall Test_AssertNotMatches_Fails();
    void __fastcall Test_Checks_Fail();
    void __fastcall Test_Passes();
};

void __fastcall TInnerTB_Matches::Test_AssertMatches_Fails()
{
    Lines["AssertMatches_Fails"] = __LINE__ + 1;
    AssertMatches(std::string("2026-1-7"), "\\d{4}-\\d{2}-\\d{2}", ASWBCB_HERE, "m");
    Trace += "AfterAssertMatches;";
}

void __fastcall TInnerTB_Matches::Test_AssertNotMatches_Fails()
{
    Lines["AssertNotMatches_Fails"] = __LINE__ + 1;
    AssertNotMatches(L"abc123", "[a-z]+\\d+", ASWBCB_HERE, "m");
    Trace += "AfterAssertNotMatches;";
}

void __fastcall TInnerTB_Matches::Test_Checks_Fail()
{
    FirstCheckLine = __LINE__ + 1;
    CheckMatches(UnicodeString(L"Error 42"), "Error \\d", ASWBCB_HERE, "m");
    CheckMatches("abc", "[a-", ASWBCB_HERE, "m");
    CheckNotMatches(L"x", std::wstring(L"."), ASWBCB_HERE, "m");
    CheckNotMatches("abc", "a(", ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Matches::Test_Passes()
{
    char const* const nullText = nullptr;

    CheckMatches("Error 42: disk full", "Error \\d+: .*", ASWBCB_HERE);
    CheckMatches(L"caf\x00E9", "caf.", ASWBCB_HERE);
    CheckMatches(std::string(), "", ASWBCB_HERE);
    CheckNotMatches("abc", "\\d+", ASWBCB_HERE);
    CheckNotMatches(nullText, ".*", ASWBCB_HERE, "a null text never matches");
    AssertMatches(AnsiString("ABC"), "(?i)abc", ASWBCB_HERE);
    AssertNotMatches(std::wstring(L"abc"), L"ab", ASWBCB_HERE, "the whole text");
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Memory: EqualsMem and NotEqualsMem
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Memory : public TTestGroupBase
{
public:
    std::string Trace;
    int FirstCheckLine;
    std::map<std::string, int> Lines; // The line of each test's failing assertion, by test name

    __fastcall TInnerTB_Memory()
        : TTestGroupBase("InnerMemory"),
          FirstCheckLine(0)
    {
    }

__published:
    void __fastcall Test_AssertEqualsMem_Fails();
    void __fastcall Test_AssertNotEqualsMem_Fails();
    void __fastcall Test_Checks_Fail();
    void __fastcall Test_Passes();
};

void __fastcall TInnerTB_Memory::Test_AssertEqualsMem_Fails()
{
    unsigned char const expected[] = { 0x01, 0x02, 0x03, 0x04 };
    unsigned char const actual[] = { 0x01, 0x02, 0xFF, 0x04 };

    Lines["AssertEqualsMem_Fails"] = __LINE__ + 1;
    AssertEqualsMem(expected, actual, sizeof(expected), ASWBCB_HERE, "m");
    Trace += "AfterAssertEqualsMem;";
}

void __fastcall TInnerTB_Memory::Test_AssertNotEqualsMem_Fails()
{
    unsigned char const expected[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    unsigned char const actual[] = { 0xDE, 0xAD, 0xBE, 0xEF };

    Lines["AssertNotEqualsMem_Fails"] = __LINE__ + 1;
    AssertNotEqualsMem(expected, actual, sizeof(expected), ASWBCB_HERE, "m");
    Trace += "AfterAssertNotEqualsMem;";
}

void __fastcall TInnerTB_Memory::Test_Checks_Fail()
{
    int const values[] = { 1, 2 };
    std::string const text = "abc";

    FirstCheckLine = __LINE__ + 1;
    CheckEqualsMem(values, nullptr, sizeof(values), ASWBCB_HERE, "m");
    CheckEqualsMem(text.c_str(), "abd", 3, ASWBCB_HERE, "m");
    CheckNotEqualsMem(values, values, sizeof(values), ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Memory::Test_Passes()
{
    int const values[] = { 1, 2 };
    int const sameValues[] = { 1, 2 };
    int const otherValues[] = { 2, 1 };

    CheckEqualsMem(values, sameValues, sizeof(values), ASWBCB_HERE);
    CheckEqualsMem(values, values, sizeof(values), ASWBCB_HERE);
    CheckEqualsMem(nullptr, nullptr, 4, ASWBCB_HERE);
    CheckEqualsMem(values, otherValues, 0, ASWBCB_HERE);
    CheckNotEqualsMem(values, otherValues, sizeof(values), ASWBCB_HERE);
    CheckNotEqualsMem(nullptr, values, sizeof(values), ASWBCB_HERE);
    AssertEqualsMem("abc", std::string("abc").c_str(), 3, ASWBCB_HERE);
    AssertNotEqualsMem(values, otherValues, sizeof(values), ASWBCB_HERE);
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Near: Near and NotNear
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Near : public TTestGroupBase
{
public:
    std::string Trace;
    std::map<std::string, int> Lines; // The line of each test's failing assertion, by test name

    __fastcall TInnerTB_Near()
        : TTestGroupBase("InnerNear")
    {
    }

__published:
    void __fastcall Test_AssertNear_Fails();
    void __fastcall Test_AssertNear_NegativeTolerance_Fails();
    void __fastcall Test_AssertNotNear_Fails();
    void __fastcall Test_AssertNotNear_NaNTolerance_Fails();
    void __fastcall Test_CheckNear_Fails();
    void __fastcall Test_CheckNear_Float_Fails();
    void __fastcall Test_CheckNear_Float_NegativeTolerance_Fails();
    void __fastcall Test_CheckNear_NaN_Fails();
    void __fastcall Test_CheckNear_NaNTolerance_Fails();
    void __fastcall Test_CheckNotNear_Fails();
    void __fastcall Test_CheckNotNear_Float_Fails();
    void __fastcall Test_CheckNotNear_NegativeTolerance_Fails();
    void __fastcall Test_Passes();
};

void __fastcall TInnerTB_Near::Test_AssertNear_Fails()
{
    Lines["AssertNear_Fails"] = __LINE__ + 1;
    AssertNear(1.0, 1.25, 0.125, ASWBCB_HERE, "m");
    Trace += "AfterAssertNear;";
}

void __fastcall TInnerTB_Near::Test_AssertNear_NegativeTolerance_Fails()
{
    Lines["AssertNear_NegativeTolerance_Fails"] = __LINE__ + 1;
    AssertNear(1.0, 1.0, -0.5, ASWBCB_HERE, "m");
    Trace += "AfterAssertNearNegativeTolerance;";
}

void __fastcall TInnerTB_Near::Test_AssertNotNear_Fails()
{
    Lines["AssertNotNear_Fails"] = __LINE__ + 1;
    AssertNotNear(1.0, 1.25, 0.5, ASWBCB_HERE, "m");
    Trace += "AfterAssertNotNear;";
}

void __fastcall TInnerTB_Near::Test_AssertNotNear_NaNTolerance_Fails()
{
    Lines["AssertNotNear_NaNTolerance_Fails"] = __LINE__ + 1;
    AssertNotNear(1.0f, 2.0f, std::numeric_limits<float>::quiet_NaN(), ASWBCB_HERE, "m");
    Trace += "AfterAssertNotNearNaNTolerance;";
}

void __fastcall TInnerTB_Near::Test_CheckNear_Fails()
{
    Lines["CheckNear_Fails"] = __LINE__ + 1;
    CheckNear(1.0, 1.25, 0.125, ASWBCB_HERE, "m");
    Trace += "AfterCheckNear;";
}

void __fastcall TInnerTB_Near::Test_CheckNear_Float_Fails()
{
    Lines["CheckNear_Float_Fails"] = __LINE__ + 1;
    CheckNear(2.5f, 3.0f, 0.25f, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Near::Test_CheckNear_Float_NegativeTolerance_Fails()
{
    Lines["CheckNear_Float_NegativeTolerance_Fails"] = __LINE__ + 1;
    CheckNear(1.0f, 1.0f, -0.25f, ASWBCB_HERE, "m");
    Trace += "AfterCheckNearNegativeTolerance;";
}

void __fastcall TInnerTB_Near::Test_CheckNear_NaN_Fails()
{
    Lines["CheckNear_NaN_Fails"] = __LINE__ + 1;
    CheckNear(std::numeric_limits<double>::quiet_NaN(), 1.0, 0.5, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Near::Test_CheckNear_NaNTolerance_Fails()
{
    Lines["CheckNear_NaNTolerance_Fails"] = __LINE__ + 1;
    CheckNear(1.0, 1.0, std::numeric_limits<double>::quiet_NaN(), ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Near::Test_CheckNotNear_Fails()
{
    Lines["CheckNotNear_Fails"] = __LINE__ + 1;
    CheckNotNear(1.0, 1.25, 0.5, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Near::Test_CheckNotNear_Float_Fails()
{
    Lines["CheckNotNear_Float_Fails"] = __LINE__ + 1;
    CheckNotNear(1.0f, 1.0f, 0.0f, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Near::Test_CheckNotNear_NegativeTolerance_Fails()
{
    Lines["CheckNotNear_NegativeTolerance_Fails"] = __LINE__ + 1;
    CheckNotNear(1.0, 2.0, -1.0, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Near::Test_Passes()
{
    double const nan = std::numeric_limits<double>::quiet_NaN();
    double const infinity = std::numeric_limits<double>::infinity();

    CheckNear(1.0, 1.05, 0.1, ASWBCB_HERE);
    CheckNear(1.0, 0.95, 0.1, ASWBCB_HERE, "below");
    CheckNear(1.0, 1.5, 0.5, ASWBCB_HERE, "a difference equal to the tolerance");
    CheckNear(1.0, 1.0, 0.0, ASWBCB_HERE, "equal values with no tolerance");
    CheckNear(1.0, 1.0, -0.0, ASWBCB_HERE, "a negative zero tolerance is zero");
    CheckNear(infinity, infinity, 0.0, ASWBCB_HERE, "equal infinities");
    CheckNear(2.0f, 2.25f, 0.5f, ASWBCB_HERE, "floats");
    CheckNotNear(1.0, 2.0, 0.5, ASWBCB_HERE);
    CheckNotNear(1.0, 1.5, 0.25, ASWBCB_HERE);
    CheckNotNear(nan, nan, 1.0, ASWBCB_HERE, "a NaN is near nothing");
    CheckNotNear(1.0, nan, 1.0, ASWBCB_HERE, "a NaN is near nothing");
    CheckNotNear(infinity, -infinity, 1.0, ASWBCB_HERE);
    CheckNotNear(2.0f, 3.0f, 0.5f, ASWBCB_HERE, "floats");
    AssertNear(10.0, 10.5, 1.0, ASWBCB_HERE);
    AssertNotNear(10.0, 12.0, 1.0, ASWBCB_HERE);
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Objects: Null, Same and IsType, and their Not forms
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Shape
{
public:
    virtual ~TInnerTB_Shape()
    {
    }
};

class TInnerTB_Circle : public TInnerTB_Shape
{
};

class TInnerTB_Square : public TInnerTB_Shape
{
};

class TInnerTB_Objects : public TTestGroupBase
{
public:
    std::string Trace;
    int First;
    int Second;
    int Values[2];
    TInnerTB_Circle Circle;
    std::map<std::string, int> Lines; // The line of each test's failing assertion, by test name

    __fastcall TInnerTB_Objects()
        : TTestGroupBase("InnerObjects"),
          First(1),
          Second(1)
    {
        Values[0] = 1;
        Values[1] = 2;
    }

__published:
    void __fastcall Test_AssertIsNotType_Fails();
    void __fastcall Test_AssertIsType_Fails();
    void __fastcall Test_AssertNotNull_Fails();
    void __fastcall Test_AssertNotSame_Fails();
    void __fastcall Test_AssertNull_Fails();
    void __fastcall Test_AssertSame_Fails();
    void __fastcall Test_CheckIsNotType_Fails();
    void __fastcall Test_CheckIsType_Fails();
    void __fastcall Test_CheckNotNull_Fails();
    void __fastcall Test_CheckNotSame_Fails();
    void __fastcall Test_CheckNull_Fails();
    void __fastcall Test_CheckSame_Fails();
    void __fastcall Test_Passes();
};

void __fastcall TInnerTB_Objects::Test_AssertIsNotType_Fails()
{
    TInnerTB_Shape* const shape = &Circle;

    Lines["AssertIsNotType_Fails"] = __LINE__ + 1;
    AssertIsNotType<TInnerTB_Circle>(shape, ASWBCB_HERE, "m");
    Trace += "AfterAssertIsNotType;";
}

void __fastcall TInnerTB_Objects::Test_AssertIsType_Fails()
{
    TInnerTB_Shape* const shape = &Circle;

    Lines["AssertIsType_Fails"] = __LINE__ + 1;
    AssertIsType<TInnerTB_Square>(shape, ASWBCB_HERE, "m");
    Trace += "AfterAssertIsType;";
}

void __fastcall TInnerTB_Objects::Test_AssertNotNull_Fails()
{
    int* const nullPointer = nullptr;

    Lines["AssertNotNull_Fails"] = __LINE__ + 1;
    AssertNotNull(nullPointer, ASWBCB_HERE, "m");
    Trace += "AfterAssertNotNull;";
}

void __fastcall TInnerTB_Objects::Test_AssertNotSame_Fails()
{
    Lines["AssertNotSame_Fails"] = __LINE__ + 1;
    AssertNotSame(&First, &First, ASWBCB_HERE, "m");
    Trace += "AfterAssertNotSame;";
}

void __fastcall TInnerTB_Objects::Test_AssertNull_Fails()
{
    Lines["AssertNull_Fails"] = __LINE__ + 1;
    AssertNull(&First, ASWBCB_HERE, "m");
    Trace += "AfterAssertNull;";
}

void __fastcall TInnerTB_Objects::Test_AssertSame_Fails()
{
    Lines["AssertSame_Fails"] = __LINE__ + 1;
    AssertSame(&First, &Second, ASWBCB_HERE, "m");
    Trace += "AfterAssertSame;";
}

void __fastcall TInnerTB_Objects::Test_CheckIsNotType_Fails()
{
    std::unique_ptr<TStringList> const list(new TStringList());

    Lines["CheckIsNotType_Fails"] = __LINE__ + 1;
    CheckIsNotType<TStrings>(list, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Objects::Test_CheckIsType_Fails()
{
    TInnerTB_Shape* const nullShape = nullptr;

    Lines["CheckIsType_Fails"] = __LINE__ + 1;
    CheckIsType<TInnerTB_Circle>(nullShape, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Objects::Test_CheckNotNull_Fails()
{
    std::unique_ptr<int> const empty;

    Lines["CheckNotNull_Fails"] = __LINE__ + 1;
    CheckNotNull(empty, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Objects::Test_CheckNotSame_Fails()
{
    Lines["CheckNotSame_Fails"] = __LINE__ + 1;
    CheckNotSame(Values, &Values[0], ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Objects::Test_CheckNull_Fails()
{
    DelphiInterface<IInterface> someInterface;
    TInterfacedObject* const interfaced = new TInterfacedObject(); // Freed by someInterface

    AssertTrue(interfaced->GetInterface(__uuidof(IInterface), &someInterface), ASWBCB_HERE);
    Lines["CheckNull_Fails"] = __LINE__ + 1;
    CheckNull(someInterface, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Objects::Test_CheckSame_Fails()
{
    Lines["CheckSame_Fails"] = __LINE__ + 1;
    CheckSame(&First, &Second, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Objects::Test_Passes()
{
    TInnerTB_Shape* const shape = &Circle;
    TInnerTB_Shape* const nullShape = nullptr;
    int* const nullPointer = nullptr;
    std::unique_ptr<int> const owned(new int(1));
    DelphiInterface<IInterface> const emptyInterface;
    std::unique_ptr<TObject> const list(new TStringList());

    CheckNull(nullPointer, ASWBCB_HERE);
    CheckNull(emptyInterface, ASWBCB_HERE);
    CheckNotNull(owned, ASWBCB_HERE);
    CheckNotNull(&First, ASWBCB_HERE);
    AssertNull(nullShape, ASWBCB_HERE);
    AssertNotNull(shape, ASWBCB_HERE);

    CheckSame(&First, &First, ASWBCB_HERE);
    CheckSame(Values, &Values[0], ASWBCB_HERE, "an array and a pointer");
    CheckSame(owned, owned.get(), ASWBCB_HERE, "a std::unique_ptr and a pointer");
    CheckNotSame(&First, &Second, ASWBCB_HERE, "equal values, different objects");
    AssertSame(shape, &Circle, ASWBCB_HERE, "a base and a derived pointer");
    AssertNotSame("abc", std::string("abc").c_str(), ASWBCB_HERE, "C strings by address");

    CheckIsType<TInnerTB_Circle>(shape, ASWBCB_HERE);
    CheckIsType<TInnerTB_Shape>(Circle, ASWBCB_HERE, "an object");
    CheckIsNotType<TInnerTB_Square>(shape, ASWBCB_HERE);
    CheckIsNotType<TInnerTB_Circle>(nullShape, ASWBCB_HERE);
    AssertIsType<TStrings>(list, ASWBCB_HERE, "an RTL class");
    AssertIsNotType<Exception>(list.get(), ASWBCB_HERE);
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Ordering: GreaterThan, LessThan and their OrEqual forms
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_Ordering : public TTestGroupBase
{
public:
    std::string Trace;
    int FirstCheckLine;
    std::map<std::string, int> Lines; // The line of each test's failing assertion, by test name

    __fastcall TInnerTB_Ordering()
        : TTestGroupBase("InnerOrdering"),
          FirstCheckLine(0)
    {
    }

__published:
    void __fastcall Test_AssertGreaterThan_Fails();
    void __fastcall Test_AssertGreaterThanOrEqual_Fails();
    void __fastcall Test_AssertLessThan_Fails();
    void __fastcall Test_AssertLessThanOrEqual_Fails();
    void __fastcall Test_Checks_Fail();
    void __fastcall Test_Passes();
};

void __fastcall TInnerTB_Ordering::Test_AssertGreaterThan_Fails()
{
    Lines["AssertGreaterThan_Fails"] = __LINE__ + 1;
    AssertGreaterThan(1, 2, ASWBCB_HERE, "m");
    Trace += "AfterAssertGreaterThan;";
}

void __fastcall TInnerTB_Ordering::Test_AssertGreaterThanOrEqual_Fails()
{
    Lines["AssertGreaterThanOrEqual_Fails"] = __LINE__ + 1;
    AssertGreaterThanOrEqual(1.5, 2, ASWBCB_HERE, "m");
    Trace += "AfterAssertGreaterThanOrEqual;";
}

void __fastcall TInnerTB_Ordering::Test_AssertLessThan_Fails()
{
    Lines["AssertLessThan_Fails"] = __LINE__ + 1;
    AssertLessThan(3u, 3, ASWBCB_HERE, "m");
    Trace += "AfterAssertLessThan;";
}

void __fastcall TInnerTB_Ordering::Test_AssertLessThanOrEqual_Fails()
{
    Lines["AssertLessThanOrEqual_Fails"] = __LINE__ + 1;
    AssertLessThanOrEqual(-1, -2, ASWBCB_HERE, "m");
    Trace += "AfterAssertLessThanOrEqual;";
}

void __fastcall TInnerTB_Ordering::Test_Checks_Fail()
{
    double const nan = std::numeric_limits<double>::quiet_NaN();

    FirstCheckLine = __LINE__ + 1;
    CheckGreaterThan(-1, 0xFFFFFFFFu, ASWBCB_HERE, "m");
    CheckGreaterThanOrEqual(0.25f, 0.5f, ASWBCB_HERE, "m");
    CheckLessThan(nan, 1.0, ASWBCB_HERE, "m");
    CheckLessThanOrEqual('b', 'a', ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Ordering::Test_Passes()
{
    CheckGreaterThan(2, 1, ASWBCB_HERE);
    CheckGreaterThan(0xFFFFFFFFu, -1, ASWBCB_HERE, "by value");
    CheckGreaterThanOrEqual(2, 2u, ASWBCB_HERE);
    CheckGreaterThanOrEqual(2.5, 2, ASWBCB_HERE);
    CheckLessThan(1, 2.5f, ASWBCB_HERE);
    CheckLessThan(-3LL, 0ULL, ASWBCB_HERE);
    CheckLessThanOrEqual(2.0, 2, ASWBCB_HERE);
    CheckLessThanOrEqual('a', 'b', ASWBCB_HERE);
    AssertGreaterThan(1.0, -1.0, ASWBCB_HERE);
    AssertGreaterThanOrEqual(5, 5, ASWBCB_HERE);
    AssertLessThan(0.1f, 0.2, ASWBCB_HERE);
    AssertLessThanOrEqual(-1, 0, ASWBCB_HERE);
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Params, TInnerTB_ParamsDuplicate: parameterized tests
/////////////////////////////////////////////////////////////////////////////
struct TInnerTB_HexRow
{
    char Input;
    int Expected;
};

TInnerTB_HexRow InnerTB_HexRow(char input, int expected)
{
    TInnerTB_HexRow row;
    row.Input = input;
    row.Expected = expected;
    return row;
}

std::string InnerTB_HexRowName(TInnerTB_HexRow const& row)
{
    return std::string(1, row.Input);
}

std::string InnerTB_SameName(int const& /*row*/)
{
    return "same";
}

class TInnerTB_Params : public TTestGroupBase
{
public:
    std::string Trace;
    int CheckLine;

    __fastcall TInnerTB_Params()
        : TTestGroupBase("InnerParams"),
          CheckLine(0)
    {
        std::vector<TInnerTB_HexRow> rows;
        rows.push_back(InnerTB_HexRow('A', 10));
        rows.push_back(InnerTB_HexRow('f', 15));
        rows.push_back(InnerTB_HexRow('0', 1)); // Fails: '0' is 0

        RegisterTestCases(Test_Hex, "Hex", rows);
        RegisterTestCases(&TInnerTB_Params::Test_Hex, "HexNamed", rows, InnerTB_HexRowName);

        rows[0].Expected = 99; // Each test case has its own copy of its row
    }

    void __fastcall Test_Hex(TInnerTB_HexRow const& row)
    {
        Trace += row.Input;
        int const value = ((row.Input >= '0') && (row.Input <= '9')) ? (row.Input - '0') :
                ((row.Input | 0x20) - 'a' + 10);
        CheckLine = __LINE__ + 1;
        CheckEquals(row.Expected, value, ASWBCB_HERE);
    }

__published:
    void __fastcall Test_Published();
};

void __fastcall TInnerTB_Params::Test_Published()
{
    Trace += "P";
}

class TInnerTB_ParamsDuplicate : public TTestGroupBase
{
public:
    __fastcall TInnerTB_ParamsDuplicate()
        : TTestGroupBase("InnerParamsDuplicate")
    {
        std::vector<int> rows(2, 0);
        RegisterTestCases(Test_Row, "Row", rows, InnerTB_SameName);
    }

    void __fastcall Test_Row(int const& /*row*/)
    {
    }
};


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_TextParts: Contains, StartsWith and EndsWith, and their Not and IC forms
/////////////////////////////////////////////////////////////////////////////
class TInnerTB_TextParts : public TTestGroupBase
{
public:
    std::string Trace;
    int FirstCheckLine;
    std::map<std::string, int> Lines; // The line of each test's failing assertion, by test name

    __fastcall TInnerTB_TextParts()
        : TTestGroupBase("InnerTextParts"),
          FirstCheckLine(0)
    {
    }

__published:
    void __fastcall Test_AssertContains_Fails();
    void __fastcall Test_AssertContainsIC_Fails();
    void __fastcall Test_AssertEndsWith_Fails();
    void __fastcall Test_AssertEndsWithIC_Fails();
    void __fastcall Test_AssertNotContains_Fails();
    void __fastcall Test_AssertNotContainsIC_Fails();
    void __fastcall Test_AssertNotEndsWith_Fails();
    void __fastcall Test_AssertNotEndsWithIC_Fails();
    void __fastcall Test_AssertNotStartsWith_Fails();
    void __fastcall Test_AssertNotStartsWithIC_Fails();
    void __fastcall Test_AssertStartsWith_Fails();
    void __fastcall Test_AssertStartsWithIC_Fails();
    void __fastcall Test_Checks_Fail();
    void __fastcall Test_Passes();
};

void __fastcall TInnerTB_TextParts::Test_AssertContains_Fails()
{
    Lines["AssertContains_Fails"] = __LINE__ + 1;
    AssertContains(UnicodeString(L"Hello World"), "xyz", ASWBCB_HERE, "m");
    Trace += "AfterContains;";
}

void __fastcall TInnerTB_TextParts::Test_AssertContainsIC_Fails()
{
    Lines["AssertContainsIC_Fails"] = __LINE__ + 1;
    AssertContainsIC(UnicodeString(L"Hello World"), L"XYZ", ASWBCB_HERE, "m");
    Trace += "AfterContainsIC;";
}

void __fastcall TInnerTB_TextParts::Test_AssertEndsWith_Fails()
{
    Lines["AssertEndsWith_Fails"] = __LINE__ + 1;
    AssertEndsWith(UnicodeString(L"Hello World"), "Hello", ASWBCB_HERE, "m");
    Trace += "AfterEndsWith;";
}

void __fastcall TInnerTB_TextParts::Test_AssertEndsWithIC_Fails()
{
    Lines["AssertEndsWithIC_Fails"] = __LINE__ + 1;
    AssertEndsWithIC(UnicodeString(L"Hello World"), L"HELLO", ASWBCB_HERE, "m");
    Trace += "AfterEndsWithIC;";
}

void __fastcall TInnerTB_TextParts::Test_AssertNotContains_Fails()
{
    Lines["AssertNotContains_Fails"] = __LINE__ + 1;
    AssertNotContains(UnicodeString(L"Hello World"), "lo W", ASWBCB_HERE, "m");
    Trace += "AfterNotContains;";
}

void __fastcall TInnerTB_TextParts::Test_AssertNotContainsIC_Fails()
{
    Lines["AssertNotContainsIC_Fails"] = __LINE__ + 1;
    AssertNotContainsIC(UnicodeString(L"Hello World"), L"LO w", ASWBCB_HERE, "m");
    Trace += "AfterNotContainsIC;";
}

void __fastcall TInnerTB_TextParts::Test_AssertNotEndsWith_Fails()
{
    Lines["AssertNotEndsWith_Fails"] = __LINE__ + 1;
    AssertNotEndsWith(UnicodeString(L"Hello World"), "World", ASWBCB_HERE, "m");
    Trace += "AfterNotEndsWith;";
}

void __fastcall TInnerTB_TextParts::Test_AssertNotEndsWithIC_Fails()
{
    Lines["AssertNotEndsWithIC_Fails"] = __LINE__ + 1;
    AssertNotEndsWithIC(UnicodeString(L"Hello World"), L"WORLD", ASWBCB_HERE, "m");
    Trace += "AfterNotEndsWithIC;";
}

void __fastcall TInnerTB_TextParts::Test_AssertNotStartsWith_Fails()
{
    Lines["AssertNotStartsWith_Fails"] = __LINE__ + 1;
    AssertNotStartsWith(UnicodeString(L"Hello World"), "Hello", ASWBCB_HERE, "m");
    Trace += "AfterNotStartsWith;";
}

void __fastcall TInnerTB_TextParts::Test_AssertNotStartsWithIC_Fails()
{
    Lines["AssertNotStartsWithIC_Fails"] = __LINE__ + 1;
    AssertNotStartsWithIC(UnicodeString(L"Hello World"), L"HELLO", ASWBCB_HERE, "m");
    Trace += "AfterNotStartsWithIC;";
}

void __fastcall TInnerTB_TextParts::Test_AssertStartsWith_Fails()
{
    Lines["AssertStartsWith_Fails"] = __LINE__ + 1;
    AssertStartsWith(UnicodeString(L"Hello World"), "World", ASWBCB_HERE, "m");
    Trace += "AfterStartsWith;";
}

void __fastcall TInnerTB_TextParts::Test_AssertStartsWithIC_Fails()
{
    Lines["AssertStartsWithIC_Fails"] = __LINE__ + 1;
    AssertStartsWithIC(UnicodeString(L"Hello World"), L"WORLD", ASWBCB_HERE, "m");
    Trace += "AfterStartsWithIC;";
}

void __fastcall TInnerTB_TextParts::Test_Checks_Fail()
{
    std::string const text = "Hello World";

    FirstCheckLine = __LINE__ + 1;
    CheckContains(text, "xyz", ASWBCB_HERE, "m");
    CheckContainsIC(text, L"XYZ", ASWBCB_HERE, "m");
    CheckEndsWith(text, "Hello", ASWBCB_HERE, "m");
    CheckEndsWithIC(text, L"HELLO", ASWBCB_HERE, "m");
    CheckNotContains(text, "lo W", ASWBCB_HERE, "m");
    CheckNotContainsIC(text, L"LO w", ASWBCB_HERE, "m");
    CheckNotEndsWith(text, "World", ASWBCB_HERE, "m");
    CheckNotEndsWithIC(text, L"WORLD", ASWBCB_HERE, "m");
    CheckNotStartsWith(text, "Hello", ASWBCB_HERE, "m");
    CheckNotStartsWithIC(text, L"HELLO", ASWBCB_HERE, "m");
    CheckStartsWith(text, "World", ASWBCB_HERE, "m");
    CheckStartsWithIC(text, L"WORLD", ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_TextParts::Test_Passes()
{
    std::string const text = "Hello World";
    char const* const nullString = nullptr;

    CheckContains(text, "lo W", ASWBCB_HERE);
    CheckContainsIC(text, "LO w", ASWBCB_HERE);
    CheckEndsWith(text, "World", ASWBCB_HERE);
    CheckEndsWithIC(text, "WORLD", ASWBCB_HERE);
    CheckNotContains(text, "xyz", ASWBCB_HERE);
    CheckNotContainsIC(text, "XYZ", ASWBCB_HERE);
    CheckNotEndsWith(text, "Hello", ASWBCB_HERE);
    CheckNotEndsWithIC(text, "HELLO", ASWBCB_HERE);
    CheckNotStartsWith(text, "World", ASWBCB_HERE);
    CheckNotStartsWithIC(text, "WORLD", ASWBCB_HERE);
    CheckStartsWith(text, "Hello", ASWBCB_HERE);
    CheckStartsWithIC(text, "HELLO", ASWBCB_HERE);
    AssertContains(L"Hello World", std::wstring(L"lo W"), ASWBCB_HERE);
    AssertContainsIC(L"Hello World", std::wstring(L"LO w"), ASWBCB_HERE);
    AssertEndsWith(L"Hello World", std::wstring(L"World"), ASWBCB_HERE);
    AssertEndsWithIC(L"Hello World", std::wstring(L"WORLD"), ASWBCB_HERE);
    AssertNotContains(L"Hello World", std::wstring(L"xyz"), ASWBCB_HERE);
    AssertNotContainsIC(L"Hello World", std::wstring(L"XYZ"), ASWBCB_HERE);
    AssertNotEndsWith(L"Hello World", std::wstring(L"Hello"), ASWBCB_HERE);
    AssertNotEndsWithIC(L"Hello World", std::wstring(L"HELLO"), ASWBCB_HERE);
    AssertNotStartsWith(L"Hello World", std::wstring(L"World"), ASWBCB_HERE);
    AssertNotStartsWithIC(L"Hello World", std::wstring(L"WORLD"), ASWBCB_HERE);
    AssertStartsWith(L"Hello World", std::wstring(L"Hello"), ASWBCB_HERE);
    AssertStartsWithIC(L"Hello World", std::wstring(L"HELLO"), ASWBCB_HERE);
    CheckContains(text, "", ASWBCB_HERE, "an empty substring");
    CheckNotContains(nullString, "", ASWBCB_HERE, "a null text");
    CheckNotStartsWith(text, nullString, ASWBCB_HERE, "a null prefix");
}


/////////////////////////////////////////////////////////////////////////////
// TInnerTB_Throws: Throws and NoThrow, with each kind of callable
/////////////////////////////////////////////////////////////////////////////

// Callables of each kind other than a closure. Functor classes must be at namespace scope (bcc32).
void InnerTB_Fault()
{
    int* volatile pointer = nullptr;
    *pointer = 1;
}

void InnerTB_ThrowInt()
{
    throw 42;
}

struct TInnerTB_FaultFunctor
{
    void operator()() const
    {
        int* volatile pointer = nullptr;
        *pointer = 1;
    }
};

void InnerTB_ThrowNothing()
{
}

struct TInnerTB_ThrowIntFunctor
{
    void operator()()
    {
        throw 7;
    }
};

struct TInnerTB_ThrowNothingFunctor
{
    void operator()() const
    {
    }
};

struct TInnerTB_ThrowOutOfRangeFunctor
{
    void operator()() const
    {
        throw std::out_of_range("index");
    }
};

// An object other than the group, for closures to its methods.
class TInnerTB_Thrower : public TObject
{
public:
    void __fastcall Throw()
    {
        throw std::runtime_error("thrower");
    }
};

class TInnerTB_Throws : public TTestGroupBase
{
public:
    std::string Trace;
    int FirstCheckLine;
    std::map<std::string, int> Lines; // The line of each test's failing assertion, by test name

    __fastcall TInnerTB_Throws()
        : TTestGroupBase("InnerThrows"),
          FirstCheckLine(0)
    {
    }

    void __fastcall AssertFalseInside()
    {
        Lines["Inner"] = __LINE__ + 1;
        AssertTrue(false, ASWBCB_HERE, "inner");
    }

    void __fastcall ConvertInvalid()
    {
        StrToInt(L"x");
    }

    void __fastcall DoNothing()
    {
    }

    void __fastcall FailInside()
    {
        Fail(ASWBCB_HERE, "inner");
    }

    void __fastcall FaultInside()
    {
        int* volatile pointer = nullptr;
        *pointer = 1;
    }

    int __fastcall GetValueThrows(); // Defined after the class: bcc32 W8027 for an inline function without a return

    void __fastcall SkipInside()
    {
        Skip(ASWBCB_HERE, "inner");
    }

    void __fastcall ThrowConvertError()
    {
        throw EConvertError(L"bad");
    }

    void __fastcall ThrowRuntimeError()
    {
        throw std::runtime_error("boom");
    }

__published:
    void __fastcall Test_AccessViolations();
    void __fastcall Test_AccessViolations_Fail();
    void __fastcall Test_AssertInside_PassesThrough();
    void __fastcall Test_AssertNoThrow_Fails();
    void __fastcall Test_AssertThrows_Fails();
    void __fastcall Test_Checks_Fail();
    void __fastcall Test_Passes();
    void __fastcall Test_SkipInside_PassesThrough();
};

int __fastcall TInnerTB_Throws::GetValueThrows()
{
    throw std::runtime_error("value");
}

// An access violation in each kind of callable is the exception the checks find: bcc64x missed one in a functor
// (CheckNoThrow() ended the process, even unoptimized), until the callable was called out of line.
void __fastcall TInnerTB_Throws::Test_AccessViolations()
{
    CheckThrows<EAccessViolation>(TInnerTB_FaultFunctor(), ASWBCB_HERE, "a functor");
    CheckThrows<EAccessViolation>(&InnerTB_Fault, ASWBCB_HERE, "a function");
    CheckThrows<EAccessViolation>(FaultInside, ASWBCB_HERE, "a closure");
    AssertThrows<EAccessViolation>(TInnerTB_FaultFunctor(), ASWBCB_HERE, "a functor");
}

void __fastcall TInnerTB_Throws::Test_AccessViolations_Fail()
{
    Lines["AccessViolations_Fail"] = __LINE__ + 1;
    CheckNoThrow(TInnerTB_FaultFunctor(), ASWBCB_HERE, "m");
    CheckThrows<EConvertError>(TInnerTB_FaultFunctor(), ASWBCB_HERE, "m");
    CheckNoThrow(&InnerTB_Fault, ASWBCB_HERE, "m");
    CheckNoThrow(FaultInside, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Throws::Test_AssertInside_PassesThrough()
{
    CheckThrows<std::exception>(&AssertFalseInside, ASWBCB_HERE, "a failed Assert* isn't the exception checked for");
    Trace += "AfterInnerAssert;";
}

void __fastcall TInnerTB_Throws::Test_AssertNoThrow_Fails()
{
    Lines["AssertNoThrow_Fails"] = __LINE__ + 1;
    AssertNoThrow(ThrowRuntimeError, ASWBCB_HERE, "m");
    Trace += "AfterAssertNoThrow;";
}

void __fastcall TInnerTB_Throws::Test_AssertThrows_Fails()
{
    Lines["AssertThrows_Fails"] = __LINE__ + 1;
    AssertThrows<EConvertError>(&DoNothing, ASWBCB_HERE, "m");
    Trace += "AfterAssertThrows;";
}

void __fastcall TInnerTB_Throws::Test_Checks_Fail()
{
    FirstCheckLine = __LINE__ + 1;
    CheckThrows<std::runtime_error>(DoNothing, ASWBCB_HERE, "m");
    CheckThrows<std::runtime_error>(&ThrowConvertError, ASWBCB_HERE, "m");
    CheckThrows<std::runtime_error>(TInnerTB_ThrowIntFunctor(), ASWBCB_HERE, "m");
    CheckThrows<EConvertError>(&ThrowRuntimeError, ASWBCB_HERE, "m");
    CheckThrows<std::runtime_error>(&ThrowRuntimeError, ASWBCB_HERE, "m", "bang");
    CheckThrows<EConvertError>(ThrowConvertError, ASWBCB_HERE, "m", "zzz");
    CheckThrows<int>(&InnerTB_ThrowInt, ASWBCB_HERE, "m", "x");
    CheckNoThrow(&ThrowConvertError, ASWBCB_HERE, "m");
    CheckNoThrow(TInnerTB_ThrowIntFunctor(), ASWBCB_HERE, "m");
    CheckNoThrow(ThrowRuntimeError, ASWBCB_HERE, "m");
}

void __fastcall TInnerTB_Throws::Test_Passes()
{
    std::unique_ptr<TInnerTB_Thrower> thrower(new TInnerTB_Thrower());

    CheckThrows<EConvertError>(ConvertInvalid, ASWBCB_HERE, "a closure");
    CheckThrows<Exception>(&ConvertInvalid, ASWBCB_HERE, "a closure with &, a base class");
    CheckThrows<std::logic_error>(TInnerTB_ThrowOutOfRangeFunctor(), ASWBCB_HERE, "a functor, a base class");
    CheckThrows<std::runtime_error>(&ThrowRuntimeError, ASWBCB_HERE, "a message", "oo");
    CheckThrows<EConvertError>(&ThrowConvertError, ASWBCB_HERE, "an RTL message", "ba");
    CheckThrows<int>(&InnerTB_ThrowInt, ASWBCB_HERE, "a function pointer, an int");
    CheckThrows<int>(InnerTB_ThrowInt, ASWBCB_HERE, "a function");
    CheckThrows<std::runtime_error>(&GetValueThrows, ASWBCB_HERE, "a method returning a value");
    CheckThrows<std::runtime_error>(thrower->Throw, ASWBCB_HERE, "a closure to another object");
    CheckThrows<std::runtime_error>(&thrower->Throw, ASWBCB_HERE, "a closure to another object, with &");
    CheckThrows<TExceptTrue>(&AssertFalseInside, ASWBCB_HERE, "a failed Assert* asked for");
    CheckThrows<TTestException>(&FailInside, ASWBCB_HERE, "Fail() asked for");
    CheckNoThrow(DoNothing, ASWBCB_HERE);
    CheckNoThrow(&DoNothing, ASWBCB_HERE);
    CheckNoThrow(TInnerTB_ThrowNothingFunctor(), ASWBCB_HERE);
    CheckNoThrow(&InnerTB_ThrowNothing, ASWBCB_HERE);
    AssertNoThrow(DoNothing, ASWBCB_HERE);
    AssertNoThrow(TInnerTB_ThrowNothingFunctor(), ASWBCB_HERE);
    AssertThrows<EConvertError>(ConvertInvalid, ASWBCB_HERE);
    AssertThrows<int>(TInnerTB_ThrowIntFunctor(), ASWBCB_HERE);
}

void __fastcall TInnerTB_Throws::Test_SkipInside_PassesThrough()
{
    CheckNoThrow(&SkipInside, ASWBCB_HERE, "Skip() isn't an exception the check finds");
    Trace += "AfterInnerSkip;";
}


/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_TestBase
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_TestBase::TTest_ASWUnitTestsBCB_TestBase()
    : inherited("ASWUnitTestsBCB_TestBase_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Asserts_EndTheTest()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Checks> group(new TInnerTB_Checks());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("fail", OutcomeOf(results, "AssertFails"), ASWBCB_HERE);
    CheckTrue(StartsWith(MessageOf(results, "AssertFails"), "Values not equal: Test_AssertFails ("), ASWBCB_HERE,
        MessageOf(results, "AssertFails"));
    CheckTrue(Contains(MessageOf(results, "AssertFails"), "): Expected: \"a\" but was \"b\". "), ASWBCB_HERE);
    CheckFalse(Contains(group->Trace, "AfterAssert;"), ASWBCB_HERE, "nothing after the failed assert ran");
    CheckEquals("pass", OutcomeOf(results, "ChecksPass"), ASWBCB_HERE, MessageOf(results, "ChecksPass"));
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Booleans_ChecksAndAsserts()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Booleans> group(new TInnerTB_Booleans());
    group->SetRunObserver(&observer);

    // Each failing Assert* test, and its message's title.
    char const* const asserts[][2] = {
        { "AssertFalse_Fails", "Expected false but was true" },
        { "AssertTrue_Fails", "Expected true but was false" },
    };

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();

    for (std::size_t i = 0; i < sizeof(asserts) / sizeof(asserts[0]); ++i)
    {
        std::string const testName = asserts[i][0];

        CheckEquals("fail", OutcomeOf(results, testName), ASWBCB_HERE, testName);
        CheckEquals(std::string(asserts[i][1]) + ": Test_" + testName + " (" + IntToString(group->Lines[testName]) +
            "): m", MessageOf(results, testName), ASWBCB_HERE, testName);
    }

    CheckEquals("", group->Trace, ASWBCB_HERE, "nothing after a failed assert ran");
    CheckEquals("Check failed for: \"Test_Checks_Fail\" (" + IntToString(group->FirstCheckLine) +
        "): Expected false but was true: \"m\"\nCheck failed for: \"Test_Checks_Fail\" (" +
        IntToString(group->FirstCheckLine + 1) + "): Expected true but was false: \"m\"",
        MessageOf(results, "Checks_Fail"), ASWBCB_HERE, "each Check* form fails");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Checks_FailTheTestAtTheEnd()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Checks> group(new TInnerTB_Checks());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert: the first with an Assert* method, which fails this test by throwing, so it still fails if failed Check*
    // calls stopped failing tests, which would make every Check* in the suite, this test's others included, pass.
    TTestResults const& results = group->Results();
    AssertEquals("fail", OutcomeOf(results, "TwoChecksFail"), ASWBCB_HERE);
    CheckTrue(Contains(group->Trace, "AfterChecks;"), ASWBCB_HERE, "the test went on after its failed checks");

    CheckEquals("fail", OutcomeOf(results, "CheckThenFail"), ASWBCB_HERE);
    CheckTrue(Contains(MessageOf(results, "CheckThenFail"), "Expected \"1\" but was \"2\". \nFailed: "), ASWBCB_HERE,
        "the record has the check failure, then Fail()'s: " + MessageOf(results, "CheckThenFail"));

    CheckEquals("skip", OutcomeOf(results, "CheckThenSkip"), ASWBCB_HERE, "Skip() still skips after a check failed");
    CheckEquals("fail", OutcomeOf(results, "CheckThenExpectedException"), ASWBCB_HERE,
        "the expected exception came, but a check failed");
    CheckEquals(4u, results.FailedCount, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Checks_LogAndRecordTheirMessages()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Checks> group(new TInnerTB_Checks());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    std::string const first = "Check failed for: \"Test_TwoChecksFail\" (" + IntToString(group->FirstCheckLine) +
        "): Expected \"1\" but was \"2\". a";
    std::string const second = "Check failed for: \"Test_TwoChecksFail\" (" + IntToString(group->SecondCheckLine) +
        "): Expected true but was false: \"b\"";

    CheckEquals(first + "\n" + second, MessageOf(group->Results(), "TwoChecksFail"), ASWBCB_HERE,
        "the record lists both, with the bare method name on every compiler");
    CheckTrue(Contains(observer.LogText, "  **" + first + "\n  **" + second + "\n***Test failed: "
        "\"InnerChecks.TwoChecksFail\"\nFinished test: \"InnerChecks.TwoChecksFail\" - failed ("), ASWBCB_HERE,
        "each is logged as it happens, and the failed line doesn't repeat them");
    CheckEquals("  **" + first, group->Results().Messages[0], ASWBCB_HERE, "and added to the group's messages");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Empty_ChecksAndAsserts()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Emptiness> group(new TInnerTB_Emptiness());
    group->SetRunObserver(&observer);

    // Each failing Assert* test, and the text after its location.
    char const* const asserts[][3] = {
        { "AssertEmpty_Fails", "Not empty", "Expected empty but was \"abc\". m" },
        { "AssertNotEmpty_Fails", "Empty", "Expected not empty but was empty. m" },
    };

    // The failing Check* calls, in order, on consecutive lines.
    char const* const checks[] = {
        "Expected empty but had 3 elements. m",
        "Expected empty but was \"(null)\". m",
        "Expected empty but was \"x\". m",
        "Expected not empty but was empty. m",
        "Expected not empty but was empty. m",
        "Expected not empty but was \"(null)\". m",
    };

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "Passes"), ASWBCB_HERE, MessageOf(results, "Passes"));

    for (std::size_t i = 0; i < sizeof(asserts) / sizeof(asserts[0]); ++i)
    {
        std::string const testName = asserts[i][0];

        CheckEquals("fail", OutcomeOf(results, testName), ASWBCB_HERE, testName);
        CheckEquals(std::string(asserts[i][1]) + ": Test_" + testName + " (" + IntToString(group->Lines[testName]) +
            "): " + asserts[i][2], MessageOf(results, testName), ASWBCB_HERE, testName);
    }

    CheckEquals("", group->Trace, ASWBCB_HERE, "nothing after a failed assert ran");

    std::string expectedChecks;

    for (std::size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i)
    {
        expectedChecks += std::string((i == 0) ? "" : "\n") + "Check failed for: \"Test_Checks_Fail\" (" +
            IntToString(group->FirstCheckLine + static_cast<int>(i)) + "): " + checks[i];
    }

    CheckEquals(expectedChecks, MessageOf(results, "Checks_Fail"), ASWBCB_HERE, "each Check* form fails");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Equals_EnumsAndPointers()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Equals> group(new TInnerTB_Equals());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("fail", OutcomeOf(results, "AssertEquals_Enums_Fails"), ASWBCB_HERE);
    CheckEquals("Values not equal: Test_AssertEquals_Enums_Fails (" +
        IntToString(group->Lines["AssertEquals_Enums_Fails"]) + "): Expected: \"0\" but was \"1\". enums",
        MessageOf(results, "AssertEquals_Enums_Fails"), ASWBCB_HERE, "enums are shown as numbers");
    CheckFalse(Contains(group->Trace, "AfterAssertEquals;"), ASWBCB_HERE, "nothing after the failed assert ran");

    CheckEquals("fail", OutcomeOf(results, "CheckEquals_Pointers_Fails"), ASWBCB_HERE);
    CheckEquals("Check failed for: \"Test_CheckEquals_Pointers_Fails\" (" +
        IntToString(group->Lines["CheckEquals_Pointers_Fails"]) + "): Expected \"" + Address(&group->First) +
        "\" but was \"" + Address(&group->Second) + "\". pointers", MessageOf(results, "CheckEquals_Pointers_Fails"),
        ASWBCB_HERE, "pointers are compared and shown by address");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_EqualsIC_ChecksAndAsserts()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Equals> group(new TInnerTB_Equals());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "EqualsIC_Passes"), ASWBCB_HERE, MessageOf(results, "EqualsIC_Passes"));

    CheckEquals("Values not equal: Test_AssertEqualsIC_Fails (" + IntToString(group->Lines["AssertEqualsIC_Fails"]) +
        "): Expected: \"abc\" but was \"ABD\" (ignoring case). ic", MessageOf(results, "AssertEqualsIC_Fails"),
        ASWBCB_HERE);
    CheckFalse(Contains(group->Trace, "AfterAssertEqualsIC;"), ASWBCB_HERE, "nothing after the failed assert ran");

    CheckEquals("Values are equal: Test_AssertNotEqualsIC_Fails (" +
        IntToString(group->Lines["AssertNotEqualsIC_Fails"]) + "): Values: \"abc\" and \"ABC\" (ignoring case). ic",
        MessageOf(results, "AssertNotEqualsIC_Fails"), ASWBCB_HERE, "both values are shown");
    CheckFalse(Contains(group->Trace, "AfterAssertNotEqualsIC;"), ASWBCB_HERE, "nothing after the failed assert ran");

    CheckEquals("Check failed for: \"Test_CheckEqualsIC_Fails\" (" + IntToString(group->Lines["CheckEqualsIC_Fails"]) +
        "): Expected \"abc\" but was \"abd\" (ignoring case). ic", MessageOf(results, "CheckEqualsIC_Fails"),
        ASWBCB_HERE);
    CheckTrue(Contains(group->Trace, "AfterCheckEqualsIC;"), ASWBCB_HERE, "the test went on after the check");

    CheckEquals("Check failed for: \"Test_CheckNotEqualsIC_Fails\" (" +
        IntToString(group->Lines["CheckNotEqualsIC_Fails"]) + "): Both values equal: \"abc\" and \"ABC\" (ignoring "
        "case). ic", MessageOf(results, "CheckNotEqualsIC_Fails"), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Exceptions_ExpectedAnyType()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Exceptions> group(new TInnerTB_Exceptions());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "Any_Std"), ASWBCB_HERE);
    CheckEquals("pass", OutcomeOf(results, "Any_RTL"), ASWBCB_HERE);
    CheckEquals("pass", OutcomeOf(results, "Any_Other"), ASWBCB_HERE);
    CheckEquals("pass", OutcomeOf(results, "Any_Cancelled"), ASWBCB_HERE, "SetExceptionExpected(false) cancels");
    CheckEquals("pass", OutcomeOf(results, "Any_NotCarriedOver"), ASWBCB_HERE, "each test starts expecting none");
    CheckEquals("Exception expected: Test_Any_None (" + IntToString(group->NoneLine) + "): why",
        MessageOf(results, "Any_None"), ASWBCB_HERE, "none came");
    CheckEquals("Exception expected: Test_WideMessage (" + IntToString(group->WideMessageLine) + "): caf\xC3\xA9",
        MessageOf(results, "WideMessage"), ASWBCB_HERE, "a UnicodeString message");
    CheckEquals("fail", OutcomeOf(results, "FailWins"), ASWBCB_HERE, "Fail() isn't taken for the exception");
    CheckEquals("skip", OutcomeOf(results, "SkipWins"), ASWBCB_HERE, "nor is Skip()");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Exceptions_ExpectedMessage()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Exceptions> group(new TInnerTB_Exceptions());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "Message_RTL_Match"), ASWBCB_HERE);
    CheckEquals("expected exception message to contain \"nope\" but caught: EConvertError: 'x' is not a valid "
        "integer value", MessageOf(results, "Message_RTL_Wrong"), ASWBCB_HERE);
    CheckEquals("pass", OutcomeOf(results, "Message_Std_Match"), ASWBCB_HERE);
    CheckEquals("expected exception message to contain \"zzz\" but caught: a boom",
        MessageOf(results, "Message_Std_Wrong"), ASWBCB_HERE);
    CheckEquals("expected exception message to contain \"x\" but caught an exception without a message: an "
        "object that is neither a std::exception nor an RTL Exception", MessageOf(results, "Message_Other"),
        ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Exceptions_ExpectedType()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Exceptions> group(new TInnerTB_Exceptions());
    group->SetRunObserver(&observer);
    std::string const wrongType = "expected exception type was not thrown (caught a different exception): ";
    std::string const convertError = "EConvertError: 'x' is not a valid integer value";
    std::string const unknown = "an object that is neither a std::exception nor an RTL Exception";

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "RTL_Match"), ASWBCB_HERE);
    CheckEquals("pass", OutcomeOf(results, "RTL_Base"), ASWBCB_HERE, "a base class matches its descendants");
    CheckEquals(wrongType + convertError, MessageOf(results, "RTL_Wrong"), ASWBCB_HERE);
    CheckEquals(wrongType + "s", MessageOf(results, "RTL_GotStd"), ASWBCB_HERE);
    CheckEquals("pass", OutcomeOf(results, "Std_Match"), ASWBCB_HERE);
    CheckEquals("pass", OutcomeOf(results, "Std_Base"), ASWBCB_HERE);
    CheckEquals(wrongType + "r", MessageOf(results, "Std_Wrong"), ASWBCB_HERE);
    CheckEquals(wrongType + convertError, MessageOf(results, "Std_GotRTL"), ASWBCB_HERE);
    CheckEquals("pass", OutcomeOf(results, "Other_Match"), ASWBCB_HERE, "a type that isn't a std::exception");
    CheckEquals("pass", OutcomeOf(results, "Other_Derived"), ASWBCB_HERE);
    CheckEquals(wrongType + unknown, MessageOf(results, "Other_Wrong"), ASWBCB_HERE);
    CheckEquals(wrongType + "s", MessageOf(results, "Other_GotStd"), ASWBCB_HERE);
    CheckEquals("pass", OutcomeOf(results, "Other_Int"), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Exceptions_Unexpected()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Exceptions> group(new TInnerTB_Exceptions());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("Unexpected exception: std boom", MessageOf(results, "Unexpected_Std"), ASWBCB_HERE);
    CheckEquals("Unexpected exception: EConvertError: 'x' is not a valid integer value",
        MessageOf(results, "Unexpected_RTL"), ASWBCB_HERE);
    CheckEquals("Unexpected exception: an object that is neither a std::exception nor an RTL Exception",
        MessageOf(results, "Unexpected_Other"), ASWBCB_HERE);
    CheckTrue(StartsWith(MessageOf(results, "Unexpected_AccessViolation"), "Unexpected exception: EAccessViolation: "),
        ASWBCB_HERE, MessageOf(results, "Unexpected_AccessViolation"));
    CheckTrue(Contains(group->Trace, "TDT:Unexpected_Std;TDT:Unexpected_RTL;TDT:Unexpected_Other;"
        "TDT:Unexpected_AccessViolation;"), ASWBCB_HERE, "TearDown_Test() still ran each time");
    CheckEquals(31u, results.SuccessCount + results.FailedCount + results.SkippedCount, ASWBCB_HERE,
        "the run went on after each");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_FailAndSkip()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Lifecycle> group(new TInnerTB_Lifecycle());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    CheckEquals("Failed: Test_Fail (" + IntToString(group->FailLine) + "): failing on purpose",
        MessageOf(group->Results(), "Fail"), ASWBCB_HERE);
    CheckEquals("Test skipped: Test_Skip (" + IntToString(group->SkipLine) + "): skipping on purpose",
        MessageOf(group->Results(), "Skip"), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_FindPublishedTests_DeclarationOrderAndPrefix()
{
    CheckEquals("Zeta,Virtual,Alpha", NamesOf(FindPublishedTests(__classid(TInnerTB_Base))), ASWBCB_HERE,
        "declaration order; OnClick and a bare Test_ aren't tests");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_FindPublishedTests_DuplicateNameThrows()
{
    // Arrange
    std::string message = "(no exception)";
    std::unique_ptr<TInnerTB_Duplicate> group(new TInnerTB_Duplicate());

    // Act
    try
    {
        FindPublishedTests(__classid(TInnerTB_Duplicate));
    }
    catch (TExceptDuplicateTestName const& duplicateEx)
    {
        message = duplicateEx.what();
    }

    // Assert
    CheckEquals("Test group class \"TInnerTB_Duplicate\" publishes more than one method named \"Test_Same\"", message,
        ASWBCB_HERE);

    SetExceptionExpected<TExceptDuplicateTestName>(ASWBCB_HERE, "GetTestCallbackList() throws it too");
    group->GetTestCallbackList();
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_FindPublishedTests_InheritedFirstAndOverrides()
{
    std::vector<TPublishedTest> const tests = FindPublishedTests(__classid(TInnerTB_Derived));

    CheckEquals("Zeta,Virtual,Alpha,Own,Beta", NamesOf(tests), ASWBCB_HERE,
        "the ancestor's tests first; a test published again keeps its position");
    AssertEquals(5u, static_cast<unsigned int>(tests.size()), ASWBCB_HERE);
    CheckEquals("Test_Own", tests[3].MethodName, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_FindPublishedTests_NoPublishedMethods()
{
    CheckEquals("", NamesOf(FindPublishedTests(__classid(TInnerTB_Empty))), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_GetTestCallbackList_RunsMostDerivedCode()
{
    // Arrange
    std::unique_ptr<TInnerTB_Derived> group(new TInnerTB_Derived());
    TTestGroupBase::TestCallbackList const& tests = group->GetTestCallbackList();

    // Act
    for (TTestGroupBase::TestCallbackList::const_iterator it = tests.begin(); it != tests.end(); ++it)
        (*it)->DoTest();

    // Assert
    CheckTrue(&tests == &group->GetTestCallbackList(), ASWBCB_HERE, "found once");
    CheckEquals("Base.Zeta;Derived.Virtual;Derived.Alpha;Derived.Own;Derived.Beta;", group->Trace, ASWBCB_HERE,
        "an override or a hiding method runs in the ancestor's place");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Matches_ChecksAndAsserts()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Matches> group(new TInnerTB_Matches());
    group->SetRunObserver(&observer);

    // Each failing Assert* test, and the text after its location.
    char const* const asserts[][3] = {
        { "AssertMatches_Fails", "Pattern not matched", "Expected \"2026-1-7\" to match \"\\d{4}-\\d{2}-\\d{2}\". m" },
        { "AssertNotMatches_Fails", "Pattern matched", "Expected \"abc123\" not to match \"[a-z]+\\d+\". m" },
    };

    // The failing Check* calls, in order, on consecutive lines. The RTL describes an invalid pattern (see
    // Values.RegularExpressions).
    std::string const checks[] = {
        "Expected \"Error 42\" to match \"Error \\d\". m",
        TextMatchFailure("abc", "[a-", true) + ". m",
        "Expected \"x\" not to match \".\". m",
        TextMatchFailure("abc", "a(", false) + ". m",
    };

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "Passes"), ASWBCB_HERE, MessageOf(results, "Passes"));

    for (std::size_t i = 0; i < sizeof(asserts) / sizeof(asserts[0]); ++i)
    {
        std::string const testName = asserts[i][0];

        CheckEquals("fail", OutcomeOf(results, testName), ASWBCB_HERE, testName);
        CheckEquals(std::string(asserts[i][1]) + ": Test_" + testName + " (" + IntToString(group->Lines[testName]) +
            "): " + asserts[i][2], MessageOf(results, testName), ASWBCB_HERE, testName);
    }

    CheckEquals("", group->Trace, ASWBCB_HERE, "nothing after a failed assert ran");
    CheckTrue(StartsWith(checks[1], "Invalid regular expression \"[a-\": "), ASWBCB_HERE, checks[1]);
    CheckTrue(StartsWith(checks[3], "Invalid regular expression \"a(\": "), ASWBCB_HERE, checks[3]);

    std::string expectedChecks;

    for (std::size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i)
    {
        expectedChecks += std::string((i == 0) ? "" : "\n") + "Check failed for: \"Test_Checks_Fail\" (" +
            IntToString(group->FirstCheckLine + static_cast<int>(i)) + "): " + checks[i];
    }

    CheckEquals(expectedChecks, MessageOf(results, "Checks_Fail"), ASWBCB_HERE, "each Check* form fails");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Memory_ChecksAndAsserts()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Memory> group(new TInnerTB_Memory());
    group->SetRunObserver(&observer);

    // Each failing Assert* test, and the text after its location.
    char const* const asserts[][3] = {
        { "AssertEqualsMem_Fails", "Memory not equal",
          "Bytes differ at offset 2 of 4: expected \"03 04\" but was \"FF 04\". m" },
        { "AssertNotEqualsMem_Fails", "Memory equal", "Expected the 4 bytes to differ, but both are \"DE AD BE EF\". m" },
    };

    // The failing Check* calls, in order, on consecutive lines (ints are little-endian on every target).
    char const* const checks[] = {
        "Cannot compare 8 bytes: actual is null. m",
        "Bytes differ at offset 2 of 3: expected \"63\" but was \"64\". m",
        "Expected the 8 bytes to differ, but both are \"01 00 00 00 02 00 00 00\". m",
    };

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "Passes"), ASWBCB_HERE, MessageOf(results, "Passes"));

    for (std::size_t i = 0; i < sizeof(asserts) / sizeof(asserts[0]); ++i)
    {
        std::string const testName = asserts[i][0];

        CheckEquals("fail", OutcomeOf(results, testName), ASWBCB_HERE, testName);
        CheckEquals(std::string(asserts[i][1]) + ": Test_" + testName + " (" + IntToString(group->Lines[testName]) +
            "): " + asserts[i][2], MessageOf(results, testName), ASWBCB_HERE, testName);
    }

    CheckEquals("", group->Trace, ASWBCB_HERE, "nothing after a failed assert ran");

    std::string expectedChecks;

    for (std::size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i)
    {
        expectedChecks += std::string((i == 0) ? "" : "\n") + "Check failed for: \"Test_Checks_Fail\" (" +
            IntToString(group->FirstCheckLine + static_cast<int>(i)) + "): " + checks[i];
    }

    CheckEquals(expectedChecks, MessageOf(results, "Checks_Fail"), ASWBCB_HERE, "each Check* form fails");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Near_ChecksAndAsserts()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Near> group(new TInnerTB_Near());
    group->SetRunObserver(&observer);

    // Each failing test, the start of its record, and the text after its location.
    char const* const records[][3] = {
        { "AssertNear_Fails", "Values not equal: Test_AssertNear_Fails (",
          "Expected: \"1.000000 (tolerance 0.125000)\" but was \"1.250000 (diff 0.250000)\". m" },
        { "AssertNear_NegativeTolerance_Fails", "Values not equal: Test_AssertNear_NegativeTolerance_Fails (",
          "Invalid tolerance -0.500000: it must be zero or more. m" },
        { "AssertNotNear_Fails", "Values are equal: Test_AssertNotNear_Fails (",
          "Value: \"1.250000 (diff 0.250000 <= tolerance 0.500000)\". m" },
        { "AssertNotNear_NaNTolerance_Fails", "Values are equal: Test_AssertNotNear_NaNTolerance_Fails (",
          "Invalid tolerance nan: it must be zero or more. m" },
        { "CheckNear_Fails", "Check failed for: \"Test_CheckNear_Fails\" (",
          "Expected \"1.000000 (tolerance 0.125000)\" but was \"1.250000 (diff 0.250000)\". m" },
        { "CheckNear_Float_Fails", "Check failed for: \"Test_CheckNear_Float_Fails\" (",
          "Expected \"2.500000 (tolerance 0.250000)\" but was \"3.000000 (diff 0.500000)\". m" },
        { "CheckNear_Float_NegativeTolerance_Fails",
          "Check failed for: \"Test_CheckNear_Float_NegativeTolerance_Fails\" (",
          "Invalid tolerance -0.250000: it must be zero or more. m" },
        { "CheckNear_NaN_Fails", "Check failed for: \"Test_CheckNear_NaN_Fails\" (",
          "Expected \"nan (tolerance 0.500000)\" but was \"1.000000 (diff nan)\". m" },
        { "CheckNear_NaNTolerance_Fails", "Check failed for: \"Test_CheckNear_NaNTolerance_Fails\" (",
          "Invalid tolerance nan: it must be zero or more. m" },
        { "CheckNotNear_Fails", "Check failed for: \"Test_CheckNotNear_Fails\" (",
          "Both values equal: \"1.250000 (diff 0.250000 <= tolerance 0.500000)\". m" },
        { "CheckNotNear_Float_Fails", "Check failed for: \"Test_CheckNotNear_Float_Fails\" (",
          "Both values equal: \"1.000000 (diff 0.000000 <= tolerance 0.000000)\". m" },
        { "CheckNotNear_NegativeTolerance_Fails", "Check failed for: \"Test_CheckNotNear_NegativeTolerance_Fails\" (",
          "Invalid tolerance -1.000000: it must be zero or more. m" },
    };

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "Passes"), ASWBCB_HERE, MessageOf(results, "Passes"));

    for (std::size_t i = 0; i < sizeof(records) / sizeof(records[0]); ++i)
    {
        std::string const testName = records[i][0];

        CheckEquals("fail", OutcomeOf(results, testName), ASWBCB_HERE, testName);
        CheckEquals(records[i][1] + IntToString(group->Lines[testName]) + "): " + records[i][2],
            MessageOf(results, testName), ASWBCB_HERE, testName);
    }

    CheckEquals("AfterCheckNear;AfterCheckNearNegativeTolerance;", group->Trace, ASWBCB_HERE,
        "a Check goes on, an Assert doesn't");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_NotEquals_ChecksAndAsserts()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Equals> group(new TInnerTB_Equals());
    group->SetRunObserver(&observer);

    // Each failing CheckNotEquals() test, and the text after its location.
    char const* const checks[][2] = {
        { "CheckNotEquals_Chars_Fails", "Both values equal: \"97\". chars" },
        { "CheckNotEquals_Enums_Fails", "Both values equal: \"1\". an enum and an int" },
        { "CheckNotEquals_Integers_Fails", "Both values equal: \"7\". integers" },
        { "CheckNotEquals_NullStrings_Fails", "Both values equal: \"(null)\". " },
    };

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "NotEquals_Passes"), ASWBCB_HERE, MessageOf(results, "NotEquals_Passes"));

    for (std::size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i)
    {
        std::string const testName = checks[i][0];
        CheckEquals("fail", OutcomeOf(results, testName), ASWBCB_HERE, testName);
        CheckEquals("Check failed for: \"Test_" + testName + "\" (" + IntToString(group->Lines[testName]) + "): " +
            checks[i][1], MessageOf(results, testName), ASWBCB_HERE, testName);
    }

    CheckTrue(Contains(group->Trace, "AfterCheckNotEquals;"), ASWBCB_HERE, "the test went on after the check");
    CheckEquals("Check failed for: \"Test_CheckNotEquals_Pointers_Fails\" (" +
        IntToString(group->Lines["CheckNotEquals_Pointers_Fails"]) + "): Both values equal: \"" +
        Address(&group->First) + "\". pointers", MessageOf(results, "CheckNotEquals_Pointers_Fails"), ASWBCB_HERE);

    CheckEquals("fail", OutcomeOf(results, "AssertNotEquals_Fails"), ASWBCB_HERE);
    CheckEquals("Values are equal: Test_AssertNotEquals_Fails (" + IntToString(group->Lines["AssertNotEquals_Fails"]) +
        "): Value: \"7\". seven", MessageOf(results, "AssertNotEquals_Fails"), ASWBCB_HERE);
    CheckFalse(Contains(group->Trace, "AfterAssertNotEquals;"), ASWBCB_HERE, "nothing after the failed assert ran");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Objects_ChecksAndAsserts()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Objects> group(new TInnerTB_Objects());
    group->SetRunObserver(&observer);

    std::string const circle = DescribeTypeInfo(typeid(TInnerTB_Circle));
    std::string const square = DescribeTypeInfo(typeid(TInnerTB_Square));
    std::string const first = Address(&group->First);
    std::string const second = Address(&group->Second);
    std::string const values = Address(&group->Values[0]);

    // Each failing test, and the text after its location: "Test_<test> (<line>): " for an Assert* (after the
    // message's title), or "\"Test_<test>\" (<line>): " for a Check*.
    std::string const records[][3] = {
        { "AssertIsNotType_Fails", "Type matched: ", "Expected not type \"" + circle + "\" but was \"" + circle +
          "\". m" },
        { "AssertIsType_Fails", "Type mismatch: ", "Expected type \"" + square + "\" but was \"" + circle + "\". m" },
        { "AssertNotNull_Fails", "Expected not null but was null: ", "m" },
        { "AssertNotSame_Fails", "Same object: ", "Expected a different object but both are \"" + first + "\". m" },
        { "AssertNull_Fails", "Expected null but was not null: ", "m" },
        { "AssertSame_Fails", "Not the same object: ", "Expected the same object as \"" + first + "\" but was \"" +
          second + "\". m" },
        { "CheckIsNotType_Fails", "", "Expected not type \"TStrings\" but was \"TStringList\". m" },
        { "CheckIsType_Fails", "", "Expected type \"" + circle + "\" but was null. m" },
        { "CheckNotNull_Fails", "", "Expected not null but was null: \"m\"" },
        { "CheckNotSame_Fails", "", "Expected a different object but both are \"" + values + "\". m" },
        { "CheckNull_Fails", "", "Expected null but was not null: \"m\"" },
        { "CheckSame_Fails", "", "Expected the same object as \"" + first + "\" but was \"" + second + "\". m" },
    };

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "Passes"), ASWBCB_HERE, MessageOf(results, "Passes"));

    for (std::size_t i = 0; i < sizeof(records) / sizeof(records[0]); ++i)
    {
        std::string const& testName = records[i][0];
        std::string const location = StartsWith(testName, "Assert") ?
                (records[i][1] + "Test_" + testName + " (") : ("Check failed for: \"Test_" + testName + "\" (");

        CheckEquals("fail", OutcomeOf(results, testName), ASWBCB_HERE, testName);
        CheckEquals(location + IntToString(group->Lines[testName]) + "): " + records[i][2],
            MessageOf(results, testName), ASWBCB_HERE, testName);
    }

    CheckEquals("", group->Trace, ASWBCB_HERE, "nothing after a failed assert ran");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Ordering_ChecksAndAsserts()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Ordering> group(new TInnerTB_Ordering());
    group->SetRunObserver(&observer);

    // Each failing Assert* test, and the text after its location.
    char const* const asserts[][2] = {
        { "AssertGreaterThan_Fails", "Expected 1 to be > 2. m" },
        { "AssertGreaterThanOrEqual_Fails", "Expected 1.5 to be >= 2. m" },
        { "AssertLessThan_Fails", "Expected 3 to be < 3. m" },
        { "AssertLessThanOrEqual_Fails", "Expected -1 to be <= -2. m" },
    };

    // The failing Check* calls, in order, on consecutive lines.
    char const* const checks[] = {
        "Expected -1 to be > 4294967295. m",
        "Expected 0.25 to be >= 0.5. m",
        "Expected nan to be < 1. m",
        "Expected 98 to be <= 97. m",
    };

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "Passes"), ASWBCB_HERE, MessageOf(results, "Passes"));

    for (std::size_t i = 0; i < sizeof(asserts) / sizeof(asserts[0]); ++i)
    {
        std::string const testName = asserts[i][0];

        CheckEquals("fail", OutcomeOf(results, testName), ASWBCB_HERE, testName);
        CheckEquals("Values out of order: Test_" + testName + " (" + IntToString(group->Lines[testName]) + "): " +
            asserts[i][1], MessageOf(results, testName), ASWBCB_HERE, testName);
    }

    CheckEquals("", group->Trace, ASWBCB_HERE, "nothing after a failed assert ran");

    std::string expectedChecks;

    for (std::size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i)
    {
        expectedChecks += std::string((i == 0) ? "" : "\n") + "Check failed for: \"Test_Checks_Fail\" (" +
            IntToString(group->FirstCheckLine + static_cast<int>(i)) + "): " + checks[i];
    }

    CheckEquals(expectedChecks, MessageOf(results, "Checks_Fail"), ASWBCB_HERE, "each Check* form fails");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_RegisterTestCases_DuplicateNameThrows()
{
    // Arrange
    std::string message = "(no exception)";
    std::unique_ptr<TInnerTB_ParamsDuplicate> group(new TInnerTB_ParamsDuplicate());

    // Act
    try
    {
        group->GetTestCallbackList();
    }
    catch (TExceptDuplicateTestName const& duplicateEx)
    {
        message = duplicateEx.what();
    }

    // Assert
    CheckEquals("Test group \"InnerParamsDuplicate\" registers more than one test named \"Row[same]\"", message,
        ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_RegisterTestCases_RowsRunAfterPublished()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Params> group(new TInnerTB_Params());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    std::string names;

    for (std::size_t i = 0; i < results.CaseRecords.size(); ++i)
        names += ((i == 0) ? std::string() : std::string(",")) + results.CaseRecords[i].TestName;

    CheckEquals("Published,Hex[0],Hex[1],Hex[2],HexNamed[A],HexNamed[f],HexNamed[0]", names, ASWBCB_HERE,
        "the published test first, then the rows in registration order");
    CheckEquals("PAf0Af0", group->Trace, ASWBCB_HERE, "each row ran once, with its own row");
    CheckEquals(5u, results.SuccessCount, ASWBCB_HERE);
    CheckEquals(2u, results.FailedCount, ASWBCB_HERE);

    std::string const failure = "Check failed for: \"Test_Hex\" (" + IntToString(group->CheckLine) +
        "): Expected \"1\" but was \"0\". ";
    CheckEquals(failure, MessageOf(results, "Hex[2]"), ASWBCB_HERE, "a closure");
    CheckEquals(failure, MessageOf(results, "HexNamed[0]"), ASWBCB_HERE, "a pointer to a member function");
    CheckEquals("pass", OutcomeOf(results, "Hex[0]"), ASWBCB_HERE, "its copy of the row, made when registered");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_CatchCrashes_IllegalInstruction()
{
    // On this thread, and on a worker thread under a timeout
    for (int onWorker = 0; onWorker < 2; ++onWorker)
    {
        // Arrange
        std::string const how = (onWorker != 0) ? "on a worker" : "on this thread";
        TOptional<unsigned int> const timeout = (onWorker != 0) ? TOptional<unsigned int>(30u) : TOptional<unsigned int>();
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_Crashes> group(new TInnerTB_Crashes());
        group->SetRunObserver(&observer);

        // Act
        group->Run(nullptr, TOptional<unsigned int>(), timeout, true);

        // Assert: recorded as failed, and the run went on.
        TTestResults const& results = group->Results();
        CheckEquals("A_Passes=pass: \nB_IllegalInstruction=fail: Test crashed: illegal instruction.\nC_Passes=pass: \n",
            RecordsOf(results), ASWBCB_HERE, how);
        CheckFalse(results.Crashed, ASWBCB_HERE, how);
        CheckEquals(2u, results.SuccessCount, ASWBCB_HERE, how);
        CheckEquals(1u, results.FailedCount, ASWBCB_HERE, how);
        CheckEquals("A;TDT:A_Passes;B;C;TDT:C_Passes;", group->Trace, ASWBCB_HERE,
            how + ": the crashed test was abandoned where it crashed, without TearDown_Test()");
        CheckEquals("S:InnerCrashes.A_Passes;F:InnerCrashes.A_Passes=pass;S:InnerCrashes.B_IllegalInstruction;"
            "F:InnerCrashes.B_IllegalInstruction=fail;S:InnerCrashes.C_Passes;F:InnerCrashes.C_Passes=pass;",
            observer.Events, ASWBCB_HERE, how);
        CheckContains(observer.LogText, "Running test: InnerCrashes.B_IllegalInstruction\n"
            "***Test failed: \"InnerCrashes.B_IllegalInstruction\": Test crashed: illegal instruction.\n"
            "!!CRASH!!: \"InnerCrashes.B_IllegalInstruction\" crashed (illegal instruction); continuing with the next "
            "test.\nRunning test: InnerCrashes.C_Passes\n", ASWBCB_HERE, how);
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_CatchCrashes_RecordsTheSame()
{
    // The crash guard leaves every other exception alone, access violations included: outcomes, messages, lifecycle
    // calls and observer events are the same as without it, and with a timeout too.
    std::string plain;
    std::string guarded;

    RunPlainAndWith<TInnerTB_Exceptions>(TOptional<unsigned int>(), true, plain, guarded);
    CheckEquals(plain, guarded, ASWBCB_HERE, "Exceptions");
    CheckContains(guarded, "Unexpected_AccessViolation=fail: Unexpected exception: EAccessViolation: ", ASWBCB_HERE,
        "Exceptions ran");

    RunPlainAndWith<TInnerTB_Exceptions>(30u, true, plain, guarded);
    CheckEquals(plain, guarded, ASWBCB_HERE, "Exceptions, with a timeout");

    RunPlainAndWith<TInnerTB_Throws>(TOptional<unsigned int>(), true, plain, guarded);
    CheckEquals(plain, guarded, ASWBCB_HERE, "Throws");

    RunPlainAndWith<TInnerTB_Lifecycle>(TOptional<unsigned int>(), true, plain, guarded);
    CheckEquals(plain, guarded, ASWBCB_HERE, "Lifecycle");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_CatchCrashes_StackOverflowEndsTheRun()
{
    // On the thread that runs the group, and on a worker thread under a timeout. A thread's stack takes one overflow
    // at most (its guard page is gone afterwards, and a second one ends the process), so the group never runs on the
    // thread running this suite: without a timeout, it runs on a new thread of its own. Otherwise a second run of
    // this suite in one process, as the GUI runner does, would end it.
    for (int onWorker = 0; onWorker < 2; ++onWorker)
    {
        // Arrange
        std::string const how = (onWorker != 0) ? "on a worker" : "on the thread running the group";
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_Overflows> group(new TInnerTB_Overflows());
        group->SetRunObserver(&observer);

        // Act
        if (onWorker != 0)
            group->Run(nullptr, TOptional<unsigned int>(), TOptional<unsigned int>(30u), true);
        else
        {
            std::unique_ptr<TGroupRunThread> const thread(new TGroupRunThread(*group));
            thread->Start();
            thread->WaitFor();
        }

        // Assert: recorded as failed, and the group's remaining tests didn't run.
        TTestResults const& results = group->Results();
        CheckEquals("A_Overflows=fail: Test crashed: stack overflow.\n", RecordsOf(results), ASWBCB_HERE, how);
        CheckTrue(results.Crashed, ASWBCB_HERE, how);
        CheckFalse(results.TimedOut, ASWBCB_HERE, how);
        CheckEquals("A;TDG;", group->Trace, ASWBCB_HERE, how + ": the group was still torn down");
        CheckEquals("S:InnerOverflows.A_Overflows;F:InnerOverflows.A_Overflows=fail;", observer.Events, ASWBCB_HERE,
            how);
        CheckEquals("***Test failed: \"InnerOverflows.A_Overflows\": Test crashed: stack overflow.",
            results.Messages.back(), ASWBCB_HERE, how);
        CheckContains(observer.LogText,
            "!!CRASH!!: \"InnerOverflows.A_Overflows\" crashed in a way that isn't safe to continue past (stack "
            "overflow); abandoning the remaining run.\nDone. Succeeded: 0, failed: 1, skipped: 0\n", ASWBCB_HERE, how);
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_FatalErrorsEndTheRun()
{
    // SetUp_Test() throws an RTL exception
    {
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_SetUpTestThrows> group(new TInnerTB_SetUpTestThrows());
        group->SetRunObserver(&observer);
        std::string message = "(no exception)";
        bool unknown = true;

        try
        {
            group->Run();
        }
        catch (TExceptFatalError const& setUpEx)
        {
            message = setUpEx.what();
            unknown = setUpEx.IsUnknownException();
        }

        CheckEquals("SetUp_Test() exception for \"InnerSetUpTestThrows.A\": EConvertError: 'y' is not a valid "
            "integer value", message, ASWBCB_HERE);
        CheckFalse(unknown, ASWBCB_HERE);
        CheckTrue(Contains(observer.LogText,
            "!!FATAL ERROR!!: SetUp_Test() exception for \"InnerSetUpTestThrows.A\"\n"), ASWBCB_HERE, "logged");
        CheckEquals("", group->Trace, ASWBCB_HERE, "neither the test nor TearDown_Group() ran");
        CheckEquals("S:InnerSetUpTestThrows.A;", observer.Events, ASWBCB_HERE, "and the test has no record");
    }

    // TearDown_Test() throws something that isn't an exception class
    {
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_TearDownTestThrows> group(new TInnerTB_TearDownTestThrows());
        group->SetRunObserver(&observer);
        std::string message = "(no exception)";
        bool unknown = false;

        try
        {
            group->Run();
        }
        catch (TExceptFatalError const& tearDownEx)
        {
            message = tearDownEx.what();
            unknown = tearDownEx.IsUnknownException();
        }

        CheckEquals("TearDown_Test() exception for \"InnerTearDownTestThrows.A\": an object that is neither a "
            "std::exception nor an RTL Exception", message, ASWBCB_HERE);
        CheckTrue(unknown, ASWBCB_HERE, "exit code 3");
    }

    // TearDown_Group() throws, after the tests ran
    {
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_TearDownGroupThrows> group(new TInnerTB_TearDownGroupThrows());
        group->SetRunObserver(&observer);
        std::string message = "(no exception)";

        try
        {
            group->Run();
        }
        catch (TExceptFatalError const& tearDownGroupEx)
        {
            message = tearDownGroupEx.what();
        }

        CheckEquals("TearDown_Group() exception for group \"InnerTearDownGroupThrows\": teardown boom", message,
            ASWBCB_HERE);
        CheckEquals("pass", OutcomeOf(group->Results(), "A"), ASWBCB_HERE, "the test ran first");
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_LifecycleOrder()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Lifecycle> group(new TInnerTB_Lifecycle());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    CheckEquals("SUG;SUT:Pass;Pass;TDT:Pass;SUT:Fail;Fail;TDT:Fail;SUT:Skip;Skip;TDT:Skip;TDG;", group->Trace,
        ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_LogLines()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Lifecycle> group(new TInnerTB_Lifecycle());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert: the same lines as asw-unit-tests
    std::string const& log = observer.LogText;
    CheckTrue(StartsWith(log, "Running 3 tests.\nRunning test: InnerLifecycle.Pass\nFinished test: "
        "\"InnerLifecycle.Pass\" - passed ("), ASWBCB_HERE, log);
    CheckTrue(Contains(log, " ms)\nRunning test: InnerLifecycle.Fail\n***Test failed: \"InnerLifecycle.Fail\": "
        "Failed: Test_Fail ("), ASWBCB_HERE);
    CheckTrue(Contains(log, "\n***Test skipped: \"InnerLifecycle.Skip\": Test skipped: Test_Skip ("), ASWBCB_HERE);
    CheckTrue(Contains(log, "Finished test: \"InnerLifecycle.Skip\" - skipped ("), ASWBCB_HERE);
    CheckTrue(Contains(log, "\nDone. Succeeded: 1, failed: 1, skipped: 1\n["), ASWBCB_HERE);
    CheckTrue(Contains(log, "] Tearing down group: \"InnerLifecycle\"\n"), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_ObserverEventsAndStop()
{
    // Arrange
    TRecordingObserver observer;
    TRecordingObserver stopper;
    stopper.StopAfterStarted = 2;
    std::unique_ptr<TInnerTB_Lifecycle> group(new TInnerTB_Lifecycle());
    group->SetRunObserver(&observer);

    // Act
    group->Run();
    TTestResults const firstRun = group->Results();
    group->SetRunObserver(&stopper);
    group->Run();

    // Assert
    CheckEquals("S:InnerLifecycle.Pass;F:InnerLifecycle.Pass=pass;S:InnerLifecycle.Fail;F:InnerLifecycle.Fail=fail;"
        "S:InnerLifecycle.Skip;F:InnerLifecycle.Skip=skip;", observer.Events, ASWBCB_HERE);
    CheckFalse(firstRun.Stopped, ASWBCB_HERE);
    CheckTrue(group->Results().Stopped, ASWBCB_HERE, "stopped before the third test");
    CheckEquals(2u, static_cast<unsigned int>(group->Results().CaseRecords.size()), ASWBCB_HERE,
        "a rerun reports only its own outcomes");
    CheckTrue(Contains(stopper.LogText, "Tearing down group"), ASWBCB_HERE, "a stopped group is still torn down");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_RecordsEachOutcome()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Lifecycle> group(new TInnerTB_Lifecycle());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals(1u, results.SuccessCount, ASWBCB_HERE);
    CheckEquals(1u, results.FailedCount, ASWBCB_HERE);
    CheckEquals(1u, results.SkippedCount, ASWBCB_HERE);
    AssertEquals(3u, static_cast<unsigned int>(results.CaseRecords.size()), ASWBCB_HERE);
    CheckEquals("InnerLifecycle", results.CaseRecords[0].GroupName, ASWBCB_HERE);
    CheckEquals("Pass", results.CaseRecords[0].TestName, ASWBCB_HERE);
    CheckEquals("pass", OutcomeOf(results, "Pass"), ASWBCB_HERE);
    CheckEquals("", results.CaseRecords[0].Message, ASWBCB_HERE, "no detail for a pass");
    CheckTrue((results.CaseRecords[0].DurationSeconds >= 0.0) && (results.CaseRecords[0].DurationSeconds < 10.0),
        ASWBCB_HERE, "a duration");
    CheckEquals("fail", OutcomeOf(results, "Fail"), ASWBCB_HERE);
    CheckEquals("skip", OutcomeOf(results, "Skip"), ASWBCB_HERE);
    AssertEquals(2u, static_cast<unsigned int>(results.Messages.size()), ASWBCB_HERE);
    CheckTrue(StartsWith(results.Messages[0], "***Test failed: \"InnerLifecycle.Fail\": Failed: "), ASWBCB_HERE);
    CheckTrue(StartsWith(results.Messages[1], "***Test skipped: \"InnerLifecycle.Skip\": Test skipped: "),
        ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_RecordsShortDurations()
{
    // Arrange: a test of 0.2 ms, which a clock that advances every 10 or 15.6 ms (e.g. GetTickCount(), or RAD Studio's
    // 32-bit std::chrono::high_resolution_clock, which asw-unit-tests 1.2.1 stopped using) would record as 0
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_ShortTest> group(new TInnerTB_ShortTest());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    AssertEquals(1u, static_cast<unsigned int>(results.CaseRecords.size()), ASWBCB_HERE);
    CheckGreaterThanOrEqual(results.CaseRecords[0].DurationSeconds, TInnerTB_ShortTest::BusyWaitSeconds, ASWBCB_HERE,
        "the recorded duration includes the busy-wait");
    CheckLessThan(results.CaseRecords[0].DurationSeconds, 1.0, ASWBCB_HERE, "in seconds");
    CheckFalse(Contains(observer.LogText, "(0.000 ms)"), ASWBCB_HERE, "the log line shows it too");

    // The log line shows the same duration in milliseconds, rounded to three decimals, so a record in another unit
    // than seconds shows up here, which the bounds above are too wide to catch.
    std::string const finished = "Finished test: \"InnerShortTest.BusyWaits\" - passed (";
    std::string::size_type const finishedPos = observer.LogText.find(finished);
    AssertTrue(finishedPos != std::string::npos, ASWBCB_HERE, observer.LogText);
    std::istringstream loggedDuration(observer.LogText.substr(finishedPos + finished.size()));
    double loggedMs = -1.0;
    loggedDuration >> loggedMs;
    CheckNear(loggedMs / 1000.0, results.CaseRecords[0].DurationSeconds, 0.000001, ASWBCB_HERE,
        "the logged duration, in seconds");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_SetUpGroupThrows_FailsEachTest()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_SetUpGroupThrows> group(new TInnerTB_SetUpGroupThrows());
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("SetUp_Group() exception: setup boom", MessageOf(results, "X"), ASWBCB_HERE);
    CheckEquals("SetUp_Group() exception: setup boom", MessageOf(results, "Y"), ASWBCB_HERE);
    CheckEquals(2u, results.FailedCount, ASWBCB_HERE);
    CheckEquals("TDG;", group->Trace, ASWBCB_HERE, "no test ran, and TearDown_Group() did");
    CheckTrue(Contains(observer.LogText, "!!ERROR!!: SetUp_Group() exception for group \"InnerSetUpGroupThrows\": "
        "setup boom\n"), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_Timeout_AbandonsHungTest()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Hangs> group(new TInnerTB_Hangs());
    group->SetRunObserver(&observer);
    TStopwatch stopwatch = TStopwatch::StartNew();

    // Act
    group->Run(nullptr, TOptional<unsigned int>(), 1u);

    // Assert
    __int64 const elapsedMs = stopwatch.ElapsedMilliseconds;
    CheckGreaterThanOrEqual(elapsedMs, 1000, ASWBCB_HERE, "waited for the whole timeout");
    CheckLessThan(elapsedMs, 5000, ASWBCB_HERE, "and no longer");

    TTestResults const& results = group->Results();
    CheckTrue(results.TimedOut, ASWBCB_HERE);
    CheckEquals("A_Passes=pass: \n"
        "B_Hangs=fail: Test exceeded its 1 second timeout and was abandoned.\n", RecordsOf(results), ASWBCB_HERE,
        "the third test didn't run");
    AssertEquals(2u, static_cast<unsigned int>(results.CaseRecords.size()), ASWBCB_HERE);
    CheckNear(1.0, results.CaseRecords[1].DurationSeconds, 1e-12, ASWBCB_HERE, "the timeout as its duration");
    CheckEquals(1u, results.SuccessCount, ASWBCB_HERE);
    CheckEquals(1u, results.FailedCount, ASWBCB_HERE);
    CheckEquals("A;B;TDG;", group->Trace, ASWBCB_HERE, "the group was still torn down");
    CheckEquals("S:InnerHangs.A_Passes;F:InnerHangs.A_Passes=pass;S:InnerHangs.B_Hangs;F:InnerHangs.B_Hangs=fail;",
        observer.Events, ASWBCB_HERE, "the abandoned test was reported");
    CheckFalse(observer.EventFromOtherThread, ASWBCB_HERE, "from this thread");
    CheckEquals("***Test failed: \"InnerHangs.B_Hangs\": Test exceeded its 1 second timeout and was abandoned.",
        results.Messages.back(), ASWBCB_HERE);
    CheckContains(observer.LogText, "Running test: InnerHangs.B_Hangs\n"
        "***Test failed: \"InnerHangs.B_Hangs\": Test exceeded its 1 second timeout and was abandoned.\n"
        "!!TIMEOUT!!: \"InnerHangs.B_Hangs\" exceeded 1 second(s); abandoning the remaining run.\n"
        "Done. Succeeded: 1, failed: 1, skipped: 0\n", ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_Timeout_FatalErrorsEndTheRun()
{
    // A SetUp_Test() that throws on the worker thread ends the run on this one, as without a timeout.
    {
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_SetUpTestThrows> group(new TInnerTB_SetUpTestThrows());
        group->SetRunObserver(&observer);
        std::string message = "(no exception)";
        bool unknown = true;

        try
        {
            group->Run(nullptr, TOptional<unsigned int>(), 30u);
        }
        catch (TExceptFatalError const& setUpEx)
        {
            message = setUpEx.what();
            unknown = setUpEx.IsUnknownException();
        }

        CheckEquals("SetUp_Test() exception for \"InnerSetUpTestThrows.A\": EConvertError: 'y' is not a valid "
            "integer value", message, ASWBCB_HERE);
        CheckFalse(unknown, ASWBCB_HERE);
        CheckEquals("", group->Trace, ASWBCB_HERE, "neither the test nor TearDown_Group() ran");
        CheckEquals("S:InnerSetUpTestThrows.A;", observer.Events, ASWBCB_HERE, "and the test has no record");
    }

    // So does a TearDown_Test() that throws something that isn't an exception class, with the same exit code.
    {
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_TearDownTestThrows> group(new TInnerTB_TearDownTestThrows());
        group->SetRunObserver(&observer);
        std::string message = "(no exception)";
        bool unknown = false;

        try
        {
            group->Run(nullptr, TOptional<unsigned int>(), 30u);
        }
        catch (TExceptFatalError const& tearDownEx)
        {
            message = tearDownEx.what();
            unknown = tearDownEx.IsUnknownException();
        }

        CheckEquals("TearDown_Test() exception for \"InnerTearDownTestThrows.A\": an object that is neither a "
            "std::exception nor an RTL Exception", message, ASWBCB_HERE);
        CheckTrue(unknown, ASWBCB_HERE, "exit code 3");
    }

    // Anything else that gets out of Test() on the worker arrives as a TExceptFatalError, with what() the
    // description the console runner would print for the original, and the same exit code.
    TInnerTB_TestEscapes::TKind const kinds[] = { TInnerTB_TestEscapes::TKind::Std,
                                                  TInnerTB_TestEscapes::TKind::RTL,
                                                  TInnerTB_TestEscapes::TKind::Other };
    char const* const messages[] = { "escaped", "EConvertError: rtl escaped", "Unknown" };

    for (std::size_t i = 0; i < (sizeof(kinds) / sizeof(kinds[0])); ++i)
    {
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_TestEscapes> group(new TInnerTB_TestEscapes());
        group->SetRunObserver(&observer);
        group->Kind = kinds[i];
        std::string message = "(no exception)";
        bool unknown = (kinds[i] != TInnerTB_TestEscapes::TKind::Other);

        try
        {
            group->Run(nullptr, TOptional<unsigned int>(), 30u);
        }
        catch (TExceptFatalError const& escapedEx)
        {
            message = escapedEx.what();
            unknown = escapedEx.IsUnknownException();
        }

        CheckEquals(messages[i], message, ASWBCB_HERE, messages[i]);
        CheckEquals(kinds[i] == TInnerTB_TestEscapes::TKind::Other, unknown, ASWBCB_HERE, messages[i]);
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_Timeout_IgnoresLateFinish()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Late> group(new TInnerTB_Late());
    group->SetRunObserver(&observer);

    // Act: time out, then let the abandoned test fail its check, end, and get past where it would record that.
    group->Run(nullptr, TOptional<unsigned int>(), 1u);
    AssertEquals(static_cast<unsigned int>(WAIT_OBJECT_0),
        static_cast<unsigned int>(::WaitForSingleObject(group->TornDown, 10000)), ASWBCB_HERE, "the test ended");
    ::Sleep(200);

    // Assert: only the timeout is recorded and logged.
    TTestResults const& results = group->Results();
    CheckTrue(results.TimedOut, ASWBCB_HERE);
    CheckEquals("Late=fail: Test exceeded its 1 second timeout and was abandoned.\n", RecordsOf(results), ASWBCB_HERE);
    CheckEquals(1u, results.FailedCount, ASWBCB_HERE);
    CheckEquals(0u, results.SuccessCount, ASWBCB_HERE);
    CheckEquals(1u, static_cast<unsigned int>(results.Messages.size()), ASWBCB_HERE, "no Check failure message");
    CheckNotContains(observer.LogText, "Check failed", ASWBCB_HERE);
    CheckNotContains(observer.LogText, "Finished test", ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_Timeout_KeepsTheFloatingPointEnvironment()
{
    // Arrange: this thread rounds downward for the runs, and as before afterwards, whatever happens. On Win64 a new
    // thread starts rounding to nearest, so a test run on one without this thread's environment would too.
    struct TRoundingModeRestorer
    {
        int const Mode;

        TRoundingModeRestorer()
            : Mode(RoundingMode())
        {
        }

        ~TRoundingModeRestorer()
        {
            SetRoundingMode(Mode);
        }
    };

    TRoundingModeRestorer const restorer;
    SetRoundingMode(DownwardRoundingMode());
    AssertEquals(DownwardRoundingMode(), RoundingMode(), ASWBCB_HERE, "this thread now rounds downward");

    // Without a timeout, the test runs on this thread, rounding downward.
    {
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_Rounding> group(new TInnerTB_Rounding());
        group->SetRunObserver(&observer);

        group->Run();

        CheckEquals("Records=pass: \n", RecordsOf(group->Results()), ASWBCB_HERE);
        CheckEquals(DownwardRoundingMode(), group->RoundingModeInTest, ASWBCB_HERE, "without a timeout");
    }

    // With one, on a worker thread, still rounding downward.
    {
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_Rounding> group(new TInnerTB_Rounding());
        group->SetRunObserver(&observer);

        group->Run(nullptr, TOptional<unsigned int>(), 10u);

        CheckEquals("Records=pass: \n", RecordsOf(group->Results()), ASWBCB_HERE);
        CheckEquals(DownwardRoundingMode(), group->RoundingModeInTest, ASWBCB_HERE, "with a timeout");
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_Timeout_RecordsTheSame()
{
    // Each test runs on a worker thread, and comes out the same: outcomes, messages (unexpected RTL exceptions and
    // access violations, expected exceptions, Check* failures), lifecycle calls and observer events.
    std::string plain;
    std::string timed;

    RunPlainAndWith<TInnerTB_Lifecycle>(30u, false, plain, timed);
    CheckEquals(plain, timed, ASWBCB_HERE, "Lifecycle");
    CheckContains(timed, "Skip=skip: ", ASWBCB_HERE, "Lifecycle ran");

    RunPlainAndWith<TInnerTB_Exceptions>(30u, false, plain, timed);
    CheckEquals(plain, timed, ASWBCB_HERE, "Exceptions");
    CheckContains(timed, "Unexpected_AccessViolation=fail: Unexpected exception: EAccessViolation: ", ASWBCB_HERE,
        "Exceptions ran");

    RunPlainAndWith<TInnerTB_Checks>(30u, false, plain, timed);
    CheckEquals(plain, timed, ASWBCB_HERE, "Checks");
    CheckContains(timed, "TwoChecksFail=fail: Check failed for: ", ASWBCB_HERE, "Checks ran");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Run_Timeout_RunsOnAWorkerThread()
{
    DWORD const thisThread = ::GetCurrentThreadId();

    // Without a timeout, the test runs on this thread, and its TThread::Synchronize() call on the main thread (this
    // one, unless this test itself runs under a timeout).
    {
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_Threads> group(new TInnerTB_Threads());
        group->SetRunObserver(&observer);

        group->Run();

        CheckEquals("Records=pass: \n", RecordsOf(group->Results()), ASWBCB_HERE);
        CheckEquals(thisThread, group->TestRanOn, ASWBCB_HERE);
        CheckEquals(MainThreadID, group->SynchronizedOn, ASWBCB_HERE);
    }

    // With one, on a worker thread, while Synchronize() still runs on the main thread, and the observer's events
    // still come from this thread.
    {
        TRecordingObserver observer;
        std::unique_ptr<TInnerTB_Threads> group(new TInnerTB_Threads());
        group->SetRunObserver(&observer);

        group->Run(nullptr, TOptional<unsigned int>(), 10u);

        CheckEquals("Records=pass: \n", RecordsOf(group->Results()), ASWBCB_HERE, "Synchronize() didn't hang");
        CheckFalse(group->Results().TimedOut, ASWBCB_HERE);
        CheckNotEquals(thisThread, group->TestRanOn, ASWBCB_HERE, "a worker thread");
        CheckNotEquals(0u, static_cast<unsigned int>(group->TestRanOn), ASWBCB_HERE, "the test ran");
        CheckEquals(MainThreadID, group->SynchronizedOn, ASWBCB_HERE);
        CheckEquals("S:InnerThreads.Records;F:InnerThreads.Records=pass;", observer.Events, ASWBCB_HERE);
        CheckFalse(observer.EventFromOtherThread, ASWBCB_HERE);

        // On the main thread, the call the test queued has run by the time Run() returns. (Elsewhere, the main thread
        // runs it whenever it gets to it.)
        if (thisThread == MainThreadID)
            CheckEquals(MainThreadID, group->QueuedOn, ASWBCB_HERE, "the queued call");
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_TextParts_ChecksAndAsserts()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_TextParts> group(new TInnerTB_TextParts());
    group->SetRunObserver(&observer);

    // Each failing Assert* test: its assertion, its message's title, and the text after the part.
    char const* const asserts[][3] = {
        { "Contains", "Substring not found", "to contain \"xyz\". m" },
        { "ContainsIC", "Substring not found", "to contain \"XYZ\" (ignoring case). m" },
        { "EndsWith", "Suffix not found", "to end with \"Hello\". m" },
        { "EndsWithIC", "Suffix not found", "to end with \"HELLO\" (ignoring case). m" },
        { "NotContains", "Substring found", "not to contain \"lo W\". m" },
        { "NotContainsIC", "Substring found", "not to contain \"LO w\" (ignoring case). m" },
        { "NotEndsWith", "Suffix found", "not to end with \"World\". m" },
        { "NotEndsWithIC", "Suffix found", "not to end with \"WORLD\" (ignoring case). m" },
        { "NotStartsWith", "Prefix found", "not to start with \"Hello\". m" },
        { "NotStartsWithIC", "Prefix found", "not to start with \"HELLO\" (ignoring case). m" },
        { "StartsWith", "Prefix not found", "to start with \"World\". m" },
        { "StartsWithIC", "Prefix not found", "to start with \"WORLD\" (ignoring case). m" },
    };

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "Passes"), ASWBCB_HERE, MessageOf(results, "Passes"));

    for (std::size_t i = 0; i < sizeof(asserts) / sizeof(asserts[0]); ++i)
    {
        std::string const testName = std::string("Assert") + asserts[i][0] + "_Fails";
        CheckEquals("fail", OutcomeOf(results, testName), ASWBCB_HERE, testName);
        CheckEquals(std::string(asserts[i][1]) + ": Test_" + testName + " (" + IntToString(group->Lines[testName]) +
            "): Expected \"Hello World\" " + asserts[i][2], MessageOf(results, testName), ASWBCB_HERE);
    }

    CheckEquals("", group->Trace, ASWBCB_HERE, "nothing after a failed assert ran");

    std::string checks;

    for (std::size_t i = 0; i < sizeof(asserts) / sizeof(asserts[0]); ++i)
    {
        checks += std::string((i == 0) ? "" : "\n") + "Check failed for: \"Test_Checks_Fail\" (" +
            IntToString(group->FirstCheckLine + static_cast<int>(i)) + "): Expected \"Hello World\" " + asserts[i][2];
    }

    CheckEquals(checks, MessageOf(results, "Checks_Fail"), ASWBCB_HERE, "each Check* form fails and the test goes on");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_TestBase::Test_Throws_ChecksAndAsserts()
{
    // Arrange
    TRecordingObserver observer;
    std::unique_ptr<TInnerTB_Throws> group(new TInnerTB_Throws());
    group->SetRunObserver(&observer);

    std::string const unknown = DescribeUnknownException();

    // Each failing Assert* test, and the text after its location.
    char const* const asserts[][3] = {
        { "AssertNoThrow_Fails", "Unexpected exception", "Expected no exception but caught: boom. m" },
        { "AssertThrows_Fails", "Expected exception not caught", "Expected an exception but none was thrown. m" },
    };

    // The failing Check* calls, in order, on consecutive lines.
    std::string const checks[] = {
        "Expected an exception but none was thrown. m",
        "Expected a different exception type but caught: EConvertError: bad. m",
        "Expected a different exception type but caught: " + unknown + ". m",
        "Expected a different exception type but caught: boom. m",
        "Expected the exception message to contain \"bang\" but caught: boom. m",
        "Expected the exception message to contain \"zzz\" but caught: EConvertError: bad. m",
        "Expected the exception message to contain \"x\" but caught an exception without a message: " + unknown +
        ". m",
        "Expected no exception but caught: EConvertError: bad. m",
        "Expected no exception but caught: " + unknown + ". m",
        "Expected no exception but caught: boom. m",
    };

    // Act
    group->Run();

    // Assert
    TTestResults const& results = group->Results();
    CheckEquals("pass", OutcomeOf(results, "Passes"), ASWBCB_HERE, MessageOf(results, "Passes"));

    for (std::size_t i = 0; i < sizeof(asserts) / sizeof(asserts[0]); ++i)
    {
        std::string const testName = asserts[i][0];

        CheckEquals("fail", OutcomeOf(results, testName), ASWBCB_HERE, testName);
        CheckEquals(std::string(asserts[i][1]) + ": Test_" + testName + " (" + IntToString(group->Lines[testName]) +
            "): " + asserts[i][2], MessageOf(results, testName), ASWBCB_HERE, testName);
    }

    std::string expectedChecks;

    for (std::size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i)
    {
        expectedChecks += std::string((i == 0) ? "" : "\n") + "Check failed for: \"Test_Checks_Fail\" (" +
            IntToString(group->FirstCheckLine + static_cast<int>(i)) + "): " + checks[i];
    }

    CheckEquals(expectedChecks, MessageOf(results, "Checks_Fail"), ASWBCB_HERE, "each Check* form fails");

    CheckEquals("fail", OutcomeOf(results, "AssertInside_PassesThrough"), ASWBCB_HERE);
    CheckEquals("Expected true but was false: AssertFalseInside (" + IntToString(group->Lines["Inner"]) + "): inner",
        MessageOf(results, "AssertInside_PassesThrough"), ASWBCB_HERE, "the inner Assert* failure, unchanged");
    CheckEquals("skip", OutcomeOf(results, "SkipInside_PassesThrough"), ASWBCB_HERE);
    CheckEquals("", group->Trace, ASWBCB_HERE, "nothing after a failed assert or a skip ran");

    // An access violation in any kind of callable is what the checks find, rather than escaping them.
    CheckEquals("pass", OutcomeOf(results, "AccessViolations"), ASWBCB_HERE, MessageOf(results, "AccessViolations"));

    std::string const avMessages = MessageOf(results, "AccessViolations_Fail");
    char const* const avChecks[] = {
        "Expected no exception but caught: EAccessViolation: ",
        "Expected a different exception type but caught: EAccessViolation: ",
        "Expected no exception but caught: EAccessViolation: ",
        "Expected no exception but caught: EAccessViolation: ",
    };
    std::string::size_type lineStart = 0;

    for (std::size_t i = 0; i < sizeof(avChecks) / sizeof(avChecks[0]); ++i)
    {
        std::string const prefix = "Check failed for: \"Test_AccessViolations_Fail\" (" +
            IntToString(group->Lines["AccessViolations_Fail"] + static_cast<int>(i)) + "): " + avChecks[i];

        CheckEquals(prefix, avMessages.substr(lineStart, prefix.size()), ASWBCB_HERE, avMessages);
        lineStart = avMessages.find('\n', lineStart);
        lineStart = (lineStart == std::string::npos) ? avMessages.size() : (lineStart + 1);
    }

    CheckEquals(avMessages.size(), lineStart, ASWBCB_HERE, "no other message: " + avMessages);
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_TestBase, "SelfTests")
