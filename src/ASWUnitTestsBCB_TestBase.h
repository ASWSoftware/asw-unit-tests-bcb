/* **************************************************************************
ASWUnitTestsBCB_TestBase.h
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

#ifndef ASWUnitTestsBCB_TestBaseH
#define ASWUnitTestsBCB_TestBaseH
//---------------------------------------------------------------------------
#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.hpp>
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Exception.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
#include "ASWUnitTestsBCB_Values.h"
//---------------------------------------------------------------------------

// The location arguments of an assertion, Fail(), Skip() or SetExceptionExpected(): the calling method's name and
// the line, e.g. CheckEquals(1, count, ASWBCB_HERE) or CheckEquals(1, count, ASWBCB_HERE, "after Add()"). The same
// as writing "__FUNC__, __LINE__" (or "__func__, __LINE__") explicitly. Failure messages show the bare method
// name with every compiler.
#define ASWBCB_HERE __FUNC__, __LINE__

namespace ASWUnitTestsBCB
{

// A test method, called as a closure on its group: "void __fastcall Test_<Name>()".
typedef void __fastcall (__closure* TTestMethod)();


/////////////////////////////////////////////////////////////////////////////
// TTestOutcome
/////////////////////////////////////////////////////////////////////////////
enum class TTestOutcome
{
    Pass,
    Fail,
    Skip,
};


/////////////////////////////////////////////////////////////////////////////
// TTestCaseRecord
//
// One test's outcome, name, and timing, collected in TTestResults::CaseRecords.
// Deliberately format-agnostic (no ANSI color codes, no XML, etc.), so a
// caller can build a structured report from it.
//
// GroupName is the group's qualified name (see
// TTestGroupBase::GetQualifiedTestGroupName()), which is just the group's
// own name when it isn't in a suite. SuitePath is the suite part on its
// own, so a caller can split the two even when a name contains '.'.
/////////////////////////////////////////////////////////////////////////////
struct TTestCaseRecord
{
    std::string GroupName;
    std::string TestName;
    double DurationSeconds;
    TTestOutcome Outcome;
    std::string Message;   // Failure/skip detail; empty for Pass.
    std::string SuitePath; // Empty when the group isn't in a suite.

    TTestCaseRecord();
};


/////////////////////////////////////////////////////////////////////////////
// TTestResults
//
// Stores test results.
/////////////////////////////////////////////////////////////////////////////
class TTestResults
{
public:
    typedef std::vector<std::string> MsgList;

public:
    bool Crashed; // Set when a test crashed severely enough to abort the run.
    unsigned int FailedCount;
    unsigned int SkippedCount;
    bool Stopped; // Set when ITestRunObserver::StopRequested() ended the run early.
    unsigned int SuccessCount;
    bool TimedOut; // Set when a test ran past its timeout, which ended the run.
    MsgList Messages;
    std::vector<TTestCaseRecord> CaseRecords;

public:
    TTestResults();

    void AddMessages(MsgList const& list);
};


/////////////////////////////////////////////////////////////////////////////
// ITestRunObserver
//
// Optional hooks for following a test run as it happens, e.g. a GUI runner
// updating its display after each test. Set with
// TTestHandler::SetRunObserver(). With none set, nothing changes: output
// goes to std::cout as usual.
//
// Threading: OnTestStarted(), OnTestFinished() and StopRequested() always
// come from the thread that called TTestHandler::Run(). OnLog() does too,
// except under a test timeout (--test-timeout-seconds), where each test
// runs on a worker thread and its own log output comes from there while
// the calling thread waits. The two never call OnLog() at once (unless a
// test times out and its abandoned thread logs later), but an observer
// that must handle it on the calling thread, such as a VCL form, has to
// pass the text on, e.g. with TThread::Queue(), which the waiting thread
// runs.
/////////////////////////////////////////////////////////////////////////////
class ITestRunObserver
{
public:
    virtual ~ITestRunObserver()
    {
    }

    // Receives the text that would otherwise be written to std::cout, including any line break, without the color
    // escape sequences TConsole adds there.
    virtual void OnLog(std::string const& text) = 0;
    // Called once for every test that records an outcome, including a test failed because its group couldn't be
    // created or set up, or because it threw an exception it didn't expect. Not called for a test whose
    // SetUp_Test() or TearDown_Test() threw, since that ends the run.
    virtual void OnTestFinished(TTestCaseRecord const& record) = 0;
    // 'groupName' is the group's qualified name, as in TTestCaseRecord::GroupName.
    virtual void OnTestStarted(std::string const& groupName, std::string const& testName) = 0;
    // Checked before each test and each group. Returning true ends the run there, with the returned
    // TTestResults::Stopped set; a test that's already running is never interrupted.
    virtual bool StopRequested() = 0;
};


/////////////////////////////////////////////////////////////////////////////
// ITestFilter
//
// Chooses the tests a run includes, by their "GroupName.TestName" full
// names, where GroupName is the group's qualified name (see
// TTestGroupBase::GetQualifiedTestGroupName()), so a full name starts with
// the group's suite path. TTestHandler::Run(), TTestHandler::ListTests() and
// TTestGroupBase::Run() take one, where nullptr means every test; the console
// runner's comes from TCLIParser::BuildTestFilter().
/////////////////////////////////////////////////////////////////////////////
class ITestFilter
{
public:
    virtual ~ITestFilter()
    {
    }

    virtual bool Matches(std::string const& fullTestName) const = 0;
};


/////////////////////////////////////////////////////////////////////////////
// ITestCase
//
// Interface for a test case.
/////////////////////////////////////////////////////////////////////////////
class ITestCase
{
private:
    ITestCase(ITestCase const&);            // Not copyable
    ITestCase& operator=(ITestCase const&); // Not copyable

protected:
    ITestCase()
    {
    }

public:
    virtual ~ITestCase()
    {
    }

    virtual void DoTest() = 0;
    virtual std::string const& GetName() const = 0;
};


/////////////////////////////////////////////////////////////////////////////
// TTestCase
//
// A test case that calls a closure, such as a group's published test
// method.
/////////////////////////////////////////////////////////////////////////////
class TTestCase : public ITestCase
{
private:
    typedef ITestCase inherited;

protected:
    TTestMethod m_Method;
    std::string m_Name;

public:
    TTestCase(TTestMethod method, std::string const& name);

    virtual void DoTest();
    virtual std::string const& GetName() const;
};


/////////////////////////////////////////////////////////////////////////////
// Parameterized test cases (see TTestGroupBase::RegisterTestCases())
/////////////////////////////////////////////////////////////////////////////

// A parameterized test method, called as a closure on its group with one row:
// "void __fastcall Test_<Name>(TParam const& row)".
template <class TParam>
struct TParamTestMethod
{
    typedef void __fastcall (__closure* Type)(TParam const&);
};

// Names a row of a parameterized test, e.g. "A" for the test "HexSingleToByte[A]".
template <class TParam>
struct TParamTestNamer
{
    typedef std::string (*Type)(TParam const&);
};

// A test case that calls a closure with its own copy of one row.
template <class TParam>
class TParamTestCase : public ITestCase
{
private:
    typedef ITestCase inherited;

protected:
    typename TParamTestMethod<TParam>::Type m_Method;
    std::string m_Name;
    TParam m_Param;

public:
    TParamTestCase(typename TParamTestMethod<TParam>::Type method, std::string const& name, TParam const& param)
        : m_Method(method),
          m_Name(name),
          m_Param(param)
    {
    }

    virtual void DoTest()
    {
        m_Method(m_Param);
    }

    virtual std::string const& GetName() const
    {
        return m_Name;
    }
};

// A test case that calls a member function of a group with its own copy of one row.
template <class T, class TParam>
class TMemberParamTestCase : public ITestCase
{
private:
    typedef ITestCase inherited;

protected:
    T* m_Group; // Not owned
    void(__fastcall T::* m_Method)(TParam const&);
    std::string m_Name;
    TParam m_Param;

public:
    TMemberParamTestCase(T* group, void(__fastcall T::* method)(TParam const&), std::string const& name,
        TParam const& param)
        : m_Group(group),
          m_Method(method),
          m_Name(name),
          m_Param(param)
    {
    }

    virtual void DoTest()
    {
        (m_Group->*m_Method)(m_Param);
    }

    virtual std::string const& GetName() const
    {
        return m_Name;
    }
};


/////////////////////////////////////////////////////////////////////////////
// TPublishedTest
//
// A test method found in a test group class's published section (see
// TTestGroupBase::FindPublishedTests()).
/////////////////////////////////////////////////////////////////////////////
struct TPublishedTest
{
    std::string MethodName; // e.g. "Test_Add"
    std::string TestName;   // e.g. "Add"
    void* Code;             // The method's code address, for a closure on a group of that class
};


/////////////////////////////////////////////////////////////////////////////
// TSuitePath
//
// The suite a test group belongs to, given to the TTestGroupBase
// constructor, e.g. TSuitePath("Framework.Console") for suite "Console"
// inside suite "Framework". Suites only organize groups: they have no
// set-up or tear-down of their own, a suite's groups run together, and the
// suite path prefixes each of its tests' full names, so
// --filter "Framework.*" selects every test in suite "Framework". An empty
// path means the group isn't in a suite.
//
// A path with an empty suite name, e.g. "Framework..Console" or
// ".Framework", isn't valid: TTestHandler::Initialize() throws
// TExceptInvalidSuitePath for a group in one, which stops the run before
// any test runs. Unlike asw-unit-tests, neither TSuitePath's constructor
// nor TTestGroupBase's throws for one: bcc32 turns a C++ exception thrown
// while a TTestGroupBase is being constructed into an access violation.
/////////////////////////////////////////////////////////////////////////////
class TSuitePath
{
private:
    std::vector<std::string> m_Names;
    std::string m_Path;
    bool m_Valid;

public:
    explicit TSuitePath(std::string const& path);

    // The suite names in order, outermost first, split at each '.'; empty for an empty path.
    std::vector<std::string> const& GetNames() const;
    std::string const& GetPath() const;
    // Whether no suite name is empty. An empty path is valid, for no suite.
    bool IsValid() const;
};


/////////////////////////////////////////////////////////////////////////////
// TCheckMessage
//
// A message argument (e.g. an assertion's 'msg'), accepting every common
// C++ and RTL string type, so one parameter covers them all. Held as UTF-8.
/////////////////////////////////////////////////////////////////////////////
class TCheckMessage
{
public:
    std::string Text; // UTF-8

public:
    TCheckMessage();
    TCheckMessage(char const* text); // UTF-8 or ASCII; nullptr is empty
    TCheckMessage(std::string const& text); // UTF-8 or ASCII
    TCheckMessage(wchar_t const* text);     // nullptr is empty
    TCheckMessage(std::wstring const& text);
    TCheckMessage(System::AnsiString const& text);
    TCheckMessage(System::UnicodeString const& text);
    TCheckMessage(System::UTF8String const& text);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptionMatcher
//
// Decides whether a caught exception is of the type a test expects (see
// TTestGroupBase::SetExceptionExpected<TException>()). Which method can
// match depends on the type: an RTL exception class matches in
// MatchesRTL(), a std::exception class in MatchesStd(), and any other type
// in MatchesCurrentOther(). Matching never rethrows an RTL exception.
/////////////////////////////////////////////////////////////////////////////
class TExceptionMatcher
{
public:
    virtual ~TExceptionMatcher()
    {
    }

    // Call only from a "catch (...)" handler, which only non-RTL exceptions reach (the RTL ones are caught first),
    // so the rethrow inside never rethrows an RTL exception.
    virtual bool MatchesCurrentOther() const = 0;
    virtual bool MatchesRTL(System::Sysutils::Exception& ex) const = 0;
    virtual bool MatchesStd(std::exception const& ex) const = 0;
};

// Matches an RTL exception class (System::Sysutils::Exception or a descendant) and its descendants.
template <class TException>
class TRTLExceptionMatcher : public TExceptionMatcher
{
public:
    virtual bool MatchesCurrentOther() const
    {
        return false;
    }

    virtual bool MatchesRTL(System::Sysutils::Exception& ex) const
    {
        return ex.InheritsFrom(__classid(TException));
    }

    virtual bool MatchesStd(std::exception const& /*ex*/) const
    {
        return false;
    }
};

