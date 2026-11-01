/* **************************************************************************
ASWUnitTestsBCB_Exception.h
Author: Anthony S. West - ASW Software

The exceptions that report a test's failure or skip, and the framework's own errors.

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

#ifndef ASWUnitTestsBCB_ExceptionH
#define ASWUnitTestsBCB_ExceptionH
//---------------------------------------------------------------------------
#include <exception>
#include <stdexcept>
#include <string>
//---------------------------------------------------------------------------
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

// Describes an exception for a failure message: what() for a std::exception, and "ClassName: Message" for an RTL
// exception (e.g. "EConvertError: 'abc' is not a valid integer value"), as UTF-8.
std::string DescribeException(std::exception const& ex);
std::string DescribeException(System::Sysutils::Exception& ex);

// Describes an exception that is neither a std::exception nor an RTL exception (caught with "catch (...)").
std::string DescribeUnknownException();

// Returns just 'ex.Message', as UTF-8.
std::string RTLExceptionMessage(System::Sysutils::Exception& ex);

// The failure exceptions are plain C++ classes, not RTL Exception descendants, so they never meet the RTL's rule
// that an Exception object is freed when the first handler that catches it ends. Their 'method' parameters take
// __FUNC__ (or __func__) as given; the message shows its bare method name (see BareMethodName()), so it reads the
// same with every compiler.

/////////////////////////////////////////////////////////////////////////////
// TTestException
//
// Base of the exceptions that end a test with an outcome other than an
// unexpected exception: a failed Assert* method, Fail(), Skip(), or an
// expected exception that never came. what() is the full message.
/////////////////////////////////////////////////////////////////////////////
class TTestException : public std::runtime_error
{
public:
    explicit TTestException(std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptExpected
//
// Thrown when a test that expected an exception (see
// TTestGroupBase::SetExceptionExpected()) finished without one.
/////////////////////////////////////////////////////////////////////////////
class TExceptExpected : public TTestException
{
public:
    explicit TExceptExpected(std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptEquals
/////////////////////////////////////////////////////////////////////////////
class TExceptEquals : public TTestException
{
public:
    // "Values not equal: <method> (<line>): <detail>"
    TExceptEquals(std::string const& method, int line, std::string const& detail);
    TExceptEquals(std::string const& method, int line, std::string const& expected, std::string const& actual,
        std::string const& msg, bool ignoreCase = false);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptNotEquals
/////////////////////////////////////////////////////////////////////////////
class TExceptNotEquals : public TTestException
{
public:
    // "Values are equal: <method> (<line>): <detail>"
    TExceptNotEquals(std::string const& method, int line, std::string const& detail);
    TExceptNotEquals(std::string const& method, int line, std::string const& value, std::string const& msg);
    TExceptNotEquals(std::string const& method, int line, std::string const& value, std::string const& otherValue,
        std::string const& msg, bool ignoreCase = false);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptEqualsMem
/////////////////////////////////////////////////////////////////////////////
class TExceptEqualsMem : public TTestException
{
public:
    TExceptEqualsMem(std::string const& method, int line, std::string const& detail, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptNotEqualsMem
/////////////////////////////////////////////////////////////////////////////
class TExceptNotEqualsMem : public TTestException
{
public:
    TExceptNotEqualsMem(std::string const& method, int line, std::string const& detail, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptContains
/////////////////////////////////////////////////////////////////////////////
class TExceptContains : public TTestException
{
public:
    TExceptContains(std::string const& method, int line, std::string const& text, std::string const& substring,
        std::string const& msg, bool ignoreCase = false);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptNotContains
/////////////////////////////////////////////////////////////////////////////
class TExceptNotContains : public TTestException
{
public:
    TExceptNotContains(std::string const& method, int line, std::string const& text, std::string const& substring,
        std::string const& msg, bool ignoreCase = false);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptStartsWith
/////////////////////////////////////////////////////////////////////////////
class TExceptStartsWith : public TTestException
{
public:
    TExceptStartsWith(std::string const& method, int line, std::string const& text, std::string const& prefix,
        std::string const& msg, bool ignoreCase = false);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptNotStartsWith
/////////////////////////////////////////////////////////////////////////////
class TExceptNotStartsWith : public TTestException
{
public:
    TExceptNotStartsWith(std::string const& method, int line, std::string const& text, std::string const& prefix,
        std::string const& msg, bool ignoreCase = false);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptEndsWith
/////////////////////////////////////////////////////////////////////////////
class TExceptEndsWith : public TTestException
{
public:
    TExceptEndsWith(std::string const& method, int line, std::string const& text, std::string const& suffix,
        std::string const& msg, bool ignoreCase = false);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptNotEndsWith
/////////////////////////////////////////////////////////////////////////////
class TExceptNotEndsWith : public TTestException
{
public:
    TExceptNotEndsWith(std::string const& method, int line, std::string const& text, std::string const& suffix,
        std::string const& msg, bool ignoreCase = false);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptMatches
/////////////////////////////////////////////////////////////////////////////
class TExceptMatches : public TTestException
{
public:
    TExceptMatches(std::string const& method, int line, std::string const& detail, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptNotMatches
/////////////////////////////////////////////////////////////////////////////
class TExceptNotMatches : public TTestException
{
public:
    TExceptNotMatches(std::string const& method, int line, std::string const& detail, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptOrdering
/////////////////////////////////////////////////////////////////////////////
class TExceptOrdering : public TTestException
{
public:
    // 'relation' is the operator the value failed, e.g. ">=".
    TExceptOrdering(std::string const& method, int line, std::string const& value, std::string const& relation,
        std::string const& bound, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptFalse
/////////////////////////////////////////////////////////////////////////////
class TExceptFalse : public TTestException
{
public:
    TExceptFalse(std::string const& method, int line, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptTrue
/////////////////////////////////////////////////////////////////////////////
class TExceptTrue : public TTestException
{
public:
    TExceptTrue(std::string const& method, int line, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptNull
/////////////////////////////////////////////////////////////////////////////
class TExceptNull : public TTestException
{
public:
    TExceptNull(std::string const& method, int line, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptNotNull
/////////////////////////////////////////////////////////////////////////////
class TExceptNotNull : public TTestException
{
public:
    TExceptNotNull(std::string const& method, int line, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptSame
/////////////////////////////////////////////////////////////////////////////
class TExceptSame : public TTestException
{
public:
    TExceptSame(std::string const& method, int line, std::string const& expected, std::string const& actual,
        std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptNotSame
/////////////////////////////////////////////////////////////////////////////
class TExceptNotSame : public TTestException
{
public:
    TExceptNotSame(std::string const& method, int line, std::string const& value, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptIsType
/////////////////////////////////////////////////////////////////////////////
class TExceptIsType : public TTestException
{
public:
    TExceptIsType(std::string const& method, int line, std::string const& detail, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptIsNotType
/////////////////////////////////////////////////////////////////////////////
class TExceptIsNotType : public TTestException
{
public:
    TExceptIsNotType(std::string const& method, int line, std::string const& detail, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptThrows
/////////////////////////////////////////////////////////////////////////////
class TExceptThrows : public TTestException
{
public:
    TExceptThrows(std::string const& method, int line, std::string const& detail, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptNoThrow
/////////////////////////////////////////////////////////////////////////////
class TExceptNoThrow : public TTestException
{
public:
    TExceptNoThrow(std::string const& method, int line, std::string const& detail, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptEmpty
/////////////////////////////////////////////////////////////////////////////
class TExceptEmpty : public TTestException
{
public:
    TExceptEmpty(std::string const& method, int line, std::string const& detail, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptNotEmpty
/////////////////////////////////////////////////////////////////////////////
class TExceptNotEmpty : public TTestException
{
public:
    TExceptNotEmpty(std::string const& method, int line, std::string const& detail, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptFail
//
// Thrown by TTestGroupBase::Fail() to abort the current test and have it
// reported as failed.
/////////////////////////////////////////////////////////////////////////////
class TExceptFail : public TTestException
{
public:
    TExceptFail(std::string const& method, int line, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptSkipped
//
// Thrown by TTestGroupBase::Skip() to abort the current test and have it
// reported as skipped rather than passed or failed.
/////////////////////////////////////////////////////////////////////////////
class TExceptSkipped : public TTestException
{
public:
    TExceptSkipped(std::string const& method, int line, std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptAbortRun
//
// Common base for a signal that a single test's failure is severe enough
// that running any further tests or groups in this process isn't safe
// (e.g. a test that timed out). The runner stops after recording the
// failure of the test that caused it.
/////////////////////////////////////////////////////////////////////////////
class TExceptAbortRun : public TTestException
{
public:
    explicit TExceptAbortRun(std::string const& msg);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptFatalError
//
// Thrown when a test group's SetUp_Test(), TearDown_Test() or
// TearDown_Group() throws, which ends the run. what() names the method and
// describes the exception it threw. Not a TTestException, so no test can
// take it for its own outcome.
/////////////////////////////////////////////////////////////////////////////
class TExceptFatalError : public std::runtime_error
{
private:
    bool m_UnknownException;

public:
    // 'unknownException' says whether the method threw something that is neither a std::exception nor an RTL
    // exception, which the program's exit code tells apart.
    TExceptFatalError(std::string const& msg, bool unknownException);

    bool IsUnknownException() const;
};


/////////////////////////////////////////////////////////////////////////////
// TExceptDuplicateTestName
//
// Thrown when a test group class publishes two methods with the same test
// name (overloads), which the published-method table can't tell apart. A
// mistake in the test code rather than a test's outcome, so it isn't a
// TTestException; it stops the run before any test runs.
/////////////////////////////////////////////////////////////////////////////
class TExceptDuplicateTestName : public std::logic_error
{
private:
    explicit TExceptDuplicateTestName(std::string const& msg);

public:
    TExceptDuplicateTestName(std::string const& groupClassName, std::string const& methodName);

    // Two of a group's tests, at least one of them registered (see TTestGroupBase::RegisterTestCases()), named
    // alike.
    static TExceptDuplicateTestName Registered(std::string const& groupName, std::string const& testName);
};


/////////////////////////////////////////////////////////////////////////////
// TExceptInvalidSuitePath
//
// Thrown by TTestHandler for a suite path with an empty suite name: one
// that starts or ends with '.', or has two in a row (see TSuitePath). Like
// TExceptDuplicateTestName, it escapes TTestHandler::Initialize() and
// stops the run before any test runs, whether the path comes from a test
// group's constructor or its registration (see
// ASWBCB_REGISTER_TEST_GROUP_IN_SUITE), or the two give a group different
// suite paths (see Conflicting()).
/////////////////////////////////////////////////////////////////////////////
class TExceptInvalidSuitePath : public std::logic_error
{
private:
    // 'msg' is the whole message; 'isMessage' only tells this constructor apart from the public one.
    TExceptInvalidSuitePath(std::string const& msg, bool isMessage);

public:
    explicit TExceptInvalidSuitePath(std::string const& suitePath);

    // A group registered in suite 'registeredPath' whose constructor put it in a different one,
    // 'constructedPath'.
    static TExceptInvalidSuitePath Conflicting(std::string const& groupName, std::string const& registeredPath,
        std::string const& constructedPath);
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_ExceptionH
