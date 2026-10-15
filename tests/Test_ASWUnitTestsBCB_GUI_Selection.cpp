/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_Selection.cpp
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
#include "Test_ASWUnitTestsBCB_GUI_Selection.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_Layout.h"
#include "ASWUnitTestsBCB_GUI_Selection.h"
#include "ASWUnitTestsBCB_GUI_TestList.h"
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TAllButFilter
//
// Matches every test but the one it names.
/////////////////////////////////////////////////////////////////////////////
class TAllButFilter : public ASWUnitTestsBCB::ITestFilter
{
private:
    std::string m_Excluded;

public:
    explicit TAllButFilter(std::string const& excluded)
        : m_Excluded(excluded)
    {
    }

    virtual bool Matches(std::string const& fullTestName) const
    {
        return fullTestName != m_Excluded;
    }
};


/////////////////////////////////////////////////////////////////////////////
// TSaved
//
// Builds a list of saved tests one call at a time, e.g.
// TSaved()(true, "Alpha", "One")(false, "Beta", "Two").Tests.
/////////////////////////////////////////////////////////////////////////////
struct TSaved
{
    std::vector<ASWUnitTestsBCB::TGUISavedTest> Tests;

    TSaved& operator()(bool checked, std::string const& groupName, std::string const& testName)
    {
        Tests.push_back(ASWUnitTestsBCB::TGUISavedTest(checked, groupName, testName));
        return *this;
    }
};

std::vector<ASWUnitTestsBCB::TTestId> SampleTests();

//---------------------------------------------------------------------------
// Four tests in three groups: indexes 0 and 1 in "Alpha", 2 in "Beta", 3 in "Gamma".
std::vector<ASWUnitTestsBCB::TTestId> SampleTests()
{
    std::vector<ASWUnitTestsBCB::TTestId> tests;
    tests.push_back(ASWUnitTestsBCB::TTestId("Alpha", "One"));
    tests.push_back(ASWUnitTestsBCB::TTestId("Alpha", "Two"));
    tests.push_back(ASWUnitTestsBCB::TTestId("Beta", "Three"));
    tests.push_back(ASWUnitTestsBCB::TTestId("Gamma", "Four"));
    return tests;
}
//---------------------------------------------------------------------------

} // namespace


namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_Selection
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_GUI_Selection::TTest_ASWUnitTestsBCB_GUI_Selection()
    : inherited("ASWUnitTestsBCB_GUI_Selection_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_CaptureGUISelection_SavesWhatRunSelectedWouldRun()
{
    // Arrange: "Beta.Three" is checked but hidden, so Run Selected wouldn't run it.
    TGUITestList list;
    list.Load(SampleTests());
    list.SetChecked(1, false);
    TAllButFilter const allButBeta("Beta.Three");
    list.SetVisibleFilter(&allButBeta);

    // Act
    std::vector<TGUISavedTest> const saved = CaptureGUISelection(list);

    // Assert
    AssertEquals(4, saved.size(), ASWBCB_HERE, "every test, checked or not");
    CheckEquals("Alpha", saved[0].GroupName, ASWBCB_HERE, "group name");
    CheckEquals("One", saved[0].TestName, ASWBCB_HERE, "test name");
    CheckTrue(saved[0].Checked, ASWBCB_HERE, "a checked, shown test is saved checked");
    CheckFalse(saved[1].Checked, ASWBCB_HERE, "an unchecked test is saved unchecked");
    CheckFalse(saved[2].Checked, ASWBCB_HERE, "a checked but hidden test is saved unchecked");
    CheckTrue(saved[3].Checked, ASWBCB_HERE, "another checked, shown test");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_DescribeGUISelectionRestore_NotesMissingAndNewTests()
{
    // Arrange
    TGUISelectionRestore plain;
    plain.CheckedCount = 1;

    TGUISelectionRestore changed;
    changed.CheckedCount = 5;
    changed.MissingCount = 2;
    changed.NewCount = 3;
    changed.NewCheckedCount = 1;

    TGUISelectionRestore single;
    single.CheckedCount = 2;
    single.MissingCount = 1;
    single.NewCount = 1;

    // Act
    std::string const plainText = DescribeGUISelectionRestore(plain);
    std::string const changedText = DescribeGUISelectionRestore(changed);
    std::string const singleText = DescribeGUISelectionRestore(single);

    // Assert
    CheckEquals("Restored the saved selection: 1 test checked.", plainText, ASWBCB_HERE, "nothing missing or new");
    CheckEquals("Restored the saved selection: 5 tests checked; 2 saved tests no longer exist; "
        "3 new or renamed tests (1 checked).", changedText, ASWBCB_HERE, "missing and new tests noted");
    CheckEquals("Restored the saved selection: 2 tests checked; 1 saved test no longer exists; "
        "1 new or renamed test (0 checked).", singleText, ASWBCB_HERE, "in the singular");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_DescribeGUISelectionRestore_NotesWhenEveryTestWasChecked()
{
    // Arrange
    TGUISelectionRestore restore;
    restore.CheckedAll = true;
    restore.CheckedCount = 4;

    // Act
    std::string const text = DescribeGUISelectionRestore(restore);

    // Assert
    CheckEquals("None of the saved selection's checked tests exist any more, so every test is checked.", text,
        ASWBCB_HERE, "the fallback is explained");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_FormatGUISelection_LeavesOutNamesItCannotStore()
{
    // Arrange
    std::vector<TGUISavedTest> const tests = TSaved()(true, "Alpha", "One")(true, "Alpha", "Tab\tName")(false,
        "Line\nBreak", "Two")(false, "Return\rHere", "Three").Tests;

    // Act
    std::vector<TGUISavedTest> const parsed = ParseGUISelection(FormatGUISelection(tests));

    // Assert
    AssertEquals(1, parsed.size(), ASWBCB_HERE, "only the test it can store");
    CheckEquals("One", parsed[0].TestName, ASWBCB_HERE, "the storable test");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_FormatGUISelection_RoundTripsThroughParse()
{
    // Arrange: names with spaces, dots, brackets, and a UTF-8 'e' with acute accent.
    std::vector<TGUISavedTest> const tests = TSaved()(true, "Alpha.Sub", "One[row 1]")(false, "Beta", "Two = 2")(true,
        "Caf\xC3\xA9", "Three").Tests;

    // Act
    std::string const text = FormatGUISelection(tests);
    std::vector<TGUISavedTest> const parsed = ParseGUISelection(text);

    // Assert
    CheckEquals("; ASWUnitTestsBCB VCL GUI test selection: \"+\" checked or \"-\" unchecked, then the group and "
        "test names, separated by a tab\r\n"
        "+Alpha.Sub\tOne[row 1]\r\n"
        "-Beta\tTwo = 2\r\n"
        "+Caf\xC3\xA9\tThree\r\n", text, ASWBCB_HERE, "a comment describing the file, then a line per test");
    AssertEquals(tests.size(), parsed.size(), ASWBCB_HERE, "every test read back");

    for (std::size_t i = 0; i < tests.size(); ++i)
    {
        std::string const which = UIntToString(static_cast<unsigned int>(i));
        CheckEquals(tests[i].GroupName, parsed[i].GroupName, ASWBCB_HERE, "group name " + which);
        CheckEquals(tests[i].TestName, parsed[i].TestName, ASWBCB_HERE, "test name " + which);
        CheckEquals(tests[i].Checked, parsed[i].Checked, ASWBCB_HERE, "check state " + which);
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_GUISelectionFilePath_IsBesideTheLayoutFile()
{
    // Act
    UnicodeString const selectionPath = GUISelectionFilePath();
    UnicodeString const layoutPath = GUILayoutFilePath();

    // Assert
    CheckEquals(ChangeFileExt(layoutPath, ".selection"), selectionPath, ASWBCB_HERE,
        "the layout file's path with a .selection extension");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_ParseGUISelection_SkipsLinesItCannotRead()
{
    // Arrange: a byte order mark, CRLF and LF line ends, and lines that aren't tests.
    std::string const text = "\xEF\xBB\xBF; comment\r\n"
        "+Alpha\tOne\r\n"
        "\r\n"
        "Alpha\tNo sign\n"
        "+No tab\n"
        "-Beta\tTwo";

    // Act
    std::vector<TGUISavedTest> const parsed = ParseGUISelection(text);

    // Assert
    AssertEquals(2, parsed.size(), ASWBCB_HERE, "only the two test lines");
    CheckEquals("Alpha", parsed[0].GroupName, ASWBCB_HERE, "first group, past the BOM");
    CheckEquals("One", parsed[0].TestName, ASWBCB_HERE, "first test, without the CR");
    CheckTrue(parsed[0].Checked, ASWBCB_HERE, "'+' is checked");
    CheckEquals("Two", parsed[1].TestName, ASWBCB_HERE, "the last line needs no line end");
    CheckFalse(parsed[1].Checked, ASWBCB_HERE, "'-' is unchecked");

    // A byte order mark right before a test line, with no comment line to hide it.
    std::vector<TGUISavedTest> const afterMark = ParseGUISelection("\xEF\xBB\xBF+Alpha\tOne\r\n");
    AssertEquals(1, afterMark.size(), ASWBCB_HERE, "the test line after the byte order mark");
    CheckEquals("Alpha", afterMark[0].GroupName, ASWBCB_HERE, "its group, without the mark");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_RestoreGUISelection_ChecksEveryTestIfNoSavedCheckedTestExists()
{
    // Arrange: the only checked test was renamed, and its group was only partly checked.
    TGUITestList list;
    list.Load(SampleTests());
    std::vector<TGUISavedTest> const saved = TSaved()(true, "Alpha", "Renamed")(false, "Alpha", "One")(false, "Beta",
        "Three").Tests;

    // Act
    TGUISelectionRestore const restore = RestoreGUISelection(list, saved);

    // Assert
    CheckTrue(restore.CheckedAll, ASWBCB_HERE, "falls back to every test");
    CheckEquals(4, list.CheckedCount(), ASWBCB_HERE, "every test is checked");
    CheckEquals(4, restore.CheckedCount, ASWBCB_HERE, "and counted");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_RestoreGUISelection_ChecksNewGroupsOnlyIfEveryTestWasChecked()
{
    // Arrange: Gamma is a new group in both cases.
    std::vector<TGUISavedTest> const allChecked = TSaved()(true, "Alpha", "One")(true, "Alpha", "Two")(true, "Beta",
        "Three").Tests;
    std::vector<TGUISavedTest> const someChecked = TSaved()(true, "Alpha", "One")(true, "Alpha", "Two")(false,
        "Beta", "Three").Tests;

    TGUITestList afterAll;
    afterAll.Load(SampleTests());
    TGUITestList afterSome;
    afterSome.Load(SampleTests());

    // Act
    RestoreGUISelection(afterAll, allChecked);
    RestoreGUISelection(afterSome, someChecked);

    // Assert
    CheckTrue(afterAll.IsChecked(3), ASWBCB_HERE, "a new group is checked when every test was");
    CheckFalse(afterSome.IsChecked(3), ASWBCB_HERE, "but not when only some were");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_RestoreGUISelection_ChecksNewTestsLikeTheirGroup()
{
    // Arrange: "Alpha.Two" and "Beta.Three" are new; Alpha was saved fully checked, Beta only partly.
    TGUITestList list;
    list.Load(SampleTests());
    std::vector<TGUISavedTest> const saved = TSaved()(true, "Alpha", "One")(true, "Beta", "Failed")(false, "Beta",
        "Passed")(false, "Gamma", "Four").Tests;

    // Act
    TGUISelectionRestore const restore = RestoreGUISelection(list, saved);

    // Assert
    CheckTrue(list.IsChecked(1), ASWBCB_HERE, "a new test in a fully checked group is checked");
    CheckFalse(list.IsChecked(2), ASWBCB_HERE, "a new test in a partly checked group isn't");
    CheckEquals(2, restore.NewCount, ASWBCB_HERE, "two new tests");
    CheckEquals(1, restore.NewCheckedCount, ASWBCB_HERE, "one of them checked");
    CheckEquals(2, restore.MissingCount, ASWBCB_HERE, "Beta's two saved tests are gone");
    CheckEquals(2, restore.CheckedCount, ASWBCB_HERE, "Alpha's two tests checked");
    CheckFalse(restore.CheckedAll, ASWBCB_HERE, "no fallback");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_RestoreGUISelection_DoesNothingWithoutASavedSelection()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    list.SetChecked(0, false);

    // Act
    TGUISelectionRestore const restore = RestoreGUISelection(list, std::vector<TGUISavedTest>());

    // Assert
    CheckFalse(list.IsChecked(0), ASWBCB_HERE, "the check states are left as they were");
    CheckEquals(3, list.CheckedCount(), ASWBCB_HERE, "the others too");
    CheckEquals(0, restore.CheckedCount + restore.NewCount + restore.MissingCount, ASWBCB_HERE, "nothing to report");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_RestoreGUISelection_KeepsASavedSelectionWithNothingChecked()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    std::vector<TGUISavedTest> const saved = TSaved()(false, "Alpha", "One")(false, "Beta", "Three").Tests;

    // Act
    TGUISelectionRestore const restore = RestoreGUISelection(list, saved);

    // Assert
    CheckFalse(restore.CheckedAll, ASWBCB_HERE, "no fallback, since nothing was checked when saved");
    CheckEquals(0, list.CheckedCount(), ASWBCB_HERE, "nothing is checked");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Selection::Test_RestoreGUISelection_RestoresSavedCheckStates()
{
    // Arrange: saved out of order, with "Alpha.Two" saved twice; the last one counts.
    TGUITestList list;
    list.Load(SampleTests());
    std::vector<TGUISavedTest> const saved = TSaved()(false, "Gamma", "Four")(true, "Alpha", "Two")(false, "Alpha",
        "One")(true, "Beta", "Three")(false, "Alpha", "Two").Tests;

    // Act
    TGUISelectionRestore const restore = RestoreGUISelection(list, saved);

    // Assert
    CheckFalse(list.IsChecked(0), ASWBCB_HERE, "Alpha.One unchecked");
    CheckFalse(list.IsChecked(1), ASWBCB_HERE, "Alpha.Two unchecked, its last saved state");
    CheckTrue(list.IsChecked(2), ASWBCB_HERE, "Beta.Three checked");
    CheckFalse(list.IsChecked(3), ASWBCB_HERE, "Gamma.Four unchecked");
    CheckEquals(1, restore.CheckedCount, ASWBCB_HERE, "one checked");
    CheckEquals(0, restore.NewCount + restore.MissingCount, ASWBCB_HERE, "nothing new or missing");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_GUI_Selection, "SelfTests.GUI")