// Matches a std::exception class and its descendants.
template <class TException>
class TStdExceptionMatcher : public TExceptionMatcher
{
public:
    virtual bool MatchesCurrentOther() const
    {
        return false;
    }

    virtual bool MatchesRTL(System::Sysutils::Exception& /*ex*/) const
    {
        return false;
    }

    virtual bool MatchesStd(std::exception const& ex) const
    {
        return dynamic_cast<TException const*>(&ex) != nullptr;
    }
};

// Matches any other C++ type (e.g. int, or a class not derived from std::exception) and its descendants.
template <class TException>
class TOtherExceptionMatcher : public TExceptionMatcher
{
public:
    virtual bool MatchesCurrentOther() const; // Defined outside the class: bcc32 W8027 for an inline function with try

    virtual bool MatchesRTL(System::Sysutils::Exception& /*ex*/) const
    {
        return false;
    }

    virtual bool MatchesStd(std::exception const& /*ex*/) const
    {
        return false;
    }
};

template <class TException>
bool TOtherExceptionMatcher<TException>::MatchesCurrentOther() const
{
    try
    {
        throw;
    }
    catch (TException const&)
    {
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// Returns the matcher for TException; the caller owns it. Call as CreateExceptionMatcher<TException>(static_cast<
// TException*>(nullptr)): the pointer argument picks the overload by what TException derives from.
template <class TException>
TExceptionMatcher* CreateExceptionMatcher(System::Sysutils::Exception* /*typeTag*/)
{
    return new TRTLExceptionMatcher<TException>();
}

template <class TException>
TExceptionMatcher* CreateExceptionMatcher(std::exception* /*typeTag*/)
{
    return new TStdExceptionMatcher<TException>();
}

template <class TException>
TExceptionMatcher* CreateExceptionMatcher(...)
{
    return new TOtherExceptionMatcher<TException>();
}


/////////////////////////////////////////////////////////////////////////////
// Throws and NoThrow
//
// What the Throws and NoThrow methods (e.g. CheckThrows()) find when they
// call a callable. Each exception is caught exactly once, by an ordinary
// typed handler, and never rethrown, except the framework's own failure
// signals (TTestException), which are C++ exceptions and pass through.
/////////////////////////////////////////////////////////////////////////////

template <bool Condition, typename T>
struct TEnableIf
{
};

template <typename T>
struct TEnableIf<true, T>
{
    typedef T Type;
};

// False for TTestMethod, which the Throws and NoThrow methods take in an overload of its own: bcc32 doesn't deduce
// a template argument from a bare closure such as DoConvert (only from &DoConvert).
template <typename T>
struct TIsNotTestMethod
{
    static const bool Value = true;
};

template <>
struct TIsNotTestMethod<TTestMethod>
{
    static const bool Value = false;
};

// Describes 'ex', caught by a handler for its own type T, for a Throws failure, and gets its message: what() for a
// std::exception, an RTL exception's Message. Returns false for any other type, which has no message. Call as
// DescribeCaughtException<T>(ex, description, message, &ex): the pointer picks the overload by what T derives from.
template <class T>
bool DescribeCaughtException(T& ex, std::string& description, std::string& message,
    std::exception const* /*typeTag*/)
{
    description = DescribeException(ex);
    message = ex.what();
    return true;
}

template <class T>
bool DescribeCaughtException(T& ex, std::string& description, std::string& message,
    System::Sysutils::Exception const* /*typeTag*/)
{
    description = DescribeException(ex);
    message = RTLExceptionMessage(ex);
    return true;
}

template <class T>
bool DescribeCaughtException(T& /*ex*/, std::string& description, std::string& /*message*/, ...)
{
    description = DescribeUnknownException();
    return false;
}

// True when T is TTestException or a descendant. Call as IsTestExceptionClass<T>(static_cast<T*>(nullptr)).
template <class T>
bool IsTestExceptionClass(TTestException const* /*typeTag*/)
{
    return true;
}

template <class T>
bool IsTestExceptionClass(...)
{
    return false;
}

// True when 'ex', caught by a handler for T, is one of the framework's failure signals (a TTestException, e.g. a
// failed Assert*) that the handler only caught because T is one of its base classes (e.g. std::exception), so it
// must pass through. Call as IsTestExceptionCaughtByBase<T>(ex, &ex).
template <class T>
bool IsTestExceptionCaughtByBase(T const& ex, std::exception const* /*typeTag*/)
{
    return !IsTestExceptionClass<T>(static_cast<T*>(nullptr)) && (dynamic_cast<TTestException const*>(&ex) != nullptr);
}

template <class T>
bool IsTestExceptionCaughtByBase(T const& /*ex*/, ...)
{
    return false;
}

// Calls invoke(callable). Defined in a .cpp file, so that no compiler can see what a call to it does; see
// TCallableInvoker.
void CallOutOfLine(void (*invoke)(void*), void* callable);

// Calls a TCallable (a function, a functor or a closure) through CallOutOfLine(), for the Throws and NoThrow
// methods' try blocks. bcc64x catches an exception, an access violation included, only where it arises in a call
// the try block covers, and leaves out a call it can see won't throw: a functor whose operator() faults ended the
// process from inside NoThrowFailure()'s try block, even unoptimized (and any callable can once optimized: RSS-4778).
// Behind CallOutOfLine(), whatever the callable does arises in a call that's covered.
template <class TCallable>
struct TCallableInvoker
{
    static void Call(TCallable& callable)
    {
        CallOutOfLine(Invoke, &callable);
    }

    static void Invoke(void* callable)
    {
        (*static_cast<TCallable*>(callable))();
    }
};

// Calls 'callable' for the NoThrow methods and returns why it failed, or "" when it threw nothing. A failure signal
// from this framework (e.g. a failed Assert* inside 'callable') passes through, failing the test as usual.
template <class TCallable>
std::string NoThrowFailure(TCallable callable)
{
    // Nothing follows the call in this function's path when 'callable' returns, so a callable that always throws
    // doesn't make code unreachable once inlined (bcc32 W8066 in Release).
    std::string failure;

    try
    {
        TCallableInvoker<TCallable>::Call(callable);
    }
    catch (TTestException const&)
    {
        throw;
    }
    catch (std::exception const& stdEx)
    {
        failure = "Expected no exception but caught: " + DescribeException(stdEx);
    }
    catch (System::Sysutils::Exception& rtlEx)
    {
        failure = "Expected no exception but caught: " + DescribeException(rtlEx);
    }
    catch (...)
    {
        failure = "Expected no exception but caught: " + DescribeUnknownException();
    }

    return failure;
}

// Calls 'callable' for the Throws methods and returns why it failed, or "" when it threw a TException (or a
// descendant) whose message contains 'expectedMessage', when that isn't empty. A failure signal from this framework
// passes through, failing the test as usual, unless TException asks for one (TTestException or a descendant).
template <class TException, class TCallable>
std::string ThrowsFailure(TCallable callable, std::string const& expectedMessage)
{
    // Set before the call, which nothing follows, so a callable that always throws doesn't make code unreachable once
    // inlined (bcc32 W8066 in Release).
    std::string failure = "Expected an exception but none was thrown";

    // The TException handler has a try block of its own, so anything else 'callable' throws reaches the outer
    // handlers once, rather than being rethrown and caught again.
    try
    {
        try
        {
            TCallableInvoker<TCallable>::Call(callable);
        }
        catch (TException& expectedEx)
        {
            if (IsTestExceptionCaughtByBase<TException>(expectedEx, &expectedEx))
                throw;

            failure.clear();

            std::string description;
            std::string message;
            bool const hasMessage = DescribeCaughtException<TException>(expectedEx, description, message,
                &expectedEx);

            if (!expectedMessage.empty() && !hasMessage)
            {
                failure = "Expected the exception message to contain \"" + expectedMessage +
                    "\" but caught an exception without a message: " + description;
            }
            else if (!expectedMessage.empty() && (message.find(expectedMessage) == std::string::npos))
            {
                failure = "Expected the exception message to contain \"" + expectedMessage + "\" but caught: " +
                    description;
            }
        }
    }
    catch (TTestException const&)
    {
        throw;
    }
    catch (std::exception const& stdEx)
    {
        failure = "Expected a different exception type but caught: " + DescribeException(stdEx);
    }
    catch (System::Sysutils::Exception& rtlEx)
    {
        failure = "Expected a different exception type but caught: " + DescribeException(rtlEx);
    }
    catch (...)
    {
        failure = "Expected a different exception type but caught: " + DescribeUnknownException();
    }

    return failure;
}


/////////////////////////////////////////////////////////////////////////////
// TTestGroupBase
//
// Base class of a test group. A group's tests are the methods in its
// __published sections named Test_<Name>, declared as
// "void __fastcall Test_<Name>()" and reported as <Name>; nothing
// registers them. Other published methods (e.g. an event handler a test
// hands to a component) aren't tests. Parameterized tests, which take a
// row, are registered by the constructor instead (see RegisterTestCases()).
//
// Tests run in declaration order, with the tests a group inherits from
// ancestor groups first, then the registered tests. Each test runs on the
// same instance, between SetUp_Group() and TearDown_Group(), with
// SetUp_Test() before it and TearDown_Test() after it.
//
// An exception a test doesn't expect fails only that test, with its
// description; SetExceptionExpected() makes a test expect one.
//
// If SetUp_Group() throws, each of the group's tests fails with that
// exception's description, and the run goes on with the next group (after
// TearDown_Group()). If SetUp_Test(), TearDown_Test() or TearDown_Group()
// throws, the run ends with a TExceptFatalError.
//
// The test methods' signature can't be checked: the published-method table
// holds only names and addresses. A Test_ method that isn't __fastcall, or
// takes parameters, is called as if it were "void __fastcall ()", which
// passes it garbage. A test method that overrides a virtual one must be
// published too, or the ancestor's method runs in its place.
/////////////////////////////////////////////////////////////////////////////
class TTestGroupBase : public System::TObject
{
private:
    typedef System::TObject inherited;

    friend class TTestHandler; // For SetSuitePath()

public:
    typedef std::vector<ITestCase*> TestCallbackList;

private:
    // What a lifecycle method that CallHook() called threw.
    struct THookError
    {
        std::string Description;
        bool UnknownException; // Neither a std::exception nor an RTL exception

        THookError()
            : UnknownException(false)
        {
        }
    };

    // The lifecycle methods CallHook() calls.
    enum class THook
    {
        SetUpGroup,
        SetUpTest,
        TearDownGroup,
        TearDownTest,
    };

    TestCallbackList m_RegisteredTests; // Owned; moved to m_TestCallbacks by GetTestCallbackList()
    TestCallbackList m_TestCallbacks;   // Owned
    bool m_TestCallbacksFound;
    ITestCase* m_GuardedTestCase;                          // The test RunGuardedTest() runs
    bool m_WorkerCatchCrashes;                             // RunCatchingCrashes()' 'catchCrashes' on the worker
    std::unique_ptr<TExceptFatalError> m_WorkerFatalError; // What ended the run on the worker; see RunWithTimeout()
    volatile long m_WorkerState;                           // Whether the worker's test may record; see RunWithTimeout()
    ITestCase* m_WorkerTestCase;                           // The test RunTestOnWorker() runs
    unsigned long m_WorkerThreadID;                        // The worker's, while it runs or once abandoned; else 0
    std::string m_QualifiedName;                           // See GetQualifiedTestGroupName()
    std::string m_SuitePath;                               // See GetSuitePath()

    // Adds 'testCase' (now owned) to the tests that run after the published ones; see RegisterTestCases().
    void AddRegisteredTest(ITestCase* testCase);

    // Used by the Assert ordering methods: throws TExceptOrdering unless 'value' has relation 'ordering' to 'bound'.
    template <typename TValue, typename TBound>
    void AssertOrdering(TValue const& value, TBound const& bound, TOrdering ordering, std::string const& method,
        int line, TCheckMessage const& msg)
    {
        if (!OrderingHolds(CompareOrdered(value, bound), ordering))
        {
            std::string valueText;
            std::string boundText;
            DescribeOrderedValues(value, bound, valueText, boundText);
            throw TExceptOrdering(method, line, valueText, OrderingSymbol(ordering), boundText, msg.Text);
        }
    }

    // On RunWithTimeout()'s worker thread, returns whether the test may change Results(): not once it has been
    // abandoned. Otherwise it may, and EndWorkerRecording() must follow, before which the test can't be abandoned;
    // with 'finishing', for its outcome, it never can be after that. Elsewhere, always true.
    bool BeginWorkerRecording(bool finishing);
    // Calls 'hook' (with 'testCase' for the per-test ones) and returns true, or, if it throws, describes the
    // exception in 'error' and returns false. Each exception is caught exactly once, never rethrown.
    bool CallHook(THook hook, ITestCase* testCase, THookError& error);
    // Used by the Check ordering methods: records a failure unless 'value' has relation 'ordering' to 'bound'.
    template <typename TValue, typename TBound>
    void CheckOrdering(TValue const& value, TBound const& bound, TOrdering ordering, std::string const& method,
        int line, TCheckMessage const& msg)
    {
        if (!OrderingHolds(CompareOrdered(value, bound), ordering))
        {
            std::string valueText;
            std::string boundText;
            DescribeOrderedValues(value, bound, valueText, boundText);
            SetTestFailedCheck(method, line, "Expected " + valueText + " to be " + OrderingSymbol(ordering) + " " +
                boundText + ". " + msg.Text);
        }
    }
    void DeleteTestCallbacks();
    // Ends what BeginWorkerRecording() (without 'finishing') allowed.
    void EndWorkerRecording();
    // Records 'testCase''s outcome, logs it, and counts it in Results().
    void FinishTest(ITestCase& testCase, TTestOutcome outcome, std::string const& detail, double durationMs);
    // Runs m_GuardedTestCase with Test(), inside RunCatchingCrashes()' crash guard. __fastcall, since the guard calls
    // it as a TCrashGuard::TBody.
    void __fastcall RunGuardedTest();
    // Runs m_WorkerTestCase with RunCatchingCrashes() on RunWithTimeout()'s worker thread, keeping what ends the run
    // (see RunWithTimeout()) in m_WorkerFatalError. __fastcall, since the worker calls it as a TThreadMethod.
    void __fastcall RunTestOnWorker();
    // Scores an exception caught while one was expected: 'typeMatches' is whether it's of the expected type (true
    // when no type was given), 'message' is the text checked for the expected message, and 'description' is how
    // the exception is shown in a failure detail. Sets 'outcome' and 'detail'.
    void ScoreExpectedException(bool typeMatches, std::string const& message, std::string const& description,
        TTestOutcome& outcome, std::string& detail);
    // Puts the group in suite 'suitePath', unchecked (see TSuitePath), for the constructor, and for TTestHandler
    // when the group's registration names its suite (see ASWBCB_REGISTER_TEST_GROUP_IN_SUITE).
    void SetSuitePath(std::string const& suitePath);
    // Records a Contains, StartsWith or EndsWith Check* failure (or a Not form's), with msg
    // "Expected "text" <relation> "part". msg" (e.g. relation "to contain"), or with 'ignoreCase'
    // "Expected "text" <relation> "part" (ignoring case). msg".
    void SetTestFailedCheckTextPart(std::string const& method, int line, std::string const& text,
        std::string const& relation, std::string const& part, bool ignoreCase, std::string const& msg);
    // Logs a fatal error in 'hook' and throws the TExceptFatalError that ends the run.
    void ThrowFatalError(std::string const& hookName, std::string const& subject, THookError const& error);

protected:
    bool m_ExceptionExpected;
    bool m_TestFailedCheck;
    // The current test's Check* failures, without the log line's "  **" prefix; see FinishTest() for how a failed
    // test's record uses them.
    std::vector<std::string> m_CheckFailureMessages;
    std::string m_ExceptionExpectedText;
    std::string m_ExpectedExceptionMessage;
    std::unique_ptr<TExceptionMatcher> m_ExpectedExceptionMatcher; // nullptr when any exception will do
    std::string m_Name;
    TTestResults m_Results;
    ITestRunObserver* m_RunObserver; // Not owned; nullptr when none is set.

    // True when the current test's exception expectation (see SetExceptionExpected<TException>()) also requires a
    // specific exception type, rather than accepting any thrown exception.
    virtual bool ExceptionTypeExpected() const;
    // Aborts the current test and reports it as failed, like a failed Assert* method, even while an exception is
    // expected.
    virtual void Fail(std::string const& method, int line, TCheckMessage const& msg = TCheckMessage());
    // The class whose published tests this group runs: its own class, unless a descendant says otherwise.
    virtual System::TClass GetTestClass();
    virtual void Log(std::string const& msg);
    virtual void LogAppend(std::string const& msg);
    // Registers a parameterized test: one test case per element of 'params', each calling 'method' with its own
    // copy of that element. Call from the group's constructor. The cases run after the published tests, in the
    // order they're registered, and are named "nameBase[i]" with the row's zero-based index, or "nameBase[label]"
    // with the label 'nameGenerator' gives the row. 'method' is a closure to a method of the group,
    // "void __fastcall Test_X(TParam const& row)", written Test_X; it must not be published, since a published
    // Test_ method would also run as an ordinary test, without its row. A name used twice in the group, published
    // or registered, stops the run at startup (TExceptDuplicateTestName from GetTestCallbackList()).
    // (Defined after the class: bcc32 W8027 for an inline function with a loop.)
    template <class TParam>
    void RegisterTestCases(typename TParamTestMethod<TParam>::Type method, std::string const& nameBase,
        std::vector<TParam> const& params, typename TParamTestNamer<TParam>::Type nameGenerator = nullptr);
    // The same, with 'method' given as a pointer to a member function, &TTest_Group::Test_X, as asw-unit-tests
    // takes it.
    template <class T, class TParam>
    void RegisterTestCases(void(__fastcall T::* method)(TParam const&), std::string const& nameBase,
        std::vector<TParam> const& params, typename TParamTestNamer<TParam>::Type nameGenerator = nullptr);
    // Called by RunCatchingCrashes() when 'testCase' crashed: records it as failed ("Test crashed: <description>."),
    // and logs whether the run goes on; with 'abortRun', it doesn't, and Results().Crashed is set.
    virtual void ReportCrashedTest(ITestCase& testCase, std::string const& description, bool abortRun);
    // Called by RunWithTimeout() when 'testCase' didn't finish within 'testTimeoutSeconds': records it as failed
    // ("Test exceeded its N second timeout and was abandoned."), with the timeout as its duration, and logs that the
    // run ends. Doesn't touch what the abandoned test may still be using (its Check* failures, for one).
    virtual void ReportTimedOutTest(ITestCase& testCase, unsigned int testTimeoutSeconds);
    virtual void ResetTestFailedOneOrMoreChecks();
    // Without 'catchCrashes', calls Test(testCase). With it, calls it through a TCrashGuard: a stack overflow or an
    // illegal instruction in the test is recorded as its failure (see ReportCrashedTest()) instead of ending the
    // process, and a stack overflow ends the run. The crashed test is abandoned where it faulted: its TearDown_Test()
    // and the destructors of what it had on the stack don't run.
    virtual void RunCatchingCrashes(ITestCase& testCase, bool catchCrashes);
    // Runs each test 'filter' matches (every test with nullptr; see RunWithTimeout()), in the order 'shuffleSeed' gives
    // (see Run()), between the observer's OnTestStarted() and OnTestFinished() calls, and logs the group's totals.
    // With a non-empty 'failureDetail', fails each of them with it instead of running it. Stops after a test that
    // timed out or crashed so that the run must end, with Results().TimedOut or Crashed set.
    void RunTests(std::string const& failureDetail, ITestFilter const* filter, TOptional<unsigned int> shuffleSeed,
        TOptional<unsigned int> testTimeoutSeconds, bool catchCrashes);
    // Without 'testTimeoutSeconds', calls RunCatchingCrashes(testCase, catchCrashes) and returns true. With it, runs
    // that on a worker thread (a TThread) while this one waits, and returns true once it ends, after throwing on this
    // thread the TExceptFatalError a failing SetUp_Test() or TearDown_Test() threw there (anything else that got out
    // of Test() arrives as one too, with the exit code it would have had). While it waits, this thread runs the
    // worker's TThread::Synchronize() and Queue() calls if it's the main thread, as it would if the test ran here.
    // If the test doesn't end within 'testTimeoutSeconds', calls ReportTimedOutTest() and returns false: the worker
    // thread can't be stopped safely, so it's abandoned, still running (or hung), with its TThread object never
    // freed. From then on, it changes nothing in Results() (see BeginWorkerRecording()), at least until the group
    // runs another test on a worker: its outcome and Check* failures, should it get that far, are dropped without
    // being logged. A test that had already recorded its outcome when the time ran out counts as finished.
    virtual bool RunWithTimeout(ITestCase& testCase, TOptional<unsigned int> testTimeoutSeconds, bool catchCrashes);
    // With 'expected' true, the rest of the current test must throw an exception (of any type), or the test fails
    // with "Exception expected: Method (Line): msg"; false cancels that. Each test starts with no exception
    // expected.
    virtual void SetExceptionExpected(bool expected, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage());
    // Expects the rest of the current test to throw a TException (or a descendant): an RTL exception class, a
    // std::exception class, or any other C++ type. When 'expectedMessage' isn't empty, the exception's message
    // (an RTL exception's Message, a std::exception's what()) must also contain it. An exception of another type,
    // or without that message, fails the test, as does none at all.
    template <class TException>
    void SetExceptionExpected(std::string const& method, int line, TCheckMessage const& msg = TCheckMessage(),
        TCheckMessage const& expectedMessage = TCheckMessage())
    {
        SetExceptionExpected(true, method, line, msg);
        m_ExpectedExceptionMessage = expectedMessage.Text;
        m_ExpectedExceptionMatcher.reset(CreateExceptionMatcher<TException>(static_cast<TException*>(nullptr)));
    }
    // Records a Check* failure: the test goes on, but fails when it ends. Logged right away as
    // "  **Check failed for: "Method" (Line): msg".
    virtual void SetTestFailedCheck(std::string const& method, int line, std::string const& msg);
    // The same, with msg "Expected "expected" but was "actual". msg".
    virtual void SetTestFailedCheck(std::string const& method, int line, std::string const& expected,
        std::string const& actual, std::string const& msg);
    // Records a CheckNotEquals() failure, with msg "Both values equal: "value". msg".
    virtual void SetTestFailedCheckNotEquals(std::string const& method, int line, std::string const& value,
        std::string const& msg);
    virtual void SetUp_Test(ITestCase& testCase); // Called just before each test
    // Aborts the current test and reports it as skipped rather than passed or failed. Can be called
    // unconditionally to keep a test from running, or after a runtime check (e.g. for a feature the system
    // lacks). No 'return' is needed afterward.
    virtual void Skip(std::string const& method, int line, TCheckMessage const& reason = TCheckMessage());
    virtual void TearDown_Test(ITestCase& testCase); // Called just after each test
    virtual void Test(ITestCase& testCase);          // Runs one test with its SetUp_Test() and TearDown_Test()
    virtual bool TestFailedOneOrMoreChecks();

protected: // Assertion/Check methods - Equals
    // 'expected' and 'actual' may be:
    // - bool;
    // - integers of any types, compared by value (-1 never equals 0xFFFFFFFF), and shown as numbers (a char too);
    // - enums, compared with each other or with integers by their integer values, and shown as numbers;
    // - pointers, compared and shown by address (two pointers that C++ can't compare don't compile);
    // - strings of any C++ or RTL string type: char*/char const*/char arrays/std::string (UTF-8),
    //   wchar_t*/wchar_t const*/wchar_t arrays/std::wstring, AnsiString, UnicodeString and UTF8String. Two narrow
    //   or two wide C++ strings compare as they are; any other pair of strings compares as Unicode. A null C string
    //   equals only another null one, and shows as "(null)".
    // Values of other types, or of two kinds that don't mix (e.g. an integer and a bool), don't compile.
    template <typename TExpected, typename TActual>
    void AssertEquals(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!ValuesEqual(expected, actual))
            throw TExceptEquals(method, line, DescribeValue(expected), DescribeValue(actual), msg.Text);
    }

    // Two strings, compared as Equals compares them but ignoring the case of the ASCII letters A-Z only (as
    // asw-unit-tests does), so the result doesn't depend on the locale. Values that aren't both strings don't
    // compile.
    template <typename TExpected, typename TActual>
    void AssertEqualsIC(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextsEqualIgnoringASCIICase(expected, actual))
            throw TExceptEquals(method, line, DescribeValue(expected), DescribeValue(actual), msg.Text, true);
    }

    template <typename TExpected, typename TActual>
    void CheckEquals(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!ValuesEqual(expected, actual))
            SetTestFailedCheck(method, line, DescribeValue(expected), DescribeValue(actual), msg.Text);
    }

    template <typename TExpected, typename TActual>
    void CheckEqualsIC(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextsEqualIgnoringASCIICase(expected, actual))
        {
            SetTestFailedCheck(method, line, "Expected \"" + DescribeValue(expected) + "\" but was \"" +
                DescribeValue(actual) + "\" (ignoring case). " + msg.Text);
        }
    }

