/* **************************************************************************
ASWUnitTestsBCB_TestBase.cpp
Author: Anthony S. West - ASW Software

The base class of test groups, and the test cases it finds in their published sections.

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
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------
#include <cmath>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <System.Diagnostics.hpp>
#include <System.SysUtils.hpp>
#include <System.TimeSpan.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Console.h"
#include "ASWUnitTestsBCB_CrashGuard.h"
#include "ASWUnitTestsBCB_Exception.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Classes;
using namespace System::Diagnostics;
using namespace System::Sysutils;
using namespace System::Timespan;
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

namespace
{

// A closure is the pair of pointers TMethod holds, which MakeTestMethod() copies into one.
typedef char TClosureMatchesTMethod[(sizeof(TTestMethod) == sizeof(TMethod)) ? 1 : -1];

// A method in a class's own published-method table.
struct TPublishedMethod
{
    std::string Name;
    void* Code;
};

/////////////////////////////////////////////////////////////////////////////
// TTestWorkerThread
//
// The thread TTestGroupBase::RunWithTimeout() runs a test on: it calls
// 'work' once. Created suspended, and never freed by itself, so the
// waiting thread decides whether to free it or abandon it.
/////////////////////////////////////////////////////////////////////////////
class TTestWorkerThread : public TThread
{
private:
    typedef TThread inherited;

private:
    TThreadMethod m_Work;

protected:
    virtual void __fastcall Execute();

public:
    explicit __fastcall TTestWorkerThread(TThreadMethod work);
};

char const TestMethodPrefix[] = "Test_";
std::size_t const TestMethodPrefixLength = sizeof(TestMethodPrefix) - 1;

// TTestGroupBase::m_WorkerState: whether a test on RunWithTimeout()'s worker thread may change the group's results.
long const WorkerIdle = 0;      // No test runs on a worker
long const WorkerRunning = 1;   // It may, once it moves to WorkerRecording or WorkerFinished
long const WorkerRecording = 2; // It is, and can't be abandoned until it's back to WorkerRunning
long const WorkerFinished = 3;  // It has recorded its outcome, and can't be abandoned
long const WorkerAbandoned = 4; // It may no longer

// Abandons the test on a worker thread whose state is 'workerState' and returns true, unless it has recorded its
// outcome (WorkerFinished); waits for a recording in progress to end first.
bool AbandonWorker(long volatile& workerState);
double ElapsedMs(TStopwatch& stopwatch);
// True when 'actual' is near 'expected' (see TTestGroupBase::CheckNear()); also sets 'difference'.
template <typename TFloat>
bool IsNear(TFloat expected, TFloat actual, TFloat tolerance, TFloat& difference);
bool IsTestMethodName(std::string const& methodName);
TTestMethod MakeTestMethod(void* code, void* data);
// The values a Near failure shows, as in asw-unit-tests: "1.250000 (diff 0.250000)" and "1.000000 (tolerance
// 0.125000)", and a NotNear failure's "1.250000 (diff 0.250000 <= tolerance 0.500000)".
std::string NearActualText(double actual, double difference);
std::string NearExpectedText(double expected, double tolerance);
std::string NotNearText(double actual, double difference, double tolerance);
std::vector<TPublishedMethod> PublishedMethods(TClass cls);
bool WaitForWorker(TThread& worker, unsigned int timeoutSeconds);

//---------------------------------------------------------------------------
__fastcall TTestWorkerThread::TTestWorkerThread(TThreadMethod work)
    : inherited(true),
      m_Work(work)
{
    FreeOnTerminate = false;
}
//---------------------------------------------------------------------------
void __fastcall TTestWorkerThread::Execute()
{
    m_Work();
}
//---------------------------------------------------------------------------
bool AbandonWorker(long volatile& workerState)
{
    for (;;)
    {
        long const state = ::InterlockedCompareExchange(&workerState, WorkerAbandoned, WorkerRunning);

        if (state != WorkerRecording)
            return state == WorkerRunning;

        ::Sleep(0);
    }
}
//---------------------------------------------------------------------------
double ElapsedMs(TStopwatch& stopwatch)
{
    TTimeSpan elapsed = stopwatch.Elapsed;
    return elapsed.TotalMilliseconds;
}
//---------------------------------------------------------------------------
template <typename TFloat>
bool IsNear(TFloat expected, TFloat actual, TFloat tolerance, TFloat& difference)
{
    difference = std::fabs(expected - actual);

    // A NaN fails both comparisons; equal infinities only the second, since their difference is a NaN.
    return (expected == actual) || (difference <= tolerance);
}
//---------------------------------------------------------------------------
/*
    IsTestMethodName

    True for "Test_<Name>" with a non-empty <Name>.
*/
bool IsTestMethodName(std::string const& methodName)
{
    return (methodName.size() > TestMethodPrefixLength) &&
        (methodName.compare(0, TestMethodPrefixLength, TestMethodPrefix) == 0);
}
//---------------------------------------------------------------------------
TTestMethod MakeTestMethod(void* code, void* data)
{
    TMethod closure;
    closure.Code = code;
    closure.Data = data;

    TTestMethod method;
    std::memcpy(&method, &closure, sizeof(method));
    return method;
}
//---------------------------------------------------------------------------
std::string NearActualText(double actual, double difference)
{
    return DescribeFixed(actual) + " (diff " + DescribeFixed(difference) + ")";
}
//---------------------------------------------------------------------------
std::string NearExpectedText(double expected, double tolerance)
{
    return DescribeFixed(expected) + " (tolerance " + DescribeFixed(tolerance) + ")";
}
//---------------------------------------------------------------------------
std::string NotNearText(double actual, double difference, double tolerance)
{
    return DescribeFixed(actual) + " (diff " + DescribeFixed(difference) + " <= tolerance " + DescribeFixed(tolerance) +
        ")";
}
//---------------------------------------------------------------------------
/*
    PublishedMethods

    Returns the methods in the published sections of 'cls' itself (not its ancestors), in declaration order, read
    from the class's published-method table, as Delphi's TObject.MethodAddress() does. The table is a 16-bit count
    followed by that many entries of { 16-bit entry length, code pointer, ShortString name, ... }, packed; some
    compilers add data after the name, so each entry is stepped over by its length. A class without a published
    method has no table.
*/
std::vector<TPublishedMethod> PublishedMethods(TClass cls)
{
    std::vector<TPublishedMethod> methods;

    unsigned char const* table = nullptr;
    std::memcpy(&table, reinterpret_cast<unsigned char const*>(cls) + vmtMethodTable, sizeof(table));

    if (table == nullptr)
        return methods;

    unsigned short count = 0;
    std::memcpy(&count, table, sizeof(count));
    unsigned char const* entry = table + sizeof(count);

    for (unsigned short i = 0; i < count; ++i)
    {
        unsigned short entryLength = 0;
        std::memcpy(&entryLength, entry, sizeof(entryLength));

        TPublishedMethod method;
        std::memcpy(&method.Code, entry + sizeof(entryLength), sizeof(method.Code));

        unsigned char const* const name = entry + sizeof(entryLength) + sizeof(method.Code);
        std::size_t const nameLength = name[0];

        // ToUTF8() is qualified, since bcc32 doesn't find a function of the enclosing namespace from an anonymous
        // one when the argument has a class type.
        if (static_cast<std::size_t>(entryLength) < sizeof(entryLength) + sizeof(method.Code) + 1 + nameLength)
            throw std::logic_error("Unexpected published-method table layout in class \"" +
                ASWUnitTestsBCB::ToUTF8(cls->ClassName()) + "\"");

        method.Name.assign(reinterpret_cast<char const*>(name + 1), nameLength);
        methods.push_back(method);

        entry += entryLength;
    }

    return methods;
}
//---------------------------------------------------------------------------
/*
    WaitForWorker

    Waits up to 'timeoutSeconds' for 'worker' to end, and returns whether it did. On the main thread, it also runs
    the worker's TThread::Synchronize() and Queue() calls (CheckSynchronize()), which wait for the main thread: those
    made while it waits, and, once the worker has ended, any it left queued. Without a timeout, a test runs on the
    main thread, where they run right away; under one, a test calling Synchronize() would otherwise hang, and its
    queued calls would wait for some later CheckSynchronize().
*/
bool WaitForWorker(TThread& worker, unsigned int timeoutSeconds)
{
    __int64 const timeoutMs = static_cast<__int64>(timeoutSeconds) * 1000;
    __int64 const longestWaitMs = 0x7FFFFFFF; // Less than INFINITE
    HANDLE const handles[2] = { reinterpret_cast<HANDLE>(worker.Handle), reinterpret_cast<HANDLE>(SyncEvent) };
    bool const onMainThread = (::GetCurrentThreadId() == MainThreadID);
    TStopwatch stopwatch = TStopwatch::StartNew();

    for (;;)
    {
        __int64 const remainingMs = timeoutMs - stopwatch.ElapsedMilliseconds;
        __int64 const waitMs = (remainingMs <= 0) ? 0 : ((remainingMs > longestWaitMs) ? longestWaitMs : remainingMs);
        DWORD const result = ::WaitForMultipleObjects(onMainThread ? 2 : 1, handles, FALSE,
            static_cast<DWORD>(waitMs));

        if (result == WAIT_OBJECT_0)
        {
            if (onMainThread)
                CheckSynchronize();

            return true;
        }

        // A wait can end a little early, so only one that began after the time ran out ends it (and a failed wait,
        // which nothing can repair, counts as one).
        if (result == (WAIT_OBJECT_0 + 1))
            CheckSynchronize();
        else if ((remainingMs <= 0) || (result == WAIT_FAILED))
            return false;
    }
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// TCheckMessage
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TCheckMessage::TCheckMessage()
{
}
//---------------------------------------------------------------------------
TCheckMessage::TCheckMessage(char const* text)
    : Text((text != nullptr) ? text : "")
{
}
//---------------------------------------------------------------------------
TCheckMessage::TCheckMessage(std::string const& text)
    : Text(text)
{
}
//---------------------------------------------------------------------------
TCheckMessage::TCheckMessage(wchar_t const* text)
    : Text((text != nullptr) ? ToUTF8(UnicodeString(text)) : std::string())
{
}
//---------------------------------------------------------------------------
TCheckMessage::TCheckMessage(std::wstring const& text)
    : Text(ToUTF8(UnicodeString(text.c_str(), static_cast<int>(text.size()))))
{
}
//---------------------------------------------------------------------------
TCheckMessage::TCheckMessage(AnsiString const& text)
    : Text(ToUTF8(UnicodeString(text)))
{
}
//---------------------------------------------------------------------------
TCheckMessage::TCheckMessage(UnicodeString const& text)
    : Text(ToUTF8(text))
{
}
//---------------------------------------------------------------------------
TCheckMessage::TCheckMessage(UTF8String const& text)
    : Text(text.c_str(), static_cast<std::string::size_type>(text.Length()))
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TSuitePath
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TSuitePath::TSuitePath(std::string const& path)
    : m_Path(path),
      m_Valid(true)
{
    if (path.empty())
        return;

    std::string::size_type start = 0;

    for (;;)
    {
        std::string::size_type const dot = path.find('.', start);
        std::string const name = path.substr(start, (dot == std::string::npos) ? std::string::npos : (dot - start));

        if (name.empty())
            m_Valid = false;

        m_Names.push_back(name);

        if (dot == std::string::npos)
            break;

        start = dot + 1;
    }
}
//---------------------------------------------------------------------------
std::vector<std::string> const& TSuitePath::GetNames() const
{
    return m_Names;
}
//---------------------------------------------------------------------------
std::string const& TSuitePath::GetPath() const
{
    return m_Path;
}
//---------------------------------------------------------------------------
bool TSuitePath::IsValid() const
{
    return m_Valid;
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TTestCase
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TTestCase::TTestCase(TTestMethod method, std::string const& name)
    : m_Method(method),
      m_Name(name)
{
}
//---------------------------------------------------------------------------
void TTestCase::DoTest()
{
    m_Method();
}
//---------------------------------------------------------------------------
std::string const& TTestCase::GetName() const
{
    return m_Name;
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TTestCaseRecord
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TTestCaseRecord::TTestCaseRecord()
    : DurationSeconds(0.0),
      Outcome(TTestOutcome::Pass)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TTestGroupBase
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTestGroupBase::TTestGroupBase(std::string const& name)
    : inherited(),
      m_TestCallbacksFound(false),
      m_GuardedTestCase(nullptr),
      m_WorkerCatchCrashes(false),
      m_WorkerState(WorkerIdle),
      m_WorkerTestCase(nullptr),
      m_WorkerThreadID(0),
      m_QualifiedName(name),
      m_ExceptionExpected(false),
      m_TestFailedCheck(false),
      m_Name(name),
      m_RunObserver(nullptr)
{
}
//---------------------------------------------------------------------------
__fastcall TTestGroupBase::TTestGroupBase(TSuitePath const& suitePath, std::string const& name)
    : inherited(),
      m_TestCallbacksFound(false),
      m_GuardedTestCase(nullptr),
      m_WorkerCatchCrashes(false),
      m_WorkerState(WorkerIdle),
      m_WorkerTestCase(nullptr),
      m_WorkerThreadID(0),
      m_ExceptionExpected(false),
      m_TestFailedCheck(false),
      m_Name(name),
      m_RunObserver(nullptr)
{
    // Not checked here: TTestHandler::Initialize() rejects an invalid path (see TSuitePath).
    SetSuitePath(suitePath.GetPath());
}
//---------------------------------------------------------------------------
__fastcall TTestGroupBase::~TTestGroupBase()
{
    DeleteTestCallbacks();
}
//---------------------------------------------------------------------------
void TTestGroupBase::AddRegisteredTest(ITestCase* testCase)
{
    std::unique_ptr<ITestCase> owned(testCase);
    m_RegisteredTests.push_back(owned.get());
    owned.release();
}
//---------------------------------------------------------------------------
void TTestGroupBase::AssertEqualsMem(void const* expected, void const* actual, std::size_t size,
    std::string const& method, int line, TCheckMessage const& msg)
{
    std::string const failure = MemoryFailure(expected, actual, size, true);
    if (!failure.empty())
        throw TExceptEqualsMem(method, line, failure, msg.Text);
}
//---------------------------------------------------------------------------
void TTestGroupBase::AssertFalse(bool testVal, std::string const& method, int line, TCheckMessage const& msg)
{
    if (testVal)
        throw TExceptFalse(method, line, msg.Text);
}
//---------------------------------------------------------------------------
void TTestGroupBase::AssertNear(float expected, float actual, float tolerance, std::string const& method, int line,
    TCheckMessage const& msg)
{
    float difference = 0;

    if (!IsNear(expected, actual, tolerance, difference))
    {
        throw TExceptEquals(method, line, NearExpectedText(expected, tolerance), NearActualText(actual, difference),
            msg.Text);
    }
}
//---------------------------------------------------------------------------
void TTestGroupBase::AssertNear(double expected, double actual, double tolerance, std::string const& method, int line,
    TCheckMessage const& msg)
{
    double difference = 0;

    if (!IsNear(expected, actual, tolerance, difference))
    {
        throw TExceptEquals(method, line, NearExpectedText(expected, tolerance), NearActualText(actual, difference),
            msg.Text);
    }
}
//---------------------------------------------------------------------------
void TTestGroupBase::AssertNoThrow(TTestMethod callable, std::string const& method, int line,
    TCheckMessage const& msg)
{
    std::string const failure = NoThrowFailure(callable);
    if (!failure.empty())
        throw TExceptNoThrow(method, line, failure, msg.Text);
}
//---------------------------------------------------------------------------
void TTestGroupBase::AssertNotEqualsMem(void const* expected, void const* actual, std::size_t size,
    std::string const& method, int line, TCheckMessage const& msg)
{
    std::string const failure = MemoryFailure(expected, actual, size, false);
    if (!failure.empty())
        throw TExceptNotEqualsMem(method, line, failure, msg.Text);
}
//---------------------------------------------------------------------------
void TTestGroupBase::AssertNotNear(float expected, float actual, float tolerance, std::string const& method, int line,
    TCheckMessage const& msg)
{
    float difference = 0;

    if (IsNear(expected, actual, tolerance, difference))
        throw TExceptNotEquals(method, line, NotNearText(actual, difference, tolerance), msg.Text);
}
//---------------------------------------------------------------------------
void TTestGroupBase::AssertNotNear(double expected, double actual, double tolerance, std::string const& method,
    int line, TCheckMessage const& msg)
{
    double difference = 0;

    if (IsNear(expected, actual, tolerance, difference))
        throw TExceptNotEquals(method, line, NotNearText(actual, difference, tolerance), msg.Text);
}
//---------------------------------------------------------------------------
void TTestGroupBase::AssertTrue(bool testVal, std::string const& method, int line, TCheckMessage const& msg)
{
    if (!testVal)
        throw TExceptTrue(method, line, msg.Text);
}
//---------------------------------------------------------------------------
bool TTestGroupBase::BeginWorkerRecording(bool finishing)
{
    if (::GetCurrentThreadId() != m_WorkerThreadID)
        return true;

    return ::InterlockedCompareExchange(&m_WorkerState, finishing ? WorkerFinished : WorkerRecording,
        WorkerRunning) == WorkerRunning;
}
//---------------------------------------------------------------------------
bool TTestGroupBase::CallHook(THook hook, ITestCase* testCase, THookError& error)
{
    try
    {
        switch (hook)
        {
            case THook::SetUpGroup:
                SetUp_Group();
                break;

            case THook::SetUpTest:
                SetUp_Test(*testCase);
                break;

            case THook::TearDownGroup:
                TearDown_Group();
                break;

            case THook::TearDownTest:
                TearDown_Test(*testCase);
                break;
        }

        return true;
    }
    catch (std::exception const& stdEx)
    {
        error.Description = DescribeException(stdEx);
        error.UnknownException = false;
    }
    catch (Exception& rtlEx)
    {
        error.Description = DescribeException(rtlEx);
        error.UnknownException = false;
    }
    catch (...)
    {
        error.Description = DescribeUnknownException();
        error.UnknownException = true;
    }

    return false;
}
//---------------------------------------------------------------------------
void TTestGroupBase::CheckEqualsMem(void const* expected, void const* actual, std::size_t size,
    std::string const& method, int line, TCheckMessage const& msg)
{
    std::string const failure = MemoryFailure(expected, actual, size, true);
    if (!failure.empty())
        SetTestFailedCheck(method, line, failure + ". " + msg.Text);
}
//---------------------------------------------------------------------------
void TTestGroupBase::CheckFalse(bool testVal, std::string const& method, int line, TCheckMessage const& msg)
{
    if (testVal)
        SetTestFailedCheck(method, line, "Expected false but was true: \"" + msg.Text + "\"");
}
//---------------------------------------------------------------------------
void TTestGroupBase::CheckNear(float expected, float actual, float tolerance, std::string const& method, int line,
    TCheckMessage const& msg)
{
    float difference = 0;

    if (!IsNear(expected, actual, tolerance, difference))
    {
        SetTestFailedCheck(method, line, NearExpectedText(expected, tolerance), NearActualText(actual, difference),
            msg.Text);
    }
}
//---------------------------------------------------------------------------
void TTestGroupBase::CheckNear(double expected, double actual, double tolerance, std::string const& method, int line,
    TCheckMessage const& msg)
{
    double difference = 0;

    if (!IsNear(expected, actual, tolerance, difference))
    {
        SetTestFailedCheck(method, line, NearExpectedText(expected, tolerance), NearActualText(actual, difference),
            msg.Text);
    }
}
//---------------------------------------------------------------------------
void TTestGroupBase::CheckNoThrow(TTestMethod callable, std::string const& method, int line,
    TCheckMessage const& msg)
{
    std::string const failure = NoThrowFailure(callable);
    if (!failure.empty())
        SetTestFailedCheck(method, line, failure + ". " + msg.Text);
}
//---------------------------------------------------------------------------
void TTestGroupBase::CheckNotEqualsMem(void const* expected, void const* actual, std::size_t size,
    std::string const& method, int line, TCheckMessage const& msg)
{
    std::string const failure = MemoryFailure(expected, actual, size, false);
    if (!failure.empty())
        SetTestFailedCheck(method, line, failure + ". " + msg.Text);
}
//---------------------------------------------------------------------------
void TTestGroupBase::CheckNotNear(float expected, float actual, float tolerance, std::string const& method, int line,
    TCheckMessage const& msg)
{
    float difference = 0;

    if (IsNear(expected, actual, tolerance, difference))
        SetTestFailedCheckNotEquals(method, line, NotNearText(actual, difference, tolerance), msg.Text);
}
//---------------------------------------------------------------------------
void TTestGroupBase::CheckNotNear(double expected, double actual, double tolerance, std::string const& method,
    int line, TCheckMessage const& msg)
{
    double difference = 0;

    if (IsNear(expected, actual, tolerance, difference))
        SetTestFailedCheckNotEquals(method, line, NotNearText(actual, difference, tolerance), msg.Text);
}
//---------------------------------------------------------------------------
void TTestGroupBase::CheckTrue(bool testVal, std::string const& method, int line, TCheckMessage const& msg)
{
    if (!testVal)
        SetTestFailedCheck(method, line, "Expected true but was false: \"" + msg.Text + "\"");
}
//---------------------------------------------------------------------------
unsigned int TTestGroupBase::CountTests(ITestFilter const* filter)
{
    TestCallbackList const& tests = GetTestCallbackList();

    if (filter == nullptr)
        return static_cast<unsigned int>(tests.size());

    unsigned int count = 0;

    for (TestCallbackList::const_iterator it = tests.begin(); it != tests.end(); ++it)
    {
        if (filter->Matches(m_QualifiedName + "." + (*it)->GetName()))
            ++count;
    }

    return count;
}
//---------------------------------------------------------------------------
void TTestGroupBase::DeleteTestCallbacks()
{
    for (TestCallbackList::iterator it = m_TestCallbacks.begin(); it != m_TestCallbacks.end(); ++it)
        delete *it;

    for (TestCallbackList::iterator it = m_RegisteredTests.begin(); it != m_RegisteredTests.end(); ++it)
        delete *it;

    m_TestCallbacks.clear();
    m_RegisteredTests.clear();
}
//---------------------------------------------------------------------------
void TTestGroupBase::EndWorkerRecording()
{
    if (::GetCurrentThreadId() == m_WorkerThreadID)
        ::InterlockedExchange(&m_WorkerState, WorkerRunning);
}
//---------------------------------------------------------------------------
bool TTestGroupBase::ExceptionTypeExpected() const
{
    return m_ExpectedExceptionMatcher.get() != nullptr;
}
//---------------------------------------------------------------------------
void TTestGroupBase::Fail(std::string const& method, int line, TCheckMessage const& msg)
{
    throw TExceptFail(method, line, msg.Text);
}
//---------------------------------------------------------------------------
std::vector<TPublishedTest> TTestGroupBase::FindPublishedTests(TClass groupClass)
{
    std::vector<TClass> classes; // The group class first, then its ancestors

    for (TClass cls = groupClass; (cls != nullptr) && (cls != __classid(TTestGroupBase));
         cls = cls->ClassParent())
    {
        classes.push_back(cls);
    }

    std::vector<TPublishedTest> tests;

    for (std::size_t classIndex = classes.size(); classIndex > 0; --classIndex)
    {
        TClass const cls = classes[classIndex - 1];
        std::vector<TPublishedMethod> const methods = PublishedMethods(cls);

        for (std::size_t methodIndex = 0; methodIndex < methods.size(); ++methodIndex)
        {
            TPublishedMethod const& method = methods[methodIndex];

            if (!IsTestMethodName(method.Name))
                continue;

            for (std::size_t earlierIndex = 0; earlierIndex < methodIndex; ++earlierIndex)
            {
                if (methods[earlierIndex].Name == method.Name)
                    throw TExceptDuplicateTestName(ToUTF8(cls->ClassName()), method.Name);
            }

            bool alreadyListed = false;

            for (std::size_t testIndex = 0; testIndex < tests.size(); ++testIndex)
            {
                if (tests[testIndex].MethodName == method.Name)
                {
                    tests[testIndex].Code = method.Code;
                    alreadyListed = true;
                    break;
                }
            }

            if (alreadyListed)
                continue;

            TPublishedTest test;
            test.MethodName = method.Name;
            test.TestName = method.Name.substr(TestMethodPrefixLength);
            test.Code = method.Code;
            tests.push_back(test);
        }
    }

    return tests;
}
//---------------------------------------------------------------------------
void TTestGroupBase::FinishTest(ITestCase& testCase, TTestOutcome outcome, std::string const& detail,
    double durationMs)
{
    // A test abandoned on a worker thread, which ends after all, gets no record or log line: it already has its
    // timeout failure.
    if (!BeginWorkerRecording(true))
        return;

    std::string const testFullName = m_QualifiedName + "." + testCase.GetName();
    std::string status;
    std::string tag;
    TLogKind kind = TLogKind::Pass;

    switch (outcome)
    {
        case TTestOutcome::Pass:
            m_Results.SuccessCount++;
            status = "passed";
            break;

        case TTestOutcome::Fail:
            m_Results.FailedCount++;
            status = "failed";
            tag = "***Test failed";
            kind = TLogKind::Fail;
            break;

        case TTestOutcome::Skip:
            m_Results.SkippedCount++;
            status = "skipped";
            tag = "***Test skipped";
            kind = TLogKind::Skip;
            break;
    }

    if (!tag.empty())
    {
        std::string const rest = ": \"" + testFullName + "\"" + (detail.empty() ? std::string() : (": " + detail));

        m_Results.Messages.push_back(tag + rest);
        Log(TConsole::Colorize(tag, kind) + rest);
    }

    // A failed test's record also gets the Check* failures it collected, ahead of 'detail', since those were only
    // logged as they happened; the logged line above leaves them out so they aren't printed twice.
    std::string recordDetail = detail;

    if ((outcome == TTestOutcome::Fail) && !m_CheckFailureMessages.empty())
    {
        std::string checkFailures;

        for (std::vector<std::string>::const_iterator it = m_CheckFailureMessages.begin();
             it != m_CheckFailureMessages.end(); ++it)
        {
            checkFailures += (checkFailures.empty() ? std::string() : std::string("\n")) + *it;
        }

        recordDetail = detail.empty() ? checkFailures : (checkFailures + "\n" + detail);
    }

    TTestCaseRecord record;
    record.GroupName = m_QualifiedName;
    record.SuitePath = m_SuitePath;
    record.TestName = testCase.GetName();
    record.DurationSeconds = durationMs / 1000.0;
    record.Outcome = outcome;
    record.Message = recordDetail;
    m_Results.CaseRecords.push_back(record);

    Log("Finished test: \"" + testFullName + "\" - " + TConsole::Colorize(status, kind) + " (" +
        FormatDurationMs(durationMs) + ")");
}
//---------------------------------------------------------------------------
std::string const& TTestGroupBase::GetQualifiedTestGroupName() const
{
    return m_QualifiedName;
}
//---------------------------------------------------------------------------
std::string const& TTestGroupBase::GetSuitePath() const
{
    return m_SuitePath;
}
//---------------------------------------------------------------------------
TTestGroupBase::TestCallbackList const& TTestGroupBase::GetTestCallbackList()
{
    if (m_TestCallbacksFound)
        return m_TestCallbacks;

    std::vector<TPublishedTest> const tests = FindPublishedTests(GetTestClass());

    // Published names are already unique (see FindPublishedTests()); a registered one must not repeat any name.
    std::set<std::string> names;

    for (std::vector<TPublishedTest>::const_iterator it = tests.begin(); it != tests.end(); ++it)
        names.insert(it->TestName);

    for (TestCallbackList::const_iterator it = m_RegisteredTests.begin(); it != m_RegisteredTests.end(); ++it)
    {
        if (!names.insert((*it)->GetName()).second)
            throw TExceptDuplicateTestName::Registered(m_QualifiedName, (*it)->GetName());
    }

    // So push_back() can't throw and leak a test case
    m_TestCallbacks.reserve(tests.size() + m_RegisteredTests.size());

    for (std::vector<TPublishedTest>::const_iterator it = tests.begin(); it != tests.end(); ++it)
    {
        ITestCase* const testCase = new TTestCase(MakeTestMethod(it->Code, this), it->TestName);
        m_TestCallbacks.push_back(testCase);
    }

    // The registered tests follow the published ones, in the order they were registered.
    m_TestCallbacks.insert(m_TestCallbacks.end(), m_RegisteredTests.begin(), m_RegisteredTests.end());
    m_RegisteredTests.clear();

    m_TestCallbacksFound = true;
    return m_TestCallbacks;
}
//---------------------------------------------------------------------------
TClass TTestGroupBase::GetTestClass()
{
    return ClassType();
}
//---------------------------------------------------------------------------
std::string const& TTestGroupBase::GetTestGroupName() const
{
    return m_Name;
}
//---------------------------------------------------------------------------
void TTestGroupBase::Log(std::string const& msg)
{
    if (m_RunObserver != nullptr)
    {
        m_RunObserver->OnLog(TConsole::StripColor(msg) + "\n");
        return;
    }

    std::cout << msg << std::endl;
}
//---------------------------------------------------------------------------
void TTestGroupBase::LogAppend(std::string const& msg)
{
    if (m_RunObserver != nullptr)
    {
        m_RunObserver->OnLog(TConsole::StripColor(msg));
        return;
    }

    std::cout << msg;
}
//---------------------------------------------------------------------------
void TTestGroupBase::ReportCrashedTest(ITestCase& testCase, std::string const& description, bool abortRun)
{
    // On a worker thread, as for its outcome (see FinishTest()), not once the test has been abandoned.
    if (!BeginWorkerRecording(true))
        return;

    std::string const testFullName = m_QualifiedName + "." + testCase.GetName();
    std::string const detail = "Test crashed: " + description + ".";
    std::string const tag = "***Test failed";
    std::string const rest = ": \"" + testFullName + "\": " + detail;

    m_Results.FailedCount++;
    m_Results.Messages.push_back(tag + rest);

    TTestCaseRecord record;
    record.GroupName = m_QualifiedName;
    record.SuitePath = m_SuitePath;
    record.TestName = testCase.GetName();
    record.DurationSeconds = 0.0;
    record.Outcome = TTestOutcome::Fail;
    record.Message = detail;
    m_Results.CaseRecords.push_back(record);

    Log(TConsole::Colorize(tag, TLogKind::Fail) + rest);

    if (abortRun)
    {
        m_Results.Crashed = true;
        Log("!!CRASH!!: \"" + testFullName + "\" crashed in a way that isn't safe to continue past (" + description +
            "); abandoning the remaining run.");
        return;
    }

    Log("!!CRASH!!: \"" + testFullName + "\" crashed (" + description + "); continuing with the next test.");
}
//---------------------------------------------------------------------------
void TTestGroupBase::ReportTimedOutTest(ITestCase& testCase, unsigned int testTimeoutSeconds)
{
    std::string const testFullName = m_QualifiedName + "." + testCase.GetName();
    std::string const detail = "Test exceeded its " + UIntToString(testTimeoutSeconds) +
        " second timeout and was abandoned.";
    std::string const tag = "***Test failed";
    std::string const rest = ": \"" + testFullName + "\": " + detail;

    m_Results.FailedCount++;
    m_Results.Messages.push_back(tag + rest);

    TTestCaseRecord record;
    record.GroupName = m_QualifiedName;
    record.SuitePath = m_SuitePath;
    record.TestName = testCase.GetName();
    record.DurationSeconds = static_cast<double>(testTimeoutSeconds);
    record.Outcome = TTestOutcome::Fail;
    record.Message = detail;
    m_Results.CaseRecords.push_back(record);

    Log(TConsole::Colorize(tag, TLogKind::Fail) + rest);
    Log("!!TIMEOUT!!: \"" + testFullName + "\" exceeded " + UIntToString(testTimeoutSeconds) +
        " second(s); abandoning the remaining run.");
}
//---------------------------------------------------------------------------
void TTestGroupBase::ResetTestFailedOneOrMoreChecks()
{
    m_TestFailedCheck = false;
    m_CheckFailureMessages.clear();
}
//---------------------------------------------------------------------------
TTestResults const& TTestGroupBase::Results() const
{
    return m_Results;
}
//---------------------------------------------------------------------------
void TTestGroupBase::Run(ITestFilter const* filter, TOptional<unsigned int> shuffleSeed,
    TOptional<unsigned int> testTimeoutSeconds, bool catchCrashes)
{
    m_Results = TTestResults();

    THookError error;
    std::string failureDetail;

    if (!CallHook(THook::SetUpGroup, nullptr, error))
    {
        failureDetail = "SetUp_Group() exception: " + error.Description;
        Log("!!ERROR!!: SetUp_Group() exception for group \"" + m_QualifiedName + "\": " + error.Description);
    }

    RunTests(failureDetail, filter, shuffleSeed, testTimeoutSeconds, catchCrashes);

    Log("[" + GetUTCTimeISO8601() + "] Tearing down group: \"" + m_QualifiedName + "\"");

    if (!CallHook(THook::TearDownGroup, nullptr, error))
        ThrowFatalError("TearDown_Group()", "group \"" + m_QualifiedName + "\"", error);
}
//---------------------------------------------------------------------------
void TTestGroupBase::RunCatchingCrashes(ITestCase& testCase, bool catchCrashes)
{
    if (!catchCrashes)
    {
        Test(testCase);
        return;
    }

    m_GuardedTestCase = &testCase;
    TCrashGuardResult const result = TCrashGuard::Run(&RunGuardedTest);

    if (result.Crashed)
        ReportCrashedTest(testCase, result.Description, result.ShouldAbortRun);
}
//---------------------------------------------------------------------------
void __fastcall TTestGroupBase::RunGuardedTest()
{
    Test(*m_GuardedTestCase);
}
//---------------------------------------------------------------------------
void __fastcall TTestGroupBase::RunTestOnWorker()
{
    // Test() records the test's outcome itself, whatever the test throws; only a TExceptFatalError (a failing
    // SetUp_Test() or TearDown_Test()) is meant to get out of it, and anything else that does is turned into one
    // that gives the same exit code, since this thread can't hand the original over (C++03 has no
    // std::exception_ptr, and an RTL exception is freed when its handler ends).
    try
    {
        RunCatchingCrashes(*m_WorkerTestCase, m_WorkerCatchCrashes);
    }
    catch (TExceptFatalError const& fatalEx)
    {
        m_WorkerFatalError.reset(new TExceptFatalError(fatalEx));
    }
    catch (std::exception const& stdEx)
    {
        m_WorkerFatalError.reset(new TExceptFatalError(DescribeException(stdEx), false));
    }
    catch (Exception& rtlEx)
    {
        m_WorkerFatalError.reset(new TExceptFatalError(DescribeException(rtlEx), false));
    }
    catch (...)
    {
        m_WorkerFatalError.reset(new TExceptFatalError("Unknown", true));
    }
}
//---------------------------------------------------------------------------
void TTestGroupBase::RunTests(std::string const& failureDetail, ITestFilter const* filter,
    TOptional<unsigned int> shuffleSeed, TOptional<unsigned int> testTimeoutSeconds, bool catchCrashes)
{
    TestCallbackList const& tests = GetTestCallbackList();
    unsigned int const testCount = CountTests(filter);
    std::vector<std::size_t> order;

    if (shuffleSeed.HasValue())
    {
        order = ShuffledIndices(tests.size(), shuffleSeed.Value());
    }
    else
    {
        order.resize(tests.size());

        for (std::size_t i = 0; i < order.size(); ++i)
            order[i] = i;
    }

    Log("Running " + UIntToString(testCount) + " test" + ((testCount == 1) ? "." : "s."));

    for (std::vector<std::size_t>::const_iterator it = order.begin(); it != order.end(); ++it)
    {
        ITestCase& testCase = *tests[*it];

        if ((filter != nullptr) && !filter->Matches(m_QualifiedName + "." + testCase.GetName()))
            continue;

        if (m_RunObserver != nullptr)
        {
            if (m_RunObserver->StopRequested())
            {
                m_Results.Stopped = true;
                break;
            }

            m_RunObserver->OnTestStarted(m_QualifiedName, testCase.GetName());
        }

        std::size_t const recordsBefore = m_Results.CaseRecords.size();
        bool finished = true;

        if (failureDetail.empty())
        {
            finished = RunWithTimeout(testCase, testTimeoutSeconds, catchCrashes);
        }
        else
        {
            Log("Running test: " + m_QualifiedName + "." + testCase.GetName());
            ResetTestFailedOneOrMoreChecks();
            FinishTest(testCase, TTestOutcome::Fail, failureDetail, 0.0);
        }

        if ((m_RunObserver != nullptr) && (m_Results.CaseRecords.size() > recordsBefore))
            m_RunObserver->OnTestFinished(m_Results.CaseRecords.back());

        if (!finished)
        {
            m_Results.TimedOut = true;
            break;
        }

        if (m_Results.Crashed)
            break;
    }

    Log("Done. " + TConsole::Colorize("Succeeded: " + UIntToString(m_Results.SuccessCount), TLogKind::Pass) + ", " +
        TConsole::Colorize("failed: " + UIntToString(m_Results.FailedCount), TLogKind::Fail) + ", " +
        TConsole::Colorize("skipped: " + UIntToString(m_Results.SkippedCount), TLogKind::Skip));
}
//---------------------------------------------------------------------------
bool TTestGroupBase::RunWithTimeout(ITestCase& testCase, TOptional<unsigned int> testTimeoutSeconds,
    bool catchCrashes)
{
    if (!testTimeoutSeconds.HasValue())
    {
        RunCatchingCrashes(testCase, catchCrashes);
        return true;
    }

    m_WorkerCatchCrashes = catchCrashes;
    m_WorkerTestCase = &testCase;
    m_WorkerFatalError.reset();

    // The closure in a statement of its own: as an argument to the constructor, bcc32 Release (inlining on) failed
    // with an internal compiler error (F1004) once RunCatchingCrashes() existed.
    TThreadMethod const work = &this->RunTestOnWorker;
    std::unique_ptr<TTestWorkerThread> worker(new TTestWorkerThread(work));
    m_WorkerThreadID = worker->ThreadID;
    m_WorkerState = WorkerRunning;
    worker->Start();

    if (!WaitForWorker(*worker, testTimeoutSeconds.Value()) && AbandonWorker(m_WorkerState))
    {
        // It may still be running, so its TThread object is never freed, and m_WorkerThreadID keeps its ID, which
        // keeps it from changing the results.
        static_cast<void>(worker.release());
        ReportTimedOutTest(testCase, testTimeoutSeconds.Value());
        return false;
    }

    // Ended, or had recorded its outcome as the time ran out and is about to.
    worker->WaitFor();
    worker.reset();
    m_WorkerThreadID = 0;
    m_WorkerState = WorkerIdle;

    if (m_WorkerFatalError.get() != nullptr)
    {
        TExceptFatalError const fatalEx(*m_WorkerFatalError);
        m_WorkerFatalError.reset();
        throw fatalEx;
    }

    return true;
}
//---------------------------------------------------------------------------
void TTestGroupBase::ScoreExpectedException(bool typeMatches, std::string const& message,
    std::string const& description, TTestOutcome& outcome, std::string& detail)
{
    if (!typeMatches)
    {
        outcome = TTestOutcome::Fail;
        detail = "expected exception type was not thrown (caught a different exception): " + description;
        return;
    }

    if (!m_ExpectedExceptionMessage.empty() && (message.find(m_ExpectedExceptionMessage) == std::string::npos))
    {
        outcome = TTestOutcome::Fail;
        detail = "expected exception message to contain \"" + m_ExpectedExceptionMessage + "\" but caught: " +
            description;
        return;
    }

    outcome = TTestOutcome::Pass;
}
//---------------------------------------------------------------------------
/*
    TTestGroupBase::SetExceptionExpected

    Call with a value of true in a test where an exception is expected.
*/
void TTestGroupBase::SetExceptionExpected(bool expected, std::string const& method, int line,
    TCheckMessage const& msg)
{
    m_ExceptionExpected = expected;
    m_ExceptionExpectedText = FormatLocation(method, line) + ": " + msg.Text;
    m_ExpectedExceptionMessage.clear();
    m_ExpectedExceptionMatcher.reset();
}
//---------------------------------------------------------------------------
void TTestGroupBase::SetRunObserver(ITestRunObserver* observer)
{
    m_RunObserver = observer;
}
//---------------------------------------------------------------------------
void TTestGroupBase::SetSuitePath(std::string const& suitePath)
{
    m_SuitePath = suitePath;
    m_QualifiedName = suitePath.empty() ? m_Name : (suitePath + "." + m_Name);
}
//---------------------------------------------------------------------------
void TTestGroupBase::SetTestFailedCheck(std::string const& method, int line, std::string const& msg)
{
    // Nor does an abandoned test's Check* failure, since the run's results may be being read by now.
    if (!BeginWorkerRecording(false))
        return;

    m_TestFailedCheck = true;

    std::string const checkFailure = "Check failed for: \"" + BareMethodName(method) + "\" (" + IntToString(line) +
        "): " + msg;
    m_CheckFailureMessages.push_back(checkFailure);

    std::string const finalMsg = "  **" + checkFailure;
    m_Results.Messages.push_back(finalMsg);
    EndWorkerRecording();
    Log(finalMsg);
}
//---------------------------------------------------------------------------
void TTestGroupBase::SetTestFailedCheck(std::string const& method, int line, std::string const& expected,
    std::string const& actual, std::string const& msg)
{
    SetTestFailedCheck(method, line, "Expected \"" + expected + "\" but was \"" + actual + "\". " + msg);
}
//---------------------------------------------------------------------------
void TTestGroupBase::SetTestFailedCheckNotEquals(std::string const& method, int line, std::string const& value,
    std::string const& msg)
{
    SetTestFailedCheck(method, line, "Both values equal: \"" + value + "\". " + msg);
}
//---------------------------------------------------------------------------
void TTestGroupBase::SetTestFailedCheckTextPart(std::string const& method, int line, std::string const& text,
    std::string const& relation, std::string const& part, bool ignoreCase, std::string const& msg)
{
    SetTestFailedCheck(method, line, "Expected \"" + text + "\" " + relation + " \"" + part + "\"" +
        (ignoreCase ? " (ignoring case)" : "") + ". " + msg);
}
//---------------------------------------------------------------------------
/*
    TTestGroupBase::SetUp_Group

    Called once before the group's first test
*/
void TTestGroupBase::SetUp_Group()
{
    // The child group can optionally override this method
}
//---------------------------------------------------------------------------
/*
    TTestGroupBase::SetUp_Test

    Called just before each test
*/
void TTestGroupBase::SetUp_Test(ITestCase& /*testCase*/)
{
    // The child group can optionally override this method
}
//---------------------------------------------------------------------------
void TTestGroupBase::Skip(std::string const& method, int line, TCheckMessage const& reason)
{
    throw TExceptSkipped(method, line, reason.Text);
}
//---------------------------------------------------------------------------
/*
    TTestGroupBase::TearDown_Group

    Called once after the group's last test
*/
void TTestGroupBase::TearDown_Group()
{
    // The child group can optionally override this method
}
//---------------------------------------------------------------------------
/*
    TTestGroupBase::TearDown_Test

    Called just after each test
*/
void TTestGroupBase::TearDown_Test(ITestCase& /*testCase*/)
{
    // The child group can optionally override this method
}
//---------------------------------------------------------------------------
/*
    TTestGroupBase::Test

    Runs one test between SetUp_Test() and TearDown_Test(), and records its outcome: passed, failed (a
    TTestException, e.g. a failed Assert* method or Fail(), or a Check* failure anywhere from SetUp_Test() to
    TearDown_Test()), or skipped (Skip(), even after a Check* failure).

    An exception the test didn't expect fails only that test, with an "Unexpected exception: " detail describing
    it; TearDown_Test() and the rest of the run go on as usual. While an exception is expected (see
    SetExceptionExpected()), one of the expected type (and message) passes the test, and anything else, including
    no exception, fails it. A TTestException is never taken for the expected exception: Fail() and a failed Assert*
    method fail the test even then. Each exception is caught exactly once, never rethrown, since an RTL exception
    object is freed when the first handler that catches it ends.

    An exception from SetUp_Test() or TearDown_Test() is fatal: it's logged as a "!!FATAL ERROR!!" and ends the run
    with a TExceptFatalError, without a record for the test.
*/
void TTestGroupBase::Test(ITestCase& testCase)
{
    std::string const testFullName = m_QualifiedName + "." + testCase.GetName();
    TStopwatch stopwatch = TStopwatch::StartNew();

    Log("Running test: " + testFullName);

    SetExceptionExpected(false, "", 0);
    ResetTestFailedOneOrMoreChecks();

    THookError error;

    if (!CallHook(THook::SetUpTest, &testCase, error))
        ThrowFatalError("SetUp_Test()", "\"" + testFullName + "\"", error);

    TTestOutcome outcome = TTestOutcome::Pass;
    std::string detail;

    try
    {
        testCase.DoTest();

        if (m_ExceptionExpected)
        {
            outcome = TTestOutcome::Fail;
            detail = TExceptExpected(m_ExceptionExpectedText).what();
        }
    }
    catch (TExceptSkipped const& skippedEx)
    {
        outcome = TTestOutcome::Skip;
        detail = skippedEx.what();
    }
    catch (TTestException const& testEx)
    {
        outcome = TTestOutcome::Fail;
        detail = testEx.what();
    }
    catch (std::exception const& stdEx)
    {
        if (m_ExceptionExpected)
        {
            bool const typeMatches = ExceptionTypeExpected() ? m_ExpectedExceptionMatcher->MatchesStd(stdEx) : true;
            ScoreExpectedException(typeMatches, stdEx.what(), DescribeException(stdEx), outcome, detail);
        }
        else
        {
            outcome = TTestOutcome::Fail;
            detail = "Unexpected exception: " + DescribeException(stdEx);
        }
    }
    catch (Exception& rtlEx)
    {
        if (m_ExceptionExpected)
        {
            bool const typeMatches = ExceptionTypeExpected() ? m_ExpectedExceptionMatcher->MatchesRTL(rtlEx) : true;
            ScoreExpectedException(typeMatches, RTLExceptionMessage(rtlEx), DescribeException(rtlEx), outcome, detail);
        }
        else
        {
            outcome = TTestOutcome::Fail;
            detail = "Unexpected exception: " + DescribeException(rtlEx);
        }
    }
    catch (...)
    {
        if (!m_ExceptionExpected)
        {
            outcome = TTestOutcome::Fail;
            detail = "Unexpected exception: " + DescribeUnknownException();
        }
        else if (ExceptionTypeExpected() && !m_ExpectedExceptionMatcher->MatchesCurrentOther())
        {
            outcome = TTestOutcome::Fail;
            detail = "expected exception type was not thrown (caught a different exception): " +
                DescribeUnknownException();
        }
        else if (!m_ExpectedExceptionMessage.empty())
        {
            // Only RTL exceptions and std::exceptions have a message to check.
            outcome = TTestOutcome::Fail;
            detail = "expected exception message to contain \"" + m_ExpectedExceptionMessage +
                "\" but caught an exception without a message: " + DescribeUnknownException();
        }
    }

    if (!CallHook(THook::TearDownTest, &testCase, error))
        ThrowFatalError("TearDown_Test()", "\"" + testFullName + "\"", error);

    // Every path that would otherwise pass the test, an expected exception arriving included, gets here, so none
    // can overlook an earlier Check* failure. The record gets the Check* failures themselves; see FinishTest().
    if ((outcome == TTestOutcome::Pass) && TestFailedOneOrMoreChecks())
        outcome = TTestOutcome::Fail;

    FinishTest(testCase, outcome, detail, ElapsedMs(stopwatch));
}
//---------------------------------------------------------------------------
bool TTestGroupBase::TestFailedOneOrMoreChecks()
{
    return m_TestFailedCheck;
}
//---------------------------------------------------------------------------
void TTestGroupBase::ThrowFatalError(std::string const& hookName, std::string const& subject,
    THookError const& error)
{
    Log("!!FATAL ERROR!!: " + hookName + " exception for " + subject);
    throw TExceptFatalError(hookName + " exception for " + subject + ": " + error.Description, error.UnknownException);
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TTestResults
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TTestResults::TTestResults()
    : Crashed(false),
      FailedCount(0),
      SkippedCount(0),
      Stopped(false),
      SuccessCount(0),
      TimedOut(false)
{
}
//---------------------------------------------------------------------------
void TTestResults::AddMessages(MsgList const& list)
{
    if (&list == &Messages)
        return;

    for (MsgList::const_iterator it = list.begin(); it != list.end(); ++it)
        Messages.push_back(*it);
}
//---------------------------------------------------------------------------


// /////// Throws and NoThrow ///////////////////////

//---------------------------------------------------------------------------
void CallOutOfLine(void (*invoke)(void*), void* callable)
{
    invoke(callable);
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
