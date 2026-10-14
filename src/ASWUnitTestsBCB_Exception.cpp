/* **************************************************************************
ASWUnitTestsBCB_Exception.cpp
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

#include <System.hpp>
#pragma hdrstop

//---------------------------------------------------------------------------
// Module header
#include "ASWUnitTestsBCB_Exception.h"
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

// The message of a failed Contains, StartsWith or EndsWith assertion (or a Not form), e.g. "Substring not found:
// Method (Line): Expected "text" to contain "part". msg".
std::string TextPartMessage(std::string const& title, std::string const& method, int line, std::string const& text,
    std::string const& relation, std::string const& part, std::string const& msg, bool ignoreCase)
{
    // Qualified, since bcc32 doesn't find a function of an enclosing namespace from an anonymous one when an argument
    // has a class type.
    return title + ": " + ASWUnitTestsBCB::FormatLocation(method, line) + ": Expected \"" + text + "\" " + relation +
        " \"" + part + "\"" + (ignoreCase ? " (ignoring case)" : "") + ". " + msg;
}

} // namespace


namespace ASWUnitTestsBCB
{

//---------------------------------------------------------------------------
std::string DescribeException(std::exception const& ex)
{
    return ex.what();
}
//---------------------------------------------------------------------------
std::string DescribeException(Exception& ex)
{
    return ToUTF8(ex.ClassName()) + ": " + RTLExceptionMessage(ex);
}
//---------------------------------------------------------------------------
std::string DescribeUnknownException()
{
    return "an object that is neither a std::exception nor an RTL Exception";
}
//---------------------------------------------------------------------------
std::string RTLExceptionMessage(Exception& ex)
{
    return ToUTF8(ex.Message);
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptAbortRun
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptAbortRun::TExceptAbortRun(std::string const& msg)
    : TTestException(msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptContains
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptContains::TExceptContains(std::string const& method, int line, std::string const& text,
    std::string const& substring, std::string const& msg, bool ignoreCase)
    : TTestException(TextPartMessage("Substring not found", method, line, text, "to contain", substring, msg,
          ignoreCase))
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptDuplicateTestName
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptDuplicateTestName::TExceptDuplicateTestName(std::string const& msg)
    : std::logic_error(msg)
{
}
//---------------------------------------------------------------------------
TExceptDuplicateTestName::TExceptDuplicateTestName(std::string const& groupClassName, std::string const& methodName)
    : std::logic_error("Test group class \"" + groupClassName + "\" publishes more than one method named \"" +
          methodName + "\"")
{
}
//---------------------------------------------------------------------------
TExceptDuplicateTestName TExceptDuplicateTestName::Registered(std::string const& groupName,
    std::string const& testName)
{
    return TExceptDuplicateTestName("Test group \"" + groupName + "\" registers more than one test named \"" +
        testName + "\"");
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptEmpty
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptEmpty::TExceptEmpty(std::string const& method, int line, std::string const& detail, std::string const& msg)
    : TTestException("Not empty: " + FormatLocation(method, line) + ": " + detail + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptEndsWith
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptEndsWith::TExceptEndsWith(std::string const& method, int line, std::string const& text,
    std::string const& suffix, std::string const& msg, bool ignoreCase)
    : TTestException(TextPartMessage("Suffix not found", method, line, text, "to end with", suffix, msg, ignoreCase))
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptEquals
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptEquals::TExceptEquals(std::string const& method, int line, std::string const& expected,
    std::string const& actual, std::string const& msg, bool ignoreCase)
    : TTestException("Values not equal: " + FormatLocation(method, line) + ": Expected: \"" + expected +
          "\" but was \"" + actual + "\"" + (ignoreCase ? " (ignoring case)" : "") + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptEqualsMem
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptEqualsMem::TExceptEqualsMem(std::string const& method, int line, std::string const& detail,
    std::string const& msg)
    : TTestException("Memory not equal: " + FormatLocation(method, line) + ": " + detail + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptExpected
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptExpected::TExceptExpected(std::string const& msg)
    : TTestException("Exception expected: " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptFail
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptFail::TExceptFail(std::string const& method, int line, std::string const& msg)
    : TTestException("Failed: " + FormatLocation(method, line) + ": " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptFatalError
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptFatalError::TExceptFatalError(std::string const& msg, bool unknownException)
    : std::runtime_error(msg),
      m_UnknownException(unknownException)
{
}
//---------------------------------------------------------------------------
bool TExceptFatalError::IsUnknownException() const
{
    return m_UnknownException;
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptFalse
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptFalse::TExceptFalse(std::string const& method, int line, std::string const& msg)
    : TTestException("Expected false but was true: " + FormatLocation(method, line) + ": " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptInvalidSuitePath
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptInvalidSuitePath::TExceptInvalidSuitePath(std::string const& msg, bool /*isMessage*/)
    : std::logic_error(msg)
{
}
//---------------------------------------------------------------------------
TExceptInvalidSuitePath::TExceptInvalidSuitePath(std::string const& suitePath)
    : std::logic_error("Suite path \"" + suitePath + "\" has an empty suite name; separate its names with single "
          "'.' characters, with none at the start or end")
{
}
//---------------------------------------------------------------------------
TExceptInvalidSuitePath TExceptInvalidSuitePath::Conflicting(std::string const& groupName,
    std::string const& registeredPath, std::string const& constructedPath)
{
    return TExceptInvalidSuitePath("Test group \"" + groupName + "\" is registered in suite \"" + registeredPath +
        "\", but its constructor puts it in suite \"" + constructedPath + "\"", true);
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptIsNotType
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptIsNotType::TExceptIsNotType(std::string const& method, int line, std::string const& detail,
    std::string const& msg)
    : TTestException("Type matched: " + FormatLocation(method, line) + ": " + detail + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptIsType
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptIsType::TExceptIsType(std::string const& method, int line, std::string const& detail, std::string const& msg)
    : TTestException("Type mismatch: " + FormatLocation(method, line) + ": " + detail + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptMatches
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptMatches::TExceptMatches(std::string const& method, int line, std::string const& detail, std::string const& msg)
    : TTestException("Pattern not matched: " + FormatLocation(method, line) + ": " + detail + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptNoThrow
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptNoThrow::TExceptNoThrow(std::string const& method, int line, std::string const& detail, std::string const& msg)
    : TTestException("Unexpected exception: " + FormatLocation(method, line) + ": " + detail + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptNotContains
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptNotContains::TExceptNotContains(std::string const& method, int line, std::string const& text,
    std::string const& substring, std::string const& msg, bool ignoreCase)
    : TTestException(TextPartMessage("Substring found", method, line, text, "not to contain", substring, msg,
          ignoreCase))
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptNotEmpty
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptNotEmpty::TExceptNotEmpty(std::string const& method, int line, std::string const& detail,
    std::string const& msg)
    : TTestException("Empty: " + FormatLocation(method, line) + ": " + detail + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptNotEndsWith
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptNotEndsWith::TExceptNotEndsWith(std::string const& method, int line, std::string const& text,
    std::string const& suffix, std::string const& msg, bool ignoreCase)
    : TTestException(TextPartMessage("Suffix found", method, line, text, "not to end with", suffix, msg, ignoreCase))
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptNotEquals
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptNotEquals::TExceptNotEquals(std::string const& method, int line, std::string const& value,
    std::string const& msg)
    : TTestException("Values are equal: " + FormatLocation(method, line) + ": Value: \"" + value + "\". " + msg)
{
}
//---------------------------------------------------------------------------
TExceptNotEquals::TExceptNotEquals(std::string const& method, int line, std::string const& value,
    std::string const& otherValue, std::string const& msg, bool ignoreCase)
    : TTestException("Values are equal: " + FormatLocation(method, line) + ": Values: \"" + value + "\" and \"" +
          otherValue + "\"" + (ignoreCase ? " (ignoring case)" : "") + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptNotEqualsMem
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptNotEqualsMem::TExceptNotEqualsMem(std::string const& method, int line, std::string const& detail,
    std::string const& msg)
    : TTestException("Memory equal: " + FormatLocation(method, line) + ": " + detail + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptNotMatches
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptNotMatches::TExceptNotMatches(std::string const& method, int line, std::string const& detail,
    std::string const& msg)
    : TTestException("Pattern matched: " + FormatLocation(method, line) + ": " + detail + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptNotNull
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptNotNull::TExceptNotNull(std::string const& method, int line, std::string const& msg)
    : TTestException("Expected not null but was null: " + FormatLocation(method, line) + ": " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptNotSame
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptNotSame::TExceptNotSame(std::string const& method, int line, std::string const& value, std::string const& msg)
    : TTestException("Same object: " + FormatLocation(method, line) + ": Expected a different object but both are \"" +
          value + "\". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptNotStartsWith
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptNotStartsWith::TExceptNotStartsWith(std::string const& method, int line, std::string const& text,
    std::string const& prefix, std::string const& msg, bool ignoreCase)
    : TTestException(TextPartMessage("Prefix found", method, line, text, "not to start with", prefix, msg,
          ignoreCase))
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptNull
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptNull::TExceptNull(std::string const& method, int line, std::string const& msg)
    : TTestException("Expected null but was not null: " + FormatLocation(method, line) + ": " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptOrdering
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptOrdering::TExceptOrdering(std::string const& method, int line, std::string const& value,
    std::string const& relation, std::string const& bound, std::string const& msg)
    : TTestException("Values out of order: " + FormatLocation(method, line) + ": Expected " + value + " to be " +
          relation + " " + bound + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptSame
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptSame::TExceptSame(std::string const& method, int line, std::string const& expected, std::string const& actual,
    std::string const& msg)
    : TTestException("Not the same object: " + FormatLocation(method, line) + ": Expected the same object as \"" +
          expected + "\" but was \"" + actual + "\". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptSkipped
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptSkipped::TExceptSkipped(std::string const& method, int line, std::string const& msg)
    : TTestException("Test skipped: " + FormatLocation(method, line) + ": " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptStartsWith
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptStartsWith::TExceptStartsWith(std::string const& method, int line, std::string const& text,
    std::string const& prefix, std::string const& msg, bool ignoreCase)
    : TTestException(TextPartMessage("Prefix not found", method, line, text, "to start with", prefix, msg,
          ignoreCase))
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptThrows
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptThrows::TExceptThrows(std::string const& method, int line, std::string const& detail, std::string const& msg)
    : TTestException("Expected exception not caught: " + FormatLocation(method, line) + ": " + detail + ". " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TExceptTrue
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TExceptTrue::TExceptTrue(std::string const& method, int line, std::string const& msg)
    : TTestException("Expected true but was false: " + FormatLocation(method, line) + ": " + msg)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TTestException
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TTestException::TTestException(std::string const& msg)
    : std::runtime_error(msg)
{
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