protected: // Assertion/Check methods - Not Equals
    // The values Equals takes (see above), compared the same way. A failure shows 'expected'.
    template <typename TExpected, typename TActual>
    void AssertNotEquals(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (ValuesEqual(expected, actual))
            throw TExceptNotEquals(method, line, DescribeValue(expected), msg.Text);
    }

    // Two strings, compared as EqualsIC compares them (see above). A failure shows both.
    template <typename TExpected, typename TActual>
    void AssertNotEqualsIC(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextsEqualIgnoringASCIICase(expected, actual))
            throw TExceptNotEquals(method, line, DescribeValue(expected), DescribeValue(actual), msg.Text, true);
    }

    template <typename TExpected, typename TActual>
    void CheckNotEquals(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (ValuesEqual(expected, actual))
            SetTestFailedCheckNotEquals(method, line, DescribeValue(expected), msg.Text);
    }

    template <typename TExpected, typename TActual>
    void CheckNotEqualsIC(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextsEqualIgnoringASCIICase(expected, actual))
        {
            SetTestFailedCheck(method, line, "Both values equal: \"" + DescribeValue(expected) + "\" and \"" +
                DescribeValue(actual) + "\" (ignoring case). " + msg.Text);
        }
    }

protected: // Assertion/Check methods - Near (floating point, absolute tolerance)
    // 'actual' is near 'expected' when they're equal (so equal infinities are) or differ by at most 'tolerance'; a
    // NaN is near nothing. A negative or NaN 'tolerance' fails all four, whatever the values ("Invalid tolerance
    // -0.500000: it must be zero or more"). Failures show values as std::to_string() does (see DescribeFixed()), e.g.
    // "0.100000".
    virtual void AssertNear(float expected, float actual, float tolerance, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage());
    virtual void AssertNear(double expected, double actual, double tolerance, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage());

    virtual void AssertNotNear(float expected, float actual, float tolerance, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage());
    virtual void AssertNotNear(double expected, double actual, double tolerance, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage());

    virtual void CheckNear(float expected, float actual, float tolerance, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage());
    virtual void CheckNear(double expected, double actual, double tolerance, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage());

    virtual void CheckNotNear(float expected, float actual, float tolerance, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage());
    virtual void CheckNotNear(double expected, double actual, double tolerance, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage());

