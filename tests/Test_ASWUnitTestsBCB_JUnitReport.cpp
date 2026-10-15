/* **************************************************************************
Test_ASWUnitTestsBCB_JUnitReport.cpp
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_JUnitReport.h.

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
#include "Test_ASWUnitTestsBCB_JUnitReport.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.IOUtils.hpp>
#include <System.SysUtils.hpp>
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_JUnitReport.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Ioutils;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

std::string ReadFileBytes(UnicodeString const& path);
std::vector<ASWUnitTestsBCB::TJUnitTestCase> ThreeTestCases();
std::string WithoutTimestamps(std::string const& xml);

//---------------------------------------------------------------------------
// Returns the file's bytes as they are.
std::string ReadFileBytes(UnicodeString const& path)
{
    TBytes bytes = TFile::ReadAllBytes(path);

    if (bytes.Length == 0)
        return std::string();

    return std::string(reinterpret_cast<char const*>(&bytes[0]), static_cast<std::string::size_type>(bytes.Length));
}
//---------------------------------------------------------------------------
// Returns a pass and a failure in one group and a skip in another, with text to escape.
std::vector<ASWUnitTestsBCB::TJUnitTestCase> ThreeTestCases()
{
    using ASWUnitTestsBCB::TJUnitOutcome;
    using ASWUnitTestsBCB::TJUnitTestCase;

    std::vector<TJUnitTestCase> testCases;
    testCases.push_back(TJUnitTestCase("GroupA", "TestPass", 0.25, TJUnitOutcome::Pass, ""));
    testCases.push_back(TJUnitTestCase("GroupA", "TestFail", 0.5, TJUnitOutcome::Fail, "expected <1> & \"2\""));
    testCases.push_back(TJUnitTestCase("GroupB", "Test'Skip", 0.0, TJUnitOutcome::Skip, "not implemented yet"));
    return testCases;
}
//---------------------------------------------------------------------------
// Returns 'xml' with each timestamp attribute's value replaced by "*", so reports made at different times compare.
std::string WithoutTimestamps(std::string const& xml)
{
    std::string const attribute = "timestamp=\"";
    std::string result = xml;
    std::string::size_type pos = result.find(attribute);

    while (pos != std::string::npos)
    {
        std::string::size_type const valuePos = pos + attribute.size();
        std::string::size_type const endPos = result.find('"', valuePos);

        if (endPos == std::string::npos)
            break;

        result.replace(valuePos, endPos - valuePos, "*");
        pos = result.find(attribute, valuePos);
    }

    return result;
}
//---------------------------------------------------------------------------

} // namespace

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_JUnitReport
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_JUnitReport::TTest_ASWUnitTestsBCB_JUnitReport()
    : inherited("ASWUnitTestsBCB_JUnitReport_Tests"),
      m_TempFolder(TPath::Combine(TPath::GetTempPath(), UnicodeString("ASWUnitTestsBCB_JUnitReport_Tests_") +
          UIntToStr(static_cast<unsigned int>(::GetCurrentProcessId()))))
{
}
//---------------------------------------------------------------------------
void TTest_ASWUnitTestsBCB_JUnitReport::SetUp_Group()
{
    Log("Setting up temp group folder: " + ToUTF8(m_TempFolder));
    TDirectory::CreateDirectory(m_TempFolder);
}
//---------------------------------------------------------------------------
void TTest_ASWUnitTestsBCB_JUnitReport::TearDown_Group()
{
    Log("Cleaning up temp group folder: " + ToUTF8(m_TempFolder));

    if (TDirectory::Exists(m_TempFolder))
        TDirectory::Delete(m_TempFolder, true);
}
//---------------------------------------------------------------------------

// /////// Begin tests after this line ///////////////////////

//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_JUnitReport::Test_BuildXML_CountsAndEscapes()
{
    std::string const xml = TJUnitReportWriter::BuildXML("My<Project>", ThreeTestCases());

    CheckEquals(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<testsuites name=\"My&lt;Project&gt;\" tests=\"3\" failures=\"1\" skipped=\"1\" time=\"0.750000\">\n"
        "  <testsuite name=\"GroupA\" tests=\"2\" failures=\"1\" skipped=\"0\" time=\"0.750000\" timestamp=\"*\">\n"
        "    <testcase name=\"TestPass\" classname=\"GroupA\" time=\"0.250000\"/>\n"
        "    <testcase name=\"TestFail\" classname=\"GroupA\" time=\"0.500000\">\n"
        "      <failure message=\"expected &lt;1&gt; &amp; &quot;2&quot;\">expected &lt;1&gt; &amp; &quot;2&quot;"
        "</failure>\n"
        "    </testcase>\n"
        "  </testsuite>\n"
        "  <testsuite name=\"GroupB\" tests=\"1\" failures=\"0\" skipped=\"1\" time=\"0.000000\" timestamp=\"*\">\n"
        "    <testcase name=\"Test&apos;Skip\" classname=\"GroupB\" time=\"0.000000\">\n"
        "      <skipped message=\"not implemented yet\"/>\n"
        "    </testcase>\n"
        "  </testsuite>\n"
        "</testsuites>\n", WithoutTimestamps(xml), ASWBCB_HERE);

    // Each <testsuite> has the report's time, in UTC to the second.
    std::string const timestampAttribute = "timestamp=\"";
    std::string::size_type const firstPos = xml.find(timestampAttribute);
    std::string::size_type const secondPos = xml.find(timestampAttribute, firstPos + 1);
    AssertTrue((firstPos != std::string::npos) && (secondPos != std::string::npos), ASWBCB_HERE, "two timestamps");

    std::string const timestamp = xml.substr(firstPos + timestampAttribute.size(), 21);
    CheckMatches(timestamp, "^\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}Z\"$", ASWBCB_HERE);
    CheckEquals(timestamp, xml.substr(secondPos + timestampAttribute.size(), 21), ASWBCB_HERE, "the same in both");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_JUnitReport::Test_BuildXML_NoTestCases()
{
    CheckEquals(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<testsuites name=\"Empty\" tests=\"0\" failures=\"0\" skipped=\"0\" time=\"0.000000\">\n"
        "</testsuites>\n", TJUnitReportWriter::BuildXML("Empty", std::vector<TJUnitTestCase>()), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_JUnitReport::Test_EscapeXML_PlainTextUnchanged()
{
    std::string const plain = "Test_HexSingleToByte[A] passed in 1.234 ms";

    CheckEquals(plain, TJUnitReportWriter::EscapeXML(plain), ASWBCB_HERE, "plain text passes through unchanged");
    CheckEquals("caf\xC3\xA9", TJUnitReportWriter::EscapeXML("caf\xC3\xA9"), ASWBCB_HERE, "UTF-8 as it is");
    CheckEquals("", TJUnitReportWriter::EscapeXML(""), ASWBCB_HERE, "empty string");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_JUnitReport::Test_EscapeXML_SpecialCharacters()
{
    CheckEquals("&amp;", TJUnitReportWriter::EscapeXML("&"), ASWBCB_HERE, "ampersand");
    CheckEquals("&lt;", TJUnitReportWriter::EscapeXML("<"), ASWBCB_HERE, "less-than");
    CheckEquals("&gt;", TJUnitReportWriter::EscapeXML(">"), ASWBCB_HERE, "greater-than");
    CheckEquals("&quot;", TJUnitReportWriter::EscapeXML("\""), ASWBCB_HERE, "double quote");
    CheckEquals("&apos;", TJUnitReportWriter::EscapeXML("'"), ASWBCB_HERE, "apostrophe");

    // A message realistically produced by a failing Check/Assert call, exercising all five together.
    CheckEquals("Expected &quot;a&quot; but was &lt;b&gt; &amp; &apos;c&apos;",
        TJUnitReportWriter::EscapeXML("Expected \"a\" but was <b> & 'c'"), ASWBCB_HERE, "combined failure message");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_JUnitReport::Test_Write_FailsWithoutFolder()
{
    UnicodeString const reportPath = TPath::Combine(TPath::Combine(m_TempFolder, "Missing"), "report.xml");

    CheckFalse(TJUnitReportWriter::Write(ToUTF8(reportPath), "MyProject", ThreeTestCases()), ASWBCB_HERE);
    CheckFalse(TFile::Exists(reportPath), ASWBCB_HERE, "no file");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_JUnitReport::Test_Write_ProducesReadableReport()
{
    // An omega (U+03A9) in the name, which no single-byte ANSI code page that has the Latin letters has, so it only
    // arrives intact if the UTF-8 path is opened as UTF-8.
    UnicodeString const reportPath = TPath::Combine(m_TempFolder, L"report-\x03A9.xml");
    std::string const projectName = "caf\xC3\xA9";

    AssertTrue(TJUnitReportWriter::Write(ToUTF8(reportPath), projectName, ThreeTestCases()), ASWBCB_HERE,
        "Write() reports success");
    AssertTrue(TFile::Exists(reportPath), ASWBCB_HERE, "the file exists, by its Unicode name");

    // Exactly BuildXML()'s bytes: UTF-8 without a byte order mark, and "\n" line ends.
    CheckEquals(WithoutTimestamps(TJUnitReportWriter::BuildXML(projectName, ThreeTestCases())),
        WithoutTimestamps(ReadFileBytes(reportPath)), ASWBCB_HERE);

    // A second report replaces the first.
    AssertTrue(TJUnitReportWriter::Write(ToUTF8(reportPath), "Empty", std::vector<TJUnitTestCase>()), ASWBCB_HERE,
        "the second Write()");
    CheckEquals(TJUnitReportWriter::BuildXML("Empty", std::vector<TJUnitTestCase>()), ReadFileBytes(reportPath),
        ASWBCB_HERE, "the shorter second report, all of the file");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_JUnitReport, "SelfTests")
