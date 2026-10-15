/* **************************************************************************
ASWUnitTestsBCB_JUnitReport.h
Author: Anthony S. West - ASW Software

JUnit-style XML test report writer, for --report-junit.

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

#ifndef ASWUnitTestsBCB_JUnitReportH
#define ASWUnitTestsBCB_JUnitReportH
//---------------------------------------------------------------------------
#include <string>
#include <vector>
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TJUnitOutcome
/////////////////////////////////////////////////////////////////////////////
struct TJUnitOutcome
{
    enum Type
    {
        Pass,
        Fail,
        Skip,
    };
};


/////////////////////////////////////////////////////////////////////////////
// TJUnitTestCase
//
// One <testcase> entry. 'Message' is the failure/skip detail text; ignored
// (and may be empty) when Outcome is Pass.
/////////////////////////////////////////////////////////////////////////////
struct TJUnitTestCase
{
    std::string GroupName;
    std::string TestName;
    double DurationSeconds;
    TJUnitOutcome::Type Outcome;
    std::string Message;

    TJUnitTestCase();
    TJUnitTestCase(std::string const& groupName, std::string const& testName, double durationSeconds,
        TJUnitOutcome::Type outcome, std::string const& message);
};


/////////////////////////////////////////////////////////////////////////////
// TJUnitReportWriter
//
// Writes a JUnit-style XML report (<testsuites>/<testsuite>/<testcase>),
// the de facto standard most CI systems (GitHub Actions, GitLab CI,
// Jenkins, Azure DevOps, CircleCI) natively recognize for test results.
// Self-contained, with its own XML escaping, so it needs no XML library.
/////////////////////////////////////////////////////////////////////////////
class TJUnitReportWriter
{
public:
    // The report's XML, UTF-8 encoded, exactly as Write() writes it. 'testCases' must already be grouped
    // contiguously by GroupName (as a full test run naturally produces them); each contiguous run of matching
    // GroupName becomes one <testsuite>, in the order first encountered.
    static std::string BuildXML(std::string const& suitesName, std::vector<TJUnitTestCase> const& testCases);

    // Escapes '&', '<', '>', '"', and '\'' for safe inclusion in XML text or attribute values.
    static std::string EscapeXML(std::string const& text);

    // Writes the report BuildXML() builds to 'filePath' (UTF-8, so any file name can be given), replacing any file
    // there. Returns false if the file couldn't be created or written.
    static bool Write(std::string const& filePath, std::string const& suitesName,
        std::vector<TJUnitTestCase> const& testCases);
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_JUnitReportH