protected: // Assertion/Check methods - Boolean
    virtual void AssertFalse(bool testVal, std::string const& method, int line, TCheckMessage const& msg =
        TCheckMessage());
    virtual void AssertTrue(bool testVal, std::string const& method, int line, TCheckMessage const& msg =
        TCheckMessage());
    virtual void CheckFalse(bool testVal, std::string const& method, int line, TCheckMessage const& msg =
        TCheckMessage());
    virtual void CheckTrue(bool testVal, std::string const& method, int line, TCheckMessage const& msg =
        TCheckMessage());

protected: // Assertion/Check methods - Null (value compared with nullptr)
    // 'value' is a raw pointer, a std::unique_ptr or a System::DelphiInterface (an interface reference, e.g.
    // _di_IXMLNode). Other values (an array, an integer, nullptr itself) don't compile.
    template <typename TValue>
    void AssertNotNull(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TNullCheck<TValue>::IsNull(value))
            throw TExceptNotNull(method, line, msg.Text);
    }

    template <typename TValue>
    void AssertNull(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TNullCheck<TValue>::IsNull(value))
            throw TExceptNull(method, line, msg.Text);
    }

    template <typename TValue>
    void CheckNotNull(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TNullCheck<TValue>::IsNull(value))
            SetTestFailedCheck(method, line, "Expected not null but was null: \"" + msg.Text + "\"");
    }

    template <typename TValue>
    void CheckNull(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TNullCheck<TValue>::IsNull(value))
            SetTestFailedCheck(method, line, "Expected null but was not null: \"" + msg.Text + "\"");
    }

