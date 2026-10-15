/* **************************************************************************
ASWUnitTestsBCB_JUnitReport.cpp
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
#include "ASWUnitTestsBCB_JUnitReport.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <iomanip>
#include <memory>
#include <sstream>
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Utils.h"
#include "ASWUnitTestsBCB_Values.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Classes;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

std::string FormatSeconds(double seconds);
std::string FormatUTCTimestamp();
void WriteTestCase(std::ostringstream& body, ASWUnitTestsBCB::TJUnitTestCase const& testCase);

//---------------------------------------------------------------------------
std::string FormatSeconds(double seconds)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6) << seconds;
    return stream.str();
}
//---------------------------------------------------------------------------
// Returns the current UTC time to the second, e.g. "2026-10-12T22:45:12Z".
std::string FormatUTCTimestamp()
{
    std::string const withMilliseconds = ASWUnitTestsBCB::GetUTCTimeISO8601(); // "2026-10-12T22:45:12.224Z"
    return withMilliseconds.substr(0, withMilliseconds.find('.')) + "Z";
}
//---------------------------------------------------------------------------
void WriteTestCase(std::ostringstream& body, ASWUnitTestsBCB::TJUnitTestCase const& testCase)
{
    using ASWUnitTestsBCB::TJUnitOutcome;
    using ASWUnitTestsBCB::TJUnitReportWriter;

    body << "    <testcase name=\"" << TJUnitReportWriter::EscapeXML(testCase.TestName) << "\" classname=\""
    << TJUnitReportWriter::EscapeXML(testCase.GroupName) << "\" time=\""
    << FormatSeconds(testCase.DurationSeconds) << "\"";

    if (testCase.Outcome == TJUnitOutcome::Pass)
    {
        body << "/>\n";
        return;
    }

    body << ">\n";

    if (testCase.Outcome == TJUnitOutcome::Fail)
    {
        body << "      <failure message=\"" << TJUnitReportWriter::EscapeXML(testCase.Message) << "\">"
        << TJUnitReportWriter::EscapeXML(testCase.Message) << "</failure>\n";
    }
    else // Skip
    {
        body << "      <skipped message=\"" << TJUnitReportWriter::EscapeXML(testCase.Message) << "\"/>\n";
    }

    body << "    </testcase>\n";
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TJUnitTestCase
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TJUnitTestCase::TJUnitTestCase()
    : DurationSeconds(0.0),
      Outcome(TJUnitOutcome::Pass)
{
}
//---------------------------------------------------------------------------
TJUnitTestCase::TJUnitTestCase(std::string const& groupName, std::string const& testName, double durationSeconds,
    TJUnitOutcome outcome, std::string const& message)
    : GroupName(groupName),
      TestName(testName),
      DurationSeconds(durationSeconds),
      Outcome(outcome),
      Message(message)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TJUnitReportWriter
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
std::string TJUnitReportWriter::BuildXML(std::string const& suitesName, std::vector<TJUnitTestCase> const& testCases)
{
    std::string const timestamp = FormatUTCTimestamp();
    std::ostringstream body;

    std::size_t totalTests = 0;
    std::size_t totalFailures = 0;
    std::size_t totalSkipped = 0;
    double totalTime = 0.0;

    std::size_t index = 0;
    while (index < testCases.size())
    {
        std::string const groupName = testCases[index].GroupName;
        std::size_t suiteTests = 0;
        std::size_t suiteFailures = 0;
        std::size_t suiteSkipped = 0;
        double suiteTime = 0.0;
        std::ostringstream suiteBody;

        while ((index < testCases.size()) && (testCases[index].GroupName == groupName))
        {
            TJUnitTestCase const& testCase = testCases[index];

            ++suiteTests;
            suiteTime += testCase.DurationSeconds;
            if (testCase.Outcome == TJUnitOutcome::Fail)
                ++suiteFailures;
            else if (testCase.Outcome == TJUnitOutcome::Skip)
                ++suiteSkipped;

            WriteTestCase(suiteBody, testCase);
            ++index;
        }

        body << "  <testsuite name=\"" << EscapeXML(groupName) << "\" tests=\"" << suiteTests << "\" failures=\""
        << suiteFailures << "\" skipped=\"" << suiteSkipped << "\" time=\"" << FormatSeconds(suiteTime)
        << "\" timestamp=\"" << timestamp << "\">\n" << suiteBody.str() << "  </testsuite>\n";

        totalTests += suiteTests;
        totalFailures += suiteFailures;
        totalSkipped += suiteSkipped;
        totalTime += suiteTime;
    }

    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml << "<testsuites name=\"" << EscapeXML(suitesName) << "\" tests=\"" << totalTests << "\" failures=\""
    << totalFailures << "\" skipped=\"" << totalSkipped << "\" time=\"" << FormatSeconds(totalTime) << "\">\n";
    xml << body.str();
    xml << "</testsuites>\n";

    return xml.str();
}
//---------------------------------------------------------------------------
std::string TJUnitReportWriter::EscapeXML(std::string const& text)
{
    std::string result;
    result.reserve(text.size());

    for (std::string::size_type i = 0; i < text.size(); ++i)
    {
        switch (text[i])
        {
            case '&':
                result += "&amp;";
                break;

            case '<':
                result += "&lt;";
                break;

            case '>':
                result += "&gt;";
                break;

            case '"':
                result += "&quot;";
                break;

            case '\'':
                result += "&apos;";
                break;

            default:
                result += text[i];
                break;
        }
    }

    return result;
}
//---------------------------------------------------------------------------
bool TJUnitReportWriter::Write(std::string const& filePath, std::string const& suitesName,
    std::vector<TJUnitTestCase> const& testCases)
{
    std::string const xml = BuildXML(suitesName, testCases);

    // A TFileStream rather than a std::ofstream, which can't open a non-ASCII path given in UTF-8, and would turn
    // each "\n" into "\r\n".
    try
    {
        std::unique_ptr<TFileStream> const file(new TFileStream(UnicodeText(filePath), fmCreate));
        file->WriteBuffer(xml.data(), static_cast<NativeInt>(xml.size()));
    }
    catch (Exception&)
    {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