protected: // Assertion/Check methods - Same (identity, compared by address)
    // 'expected' and 'actual' are raw pointers, arrays, std::unique_ptrs or System::DelphiInterfaces, mixed freely,
    // that C++ can compare by address. Unlike Equals, two C strings are compared by address too.
    template <typename TExpected, typename TActual>
    void AssertNotSame(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TSameAddress<TExpected>::Get(expected) == TSameAddress<TActual>::Get(actual))
            throw TExceptNotSame(method, line, DescribePointer(TSameAddress<TExpected>::Get(expected)), msg.Text);
    }

    template <typename TExpected, typename TActual>
    void AssertSame(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TSameAddress<TExpected>::Get(expected) != TSameAddress<TActual>::Get(actual))
        {
            throw TExceptSame(method, line, DescribePointer(TSameAddress<TExpected>::Get(expected)),
                DescribePointer(TSameAddress<TActual>::Get(actual)), msg.Text);
        }
    }

    template <typename TExpected, typename TActual>
    void CheckNotSame(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TSameAddress<TExpected>::Get(expected) == TSameAddress<TActual>::Get(actual))
        {
            SetTestFailedCheck(method, line, "Expected a different object but both are \"" +
                DescribePointer(TSameAddress<TExpected>::Get(expected)) + "\". " + msg.Text);
        }
    }

    template <typename TExpected, typename TActual>
    void CheckSame(TExpected const& expected, TActual const& actual, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TSameAddress<TExpected>::Get(expected) != TSameAddress<TActual>::Get(actual))
        {
            SetTestFailedCheck(method, line, "Expected the same object as \"" +
                DescribePointer(TSameAddress<TExpected>::Get(expected)) + "\" but was \"" +
                DescribePointer(TSameAddress<TActual>::Get(actual)) + "\". " + msg.Text);
        }
    }

protected: // Assertion/Check methods - Type (dynamic type, via dynamic_cast)
    // Each takes a raw pointer, a std::unique_ptr or an object, of a polymorphic class (any TObject descendant is
    // one), and checks whether it is a TType, a descendant counting as one; e.g. CheckIsType<TCircle>(shape, ...).
    // A null pointer is never a TType. A failure shows TType and the object's class (see TypeCheckFailure()).
    template <class TType, typename TValue>
    void AssertIsNotType(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = TypeCheckFailure<TType>(value, false);
        if (!failure.empty())
            throw TExceptIsNotType(method, line, failure, msg.Text);
    }

    template <class TType, typename TValue>
    void AssertIsType(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = TypeCheckFailure<TType>(value, true);
        if (!failure.empty())
            throw TExceptIsType(method, line, failure, msg.Text);
    }

    template <class TType, typename TValue>
    void CheckIsNotType(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = TypeCheckFailure<TType>(value, false);
        if (!failure.empty())
            SetTestFailedCheck(method, line, failure + ". " + msg.Text);
    }

    template <class TType, typename TValue>
    void CheckIsType(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = TypeCheckFailure<TType>(value, true);
        if (!failure.empty())
            SetTestFailedCheck(method, line, failure + ". " + msg.Text);
    }

protected: // Assertion/Check methods - Throws (exception from a callable)
    // Each calls 'callable' and checks whether it throws. Unlike SetExceptionExpected(), the test carries on
    // afterwards (unless an Assert* method fails), so it can check several calls and the state after each.
    // 'callable' is a closure to a group's "void __fastcall Method()" (DoConvert or &DoConvert), or to any other
    // method without parameters (&Method; bcc32 needs the & there), a function pointer, or a functor object, whose
    // class must be at namespace scope (bcc32 rejects local classes as template arguments). A return value is
    // ignored.
    // The Throws methods pass when 'callable' throws a TException or a descendant (an RTL exception class, a
    // std::exception class or any other C++ type) whose message (what(), or an RTL exception's Message) contains
    // 'expectedMessage', when that isn't empty. A failure signal from this framework inside 'callable' (a failed
    // Assert*, Fail() or Skip()) still ends the test as usual, unless TException is TTestException or a descendant.
    virtual void AssertNoThrow(TTestMethod callable, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage());

    template <class TCallable>
    typename TEnableIf<TIsNotTestMethod<TCallable>::Value, void>::Type AssertNoThrow(TCallable callable,
        std::string const& method, int line, TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = NoThrowFailure(callable);
        if (!failure.empty())
            throw TExceptNoThrow(method, line, failure, msg.Text);
    }

    template <class TException>
    void AssertThrows(TTestMethod callable, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage(), TCheckMessage const& expectedMessage = TCheckMessage())
    {
        std::string const failure = ThrowsFailure<TException>(callable, expectedMessage.Text);
        if (!failure.empty())
            throw TExceptThrows(method, line, failure, msg.Text);
    }

    template <class TException, class TCallable>
    typename TEnableIf<TIsNotTestMethod<TCallable>::Value, void>::Type AssertThrows(TCallable callable,
        std::string const& method, int line, TCheckMessage const& msg = TCheckMessage(),
        TCheckMessage const& expectedMessage = TCheckMessage())
    {
        std::string const failure = ThrowsFailure<TException>(callable, expectedMessage.Text);
        if (!failure.empty())
            throw TExceptThrows(method, line, failure, msg.Text);
    }

    virtual void CheckNoThrow(TTestMethod callable, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage());

    template <class TCallable>
    typename TEnableIf<TIsNotTestMethod<TCallable>::Value, void>::Type CheckNoThrow(TCallable callable,
        std::string const& method, int line, TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = NoThrowFailure(callable);
        if (!failure.empty())
            SetTestFailedCheck(method, line, failure + ". " + msg.Text);
    }

    template <class TException>
    void CheckThrows(TTestMethod callable, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage(), TCheckMessage const& expectedMessage = TCheckMessage())
    {
        std::string const failure = ThrowsFailure<TException>(callable, expectedMessage.Text);
        if (!failure.empty())
            SetTestFailedCheck(method, line, failure + ". " + msg.Text);
    }

    template <class TException, class TCallable>
    typename TEnableIf<TIsNotTestMethod<TCallable>::Value, void>::Type CheckThrows(TCallable callable,
        std::string const& method, int line, TCheckMessage const& msg = TCheckMessage(),
        TCheckMessage const& expectedMessage = TCheckMessage())
    {
        std::string const failure = ThrowsFailure<TException>(callable, expectedMessage.Text);
        if (!failure.empty())
            SetTestFailedCheck(method, line, failure + ". " + msg.Text);
    }

protected: // Assertion/Check methods - Empty (string or container)
    // 'value' is a string of any C++ or RTL string type, or a container: any class with empty() and size() members,
    // e.g. a std::vector, a std::map or a System::DynamicArray (TBytes). A null C string is neither empty nor not
    // empty, so it fails both, shown as "(null)". A failure shows a string's text or a container's element count.
    // Other values (a bool, an integer, an enum, a pointer) don't compile.
    template <typename TValue>
    void AssertEmpty(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = EmptyFailure(value);
        if (!failure.empty())
            throw TExceptEmpty(method, line, failure, msg.Text);
    }

    template <typename TValue>
    void AssertNotEmpty(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = NotEmptyFailure(value);
        if (!failure.empty())
            throw TExceptNotEmpty(method, line, failure, msg.Text);
    }

    template <typename TValue>
    void CheckEmpty(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = EmptyFailure(value);
        if (!failure.empty())
            SetTestFailedCheck(method, line, failure + ". " + msg.Text);
    }

    template <typename TValue>
    void CheckNotEmpty(TValue const& value, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = NotEmptyFailure(value);
        if (!failure.empty())
            SetTestFailedCheck(method, line, failure + ". " + msg.Text);
    }

protected: // Assertion/Check methods - Contains (substring)
    // 'text' and 'substring' are strings of any C++ or RTL string types, paired as Equals pairs them; the IC forms
    // ignore the case of the ASCII letters A-Z only, as EqualsIC does. Every text contains an empty substring. A
    // null C string (shown as "(null)") neither contains nor is contained in anything. Values that aren't both
    // strings don't compile.
    template <typename TText, typename TSubstring>
    void AssertContains(TText const& text, TSubstring const& substring, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, substring, TTextPart::Contains, false))
            throw TExceptContains(method, line, DescribeValue(text), DescribeValue(substring), msg.Text);
    }

    template <typename TText, typename TSubstring>
    void AssertContainsIC(TText const& text, TSubstring const& substring, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, substring, TTextPart::Contains, true))
            throw TExceptContains(method, line, DescribeValue(text), DescribeValue(substring), msg.Text, true);
    }

    template <typename TText, typename TSubstring>
    void AssertNotContains(TText const& text, TSubstring const& substring, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, substring, TTextPart::Contains, false))
            throw TExceptNotContains(method, line, DescribeValue(text), DescribeValue(substring), msg.Text);
    }

    template <typename TText, typename TSubstring>
    void AssertNotContainsIC(TText const& text, TSubstring const& substring, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, substring, TTextPart::Contains, true))
            throw TExceptNotContains(method, line, DescribeValue(text), DescribeValue(substring), msg.Text, true);
    }

    template <typename TText, typename TSubstring>
    void CheckContains(TText const& text, TSubstring const& substring, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, substring, TTextPart::Contains, false))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "to contain", DescribeValue(substring),
                false, msg.Text);
        }
    }

    template <typename TText, typename TSubstring>
    void CheckContainsIC(TText const& text, TSubstring const& substring, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, substring, TTextPart::Contains, true))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "to contain", DescribeValue(substring),
                true, msg.Text);
        }
    }

    template <typename TText, typename TSubstring>
    void CheckNotContains(TText const& text, TSubstring const& substring, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, substring, TTextPart::Contains, false))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "not to contain", DescribeValue(substring),
                false, msg.Text);
        }
    }

    template <typename TText, typename TSubstring>
    void CheckNotContainsIC(TText const& text, TSubstring const& substring, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, substring, TTextPart::Contains, true))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "not to contain", DescribeValue(substring),
                true, msg.Text);
        }
    }

protected: // Assertion/Check methods - Starts/Ends With (prefix/suffix)
    // 'text' and 'prefix' or 'suffix' are strings, as for Contains (see above). Every text starts and ends with an
    // empty string.
    template <typename TText, typename TSuffix>
    void AssertEndsWith(TText const& text, TSuffix const& suffix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, suffix, TTextPart::EndsWith, false))
            throw TExceptEndsWith(method, line, DescribeValue(text), DescribeValue(suffix), msg.Text);
    }

    template <typename TText, typename TSuffix>
    void AssertEndsWithIC(TText const& text, TSuffix const& suffix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, suffix, TTextPart::EndsWith, true))
            throw TExceptEndsWith(method, line, DescribeValue(text), DescribeValue(suffix), msg.Text, true);
    }

    template <typename TText, typename TSuffix>
    void AssertNotEndsWith(TText const& text, TSuffix const& suffix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, suffix, TTextPart::EndsWith, false))
            throw TExceptNotEndsWith(method, line, DescribeValue(text), DescribeValue(suffix), msg.Text);
    }

    template <typename TText, typename TSuffix>
    void AssertNotEndsWithIC(TText const& text, TSuffix const& suffix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, suffix, TTextPart::EndsWith, true))
            throw TExceptNotEndsWith(method, line, DescribeValue(text), DescribeValue(suffix), msg.Text, true);
    }

    template <typename TText, typename TSuffix>
    void CheckEndsWith(TText const& text, TSuffix const& suffix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, suffix, TTextPart::EndsWith, false))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "to end with", DescribeValue(suffix),
                false, msg.Text);
        }
    }

    template <typename TText, typename TSuffix>
    void CheckEndsWithIC(TText const& text, TSuffix const& suffix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, suffix, TTextPart::EndsWith, true))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "to end with", DescribeValue(suffix),
                true, msg.Text);
        }
    }

    template <typename TText, typename TSuffix>
    void CheckNotEndsWith(TText const& text, TSuffix const& suffix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, suffix, TTextPart::EndsWith, false))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "not to end with", DescribeValue(suffix),
                false, msg.Text);
        }
    }

    template <typename TText, typename TSuffix>
    void CheckNotEndsWithIC(TText const& text, TSuffix const& suffix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, suffix, TTextPart::EndsWith, true))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "not to end with", DescribeValue(suffix),
                true, msg.Text);
        }
    }

    template <typename TText, typename TPrefix>
    void AssertStartsWith(TText const& text, TPrefix const& prefix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, prefix, TTextPart::StartsWith, false))
            throw TExceptStartsWith(method, line, DescribeValue(text), DescribeValue(prefix), msg.Text);
    }

    template <typename TText, typename TPrefix>
    void AssertStartsWithIC(TText const& text, TPrefix const& prefix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, prefix, TTextPart::StartsWith, true))
            throw TExceptStartsWith(method, line, DescribeValue(text), DescribeValue(prefix), msg.Text, true);
    }

    template <typename TText, typename TPrefix>
    void AssertNotStartsWith(TText const& text, TPrefix const& prefix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, prefix, TTextPart::StartsWith, false))
            throw TExceptNotStartsWith(method, line, DescribeValue(text), DescribeValue(prefix), msg.Text);
    }

    template <typename TText, typename TPrefix>
    void AssertNotStartsWithIC(TText const& text, TPrefix const& prefix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, prefix, TTextPart::StartsWith, true))
            throw TExceptNotStartsWith(method, line, DescribeValue(text), DescribeValue(prefix), msg.Text, true);
    }

    template <typename TText, typename TPrefix>
    void CheckStartsWith(TText const& text, TPrefix const& prefix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, prefix, TTextPart::StartsWith, false))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "to start with", DescribeValue(prefix),
                false, msg.Text);
        }
    }

    template <typename TText, typename TPrefix>
    void CheckStartsWithIC(TText const& text, TPrefix const& prefix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (!TextHasPart(text, prefix, TTextPart::StartsWith, true))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "to start with", DescribeValue(prefix),
                true, msg.Text);
        }
    }

    template <typename TText, typename TPrefix>
    void CheckNotStartsWith(TText const& text, TPrefix const& prefix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, prefix, TTextPart::StartsWith, false))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "not to start with", DescribeValue(prefix),
                false, msg.Text);
        }
    }

    template <typename TText, typename TPrefix>
    void CheckNotStartsWithIC(TText const& text, TPrefix const& prefix, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        if (TextHasPart(text, prefix, TTextPart::StartsWith, true))
        {
            SetTestFailedCheckTextPart(method, line, DescribeValue(text), "not to start with", DescribeValue(prefix),
                true, msg.Text);
        }
    }

protected: // Assertion/Check methods - Matches (regular expression)
    // Each checks whether the whole of 'text' matches the regular expression 'pattern', both strings of any C++ or
    // RTL string types. The syntax is PCRE's, as the RTL's TRegEx takes it (asw-unit-tests uses ECMAScript); start or
    // end a pattern with ".*" to match part of the text. An invalid pattern fails the check, showing why. A null C
    // string (shown as "(null)"), as either, never matches. Values that aren't both strings don't compile.
    template <typename TText, typename TPattern>
    void AssertMatches(TText const& text, TPattern const& pattern, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = TextMatchFailure(text, pattern, true);
        if (!failure.empty())
            throw TExceptMatches(method, line, failure, msg.Text);
    }

    template <typename TText, typename TPattern>
    void AssertNotMatches(TText const& text, TPattern const& pattern, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = TextMatchFailure(text, pattern, false);
        if (!failure.empty())
            throw TExceptNotMatches(method, line, failure, msg.Text);
    }

    template <typename TText, typename TPattern>
    void CheckMatches(TText const& text, TPattern const& pattern, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = TextMatchFailure(text, pattern, true);
        if (!failure.empty())
            SetTestFailedCheck(method, line, failure + ". " + msg.Text);
    }

    template <typename TText, typename TPattern>
    void CheckNotMatches(TText const& text, TPattern const& pattern, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        std::string const failure = TextMatchFailure(text, pattern, false);
        if (!failure.empty())
            SetTestFailedCheck(method, line, failure + ". " + msg.Text);
    }

protected: // Assertion/Check methods - Ordering (value compared with a bound)
    // 'value' and 'bound' are integers (not bool or enums; char counts) or floating-point values, in any mix; see
    // CompareOrdered(). A NaN is in no order, so every ordering check of one fails. Values that can't be ordered
    // don't compile.
    template <typename TValue, typename TBound>
    void AssertGreaterThan(TValue const& value, TBound const& bound, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        AssertOrdering(value, bound, TOrdering::GreaterThan, method, line, msg);
    }

    template <typename TValue, typename TBound>
    void AssertGreaterThanOrEqual(TValue const& value, TBound const& bound, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        AssertOrdering(value, bound, TOrdering::GreaterThanOrEqual, method, line, msg);
    }

    template <typename TValue, typename TBound>
    void AssertLessThan(TValue const& value, TBound const& bound, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        AssertOrdering(value, bound, TOrdering::LessThan, method, line, msg);
    }

    template <typename TValue, typename TBound>
    void AssertLessThanOrEqual(TValue const& value, TBound const& bound, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        AssertOrdering(value, bound, TOrdering::LessThanOrEqual, method, line, msg);
    }

    template <typename TValue, typename TBound>
    void CheckGreaterThan(TValue const& value, TBound const& bound, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        CheckOrdering(value, bound, TOrdering::GreaterThan, method, line, msg);
    }

    template <typename TValue, typename TBound>
    void CheckGreaterThanOrEqual(TValue const& value, TBound const& bound, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        CheckOrdering(value, bound, TOrdering::GreaterThanOrEqual, method, line, msg);
    }

    template <typename TValue, typename TBound>
    void CheckLessThan(TValue const& value, TBound const& bound, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        CheckOrdering(value, bound, TOrdering::LessThan, method, line, msg);
    }

    template <typename TValue, typename TBound>
    void CheckLessThanOrEqual(TValue const& value, TBound const& bound, std::string const& method, int line,
        TCheckMessage const& msg = TCheckMessage())
    {
        CheckOrdering(value, bound, TOrdering::LessThanOrEqual, method, line, msg);
    }

protected: // Assertion/Check methods - Memory (bytes)
    // Each compares the 'size' bytes at 'expected' with those at 'actual'. A failure shows the offset of the first
    // differing byte and up to 16 bytes from there, in hexadecimal. Zero bytes, or the same pointer twice (even
    // nullptr), are equal; exactly one null pointer isn't equal to anything, and neither is read.
    virtual void AssertEqualsMem(void const* expected, void const* actual, std::size_t size, std::string const& method,
        int line, TCheckMessage const& msg = TCheckMessage());
    virtual void AssertNotEqualsMem(void const* expected, void const* actual, std::size_t size,
        std::string const& method, int line, TCheckMessage const& msg = TCheckMessage());
    virtual void CheckEqualsMem(void const* expected, void const* actual, std::size_t size, std::string const& method,
        int line, TCheckMessage const& msg = TCheckMessage());
    virtual void CheckNotEqualsMem(void const* expected, void const* actual, std::size_t size,
        std::string const& method, int line, TCheckMessage const& msg = TCheckMessage());

public:
    // Returns the tests that 'groupClass' (a TTestGroupBase descendant) publishes, in the order they run: its
    // ancestors' first, each class's in declaration order, and each test name once. A test method that a
    // descendant publishes again (an override, or one that hides it) keeps its ancestor's position, with the
    // descendant's code. Throws TExceptDuplicateTestName if one class publishes a test method name twice.
    static std::vector<TPublishedTest> FindPublishedTests(System::TClass groupClass);

    __fastcall TTestGroupBase(std::string const& name);
    // For a group in a suite, e.g. inherited(TSuitePath("Framework.Console"), "TTest_TConsole").
    __fastcall TTestGroupBase(TSuitePath const& suitePath, std::string const& name);
    __fastcall virtual ~TTestGroupBase();

    // The number of this group's tests that 'filter' matches; all of them with nullptr.
    unsigned int CountTests(ITestFilter const* filter);
    // GetTestGroupName(), prefixed by GetSuitePath() and a '.' when the group is in a suite, e.g.
    // "Framework.Console.TTest_TConsole". Identifies the group in its tests' full names, records and reports.
    std::string const& GetQualifiedTestGroupName() const;
    // The suite this group belongs to, e.g. "Framework.Console" (see TSuitePath); empty when it isn't in one.
    std::string const& GetSuitePath() const;
    // This group's tests (see FindPublishedTests()), found on first use. Throws TExceptDuplicateTestName as
    // FindPublishedTests() does.
    TestCallbackList const& GetTestCallbackList();
    // The group's own name, without its suite path.
    std::string const& GetTestGroupName() const;
    TTestResults const& Results() const;
    // Runs the group: SetUp_Group(), the tests 'filter' matches (every test with nullptr), then TearDown_Group().
    // Without a 'shuffleSeed', the tests run in the order GetTestCallbackList() gives; with one, in the order
    // ShuffledIndices() gives that seed, before the filter is applied, so a filtered run keeps the relative order of
    // an unfiltered one. Starts from empty Results(), so running the group again reports only the new run's outcomes.
    // With 'testTimeoutSeconds', each test runs on a worker thread (see RunWithTimeout()), and a test that doesn't
    // finish within that many seconds is abandoned and recorded as failed; the group's remaining tests don't run,
    // Results().TimedOut is set, and TearDown_Group() still runs. With 'catchCrashes', a test that crashes is
    // recorded as failed (see RunCatchingCrashes()); after a stack overflow, the remaining tests don't run, the same
    // way, with Results().Crashed set.
    virtual void Run(ITestFilter const* filter = nullptr, TOptional<unsigned int> shuffleSeed = TOptional<unsigned int>(),
        TOptional<unsigned int> testTimeoutSeconds = TOptional<unsigned int>(), bool catchCrashes = false);
    // Sets (or, with nullptr, clears) the observer that receives this group's log output and per-test events. Not
    // owned: it must outlive its use here.
    void SetRunObserver(ITestRunObserver* observer);
    virtual void SetUp_Group();    // Called once before the group's first test
    virtual void TearDown_Group(); // Called once after the group's last test
};

template <class TParam>
void TTestGroupBase::RegisterTestCases(typename TParamTestMethod<TParam>::Type method, std::string const& nameBase,
    std::vector<TParam> const& params, typename TParamTestNamer<TParam>::Type nameGenerator)
{
    for (std::size_t i = 0; i < params.size(); ++i)
    {
        std::string const label = (nameGenerator != nullptr) ? nameGenerator(params[i]) : DescribeUnsignedInteger(i);
        AddRegisteredTest(new TParamTestCase<TParam>(method, nameBase + "[" + label + "]", params[i]));
    }
}

template <class T, class TParam>
void TTestGroupBase::RegisterTestCases(void(__fastcall T::* method)(TParam const&), std::string const& nameBase,
    std::vector<TParam> const& params, typename TParamTestNamer<TParam>::Type nameGenerator)
{
    for (std::size_t i = 0; i < params.size(); ++i)
    {
        std::string const label = (nameGenerator != nullptr) ? nameGenerator(params[i]) : DescribeUnsignedInteger(i);
        AddRegisteredTest(new TMemberParamTestCase<T, TParam>(static_cast<T*>(this), method,
            nameBase + "[" + label + "]", params[i]));
    }
}

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_TestBaseH
