/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_TestList.cpp
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
#include "Test_ASWUnitTestsBCB_GUI_TestList.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_TestList.h"
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
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

std::string Joined(std::vector<std::size_t> const& indexes);
std::string Joined(std::vector<std::string> const& names);
ASWUnitTestsBCB::TTestCaseRecord Record(std::string const& groupName, std::string const& testName,
    double durationSeconds, ASWUnitTestsBCB::TTestOutcome::Type outcome, std::string const& message);
std::vector<ASWUnitTestsBCB::TTestId> SampleTests();
std::vector<ASWUnitTestsBCB::TTestId> SuiteTests();

//---------------------------------------------------------------------------
// e.g. "0;1;"
std::string Joined(std::vector<std::size_t> const& indexes)
{
    std::string text;

    for (std::size_t i = 0; i < indexes.size(); ++i)
        text += ASWUnitTestsBCB::UIntToString(static_cast<unsigned int>(indexes[i])) + ";";

    return text;
}
//---------------------------------------------------------------------------
// e.g. "Alpha;Beta;"
std::string Joined(std::vector<std::string> const& names)
{
    std::string text;

    for (std::size_t i = 0; i < names.size(); ++i)
        text += names[i] + ";";

    return text;
}
//---------------------------------------------------------------------------
ASWUnitTestsBCB::TTestCaseRecord Record(std::string const& groupName, std::string const& testName,
    double durationSeconds, ASWUnitTestsBCB::TTestOutcome::Type outcome, std::string const& message)
{
    ASWUnitTestsBCB::TTestCaseRecord record;
    record.GroupName = groupName;
    record.TestName = testName;
    record.DurationSeconds = durationSeconds;
    record.Outcome = outcome;
    record.Message = message;
    return record;
}
//---------------------------------------------------------------------------
// Four tests in three groups, one with a '.' in its name: indexes 0 and 1 in "Alpha", 2 in "Beta", 3 in "Alpha.Sub".
std::vector<ASWUnitTestsBCB::TTestId> SampleTests()
{
    std::vector<ASWUnitTestsBCB::TTestId> tests;
    tests.push_back(ASWUnitTestsBCB::TTestId("Alpha", "One"));
    tests.push_back(ASWUnitTestsBCB::TTestId("Alpha", "Two"));
    tests.push_back(ASWUnitTestsBCB::TTestId("Beta", "Three"));
    tests.push_back(ASWUnitTestsBCB::TTestId("Alpha.Sub", "Four"));
    return tests;
}
//---------------------------------------------------------------------------
// Six tests, with groups in suites: index 0 in top-level group "Top"; 1 and 2 in group "Deep" in suite
// "Outer.Inner"; 3 in group "Mid" in suite "Outer"; 4 in group "Solo" in suite "Other"; and 5 in group "Near" in
// suite "Outermost", whose path starts with "Outer" but isn't inside it.
std::vector<ASWUnitTestsBCB::TTestId> SuiteTests()
{
    std::vector<ASWUnitTestsBCB::TTestId> tests;
    tests.push_back(ASWUnitTestsBCB::TTestId("Top", "One", ""));
    tests.push_back(ASWUnitTestsBCB::TTestId("Outer.Inner.Deep", "Two", "Outer.Inner"));
    tests.push_back(ASWUnitTestsBCB::TTestId("Outer.Inner.Deep", "Three", "Outer.Inner"));
    tests.push_back(ASWUnitTestsBCB::TTestId("Outer.Mid", "Four", "Outer"));
    tests.push_back(ASWUnitTestsBCB::TTestId("Other.Solo", "Five", "Other"));
    tests.push_back(ASWUnitTestsBCB::TTestId("Outermost.Near", "Six", "Outermost"));
    return tests;
}
//---------------------------------------------------------------------------

} // namespace


namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_TestList
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_GUI_TestList::TTest_ASWUnitTestsBCB_GUI_TestList()
    : inherited("ASWUnitTestsBCB_GUI_TestList_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_CheckedFilter_MatchesExactlyTheCheckedTests()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    list.SetChecked(1, false);
    list.SetChecked(2, false);

    // Act
    TGUITestNamesFilter const filter = list.CheckedFilter();

    // Assert
    CheckTrue(filter.Matches("Alpha.One"), ASWBCB_HERE, "a checked test matches");
    CheckFalse(filter.Matches("Alpha.Two"), ASWBCB_HERE, "an unchecked test doesn't");
    CheckFalse(filter.Matches("Beta.Three"), ASWBCB_HERE, "nor does another group's unchecked test");
    CheckTrue(filter.Matches("Alpha.Sub.Four"), ASWBCB_HERE, "a group name containing '.' still matches exactly");
    CheckFalse(filter.Matches("Alpha.On"), ASWBCB_HERE, "matching is exact, not by prefix");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_CheckMatching_ChecksExactlyTheMatches()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    TPatternTestFilter const alpha("Alpha.*", false);

    // Act
    list.CheckMatching(&alpha);
    std::size_t const checkedByFilter = list.CheckedCount();
    bool const betaCheckedByFilter = list.IsChecked(2);
    list.CheckMatching(nullptr);

    // Assert
    CheckEquals(3, checkedByFilter, ASWBCB_HERE,
        "a filter checks exactly its matches (here, both Alpha tests and Alpha.Sub's)");
    CheckFalse(betaCheckedByFilter, ASWBCB_HERE, "and unchecks the rest");
    CheckEquals(4, list.CheckedCount(), ASWBCB_HERE, "no filter checks everything");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_CheckOnlyFailed_ChecksVisibleFailuresOnly()
{
    // Arrange: "Beta.Three" failed but is unchecked and hidden, which must leave it as it is.
    TGUITestList list;
    list.Load(SampleTests());
    list.SetStatus(1, TGUITestStatus::Failed);
    list.SetStatus(2, TGUITestStatus::Failed);
    list.SetStatus(3, TGUITestStatus::Skipped);
    list.SetChecked(2, false);
    TAllButFilter const allButBeta("Beta.Three");
    list.SetVisibleFilter(&allButBeta);

    // Act
    list.CheckOnlyFailed();

    // Assert
    CheckFalse(list.IsChecked(0), ASWBCB_HERE, "a visible test that didn't fail is unchecked");
    CheckTrue(list.IsChecked(1), ASWBCB_HERE, "a visible failed test is checked");
    CheckFalse(list.IsChecked(2), ASWBCB_HERE, "a hidden failed test is left alone, like Select All does");
    CheckFalse(list.IsChecked(3), ASWBCB_HERE, "a skipped test isn't a failure");
    CheckEquals(1, list.CheckedCount(), ASWBCB_HERE, "only the visible failure");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_CountMatching_CountsFilterMatches()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    TPatternTestFilter const alpha("Alpha.*", false);

    // Act
    std::size_t const alphaTests = list.CountMatching(&alpha);
    std::size_t const allTests = list.CountMatching(nullptr);

    // Assert
    CheckEquals(3, alphaTests, ASWBCB_HERE, "a filter's matches are counted");
    CheckEquals(4, allTests, ASWBCB_HERE, "no filter matches everything");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_DetailText_DescribesAFinishedTest()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    list.SetResult(1, Record("Alpha", "Two", 0.000412, TTestOutcome::Fail, "Check failed for: needle"),
        "Running test: Alpha.Two\n");

    // Act
    std::string const text = list.DetailText(1);

    // Assert
    CheckEquals("Alpha.Two\nResult: Failed (0.412 ms)\n\nCheck failed for: needle\n\nLog:\nRunning test: Alpha.Two\n",
        text, ASWBCB_HERE, "the full name, status and duration, failure detail, and the test's own log");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_DetailText_DescribesATestThatHasNotRun()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());

    // Act
    std::string const text = list.DetailText(3);

    // Assert
    CheckEquals("Alpha.Sub.Four\nResult: Not run\n", text, ASWBCB_HERE,
        "just the full name and status, with no duration, detail, or log");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_FailedFilter_MatchesEveryFailedTestEvenHiddenOnes()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    list.SetStatus(1, TGUITestStatus::Failed);
    list.SetStatus(2, TGUITestStatus::Failed);
    list.SetStatus(3, TGUITestStatus::Skipped);
    TAllButFilter const allButBeta("Beta.Three");
    list.SetVisibleFilter(&allButBeta);

    // Act
    TGUITestNamesFilter const filter = list.FailedFilter();

    // Assert
    CheckTrue(filter.Matches("Alpha.Two"), ASWBCB_HERE, "a visible failed test matches");
    CheckTrue(filter.Matches("Beta.Three"), ASWBCB_HERE, "so does a hidden one, since Run Failed reruns them all");
    CheckFalse(filter.Matches("Alpha.One"), ASWBCB_HERE, "a test that didn't fail doesn't");
    CheckFalse(filter.Matches("Alpha.Sub.Four"), ASWBCB_HERE, "nor does a skipped one");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_GroupDetailText_CountsEachStatus()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    list.SetStatus(0, TGUITestStatus::Passed);
    list.SetStatus(1, TGUITestStatus::Running);
    list.SetStatus(2, TGUITestStatus::Failed);

    // Act
    std::string const alpha = list.GroupDetailText("Alpha");
    std::string const beta = list.GroupDetailText("Beta");

    // Assert
    CheckEquals("Alpha\n2 tests: 1 passed, 0 failed, 0 skipped, 0 not run, 1 running\n", alpha, ASWBCB_HERE,
        "the group's name, then how many of its tests have each status");
    CheckEquals("Beta\n1 test: 0 passed, 1 failed, 0 skipped, 0 not run\n", beta, ASWBCB_HERE,
        "one test, and none running");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_GroupStatus_ShowsRunningThenWorstOutcome()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    TGUITestStatus::Type const beforeRun = list.GroupStatus("Alpha");

    // Act, and Assert as the group's tests progress
    list.SetStatus(0, TGUITestStatus::Passed);
    CheckEquals(TGUITestStatus::Passed, list.GroupStatus("Alpha"), ASWBCB_HERE,
        "one test passed, the other not run: passed");

    list.SetStatus(1, TGUITestStatus::Running);
    CheckEquals(TGUITestStatus::Running, list.GroupStatus("Alpha"), ASWBCB_HERE, "running while any test is");

    list.SetStatus(1, TGUITestStatus::Skipped);
    CheckEquals(TGUITestStatus::Skipped, list.GroupStatus("Alpha"), ASWBCB_HERE, "a skip outranks a pass");

    list.SetStatus(0, TGUITestStatus::Failed);
    CheckEquals(TGUITestStatus::Failed, list.GroupStatus("Alpha"), ASWBCB_HERE, "a failure outranks a skip");

    list.SetStatus(1, TGUITestStatus::Running);
    CheckEquals(TGUITestStatus::Running, list.GroupStatus("Alpha"), ASWBCB_HERE, "running outranks even a failure");

    CheckEquals(TGUITestStatus::NotRun, beforeRun, ASWBCB_HERE, "nothing run yet: not run");
    CheckEquals(TGUITestStatus::NotRun, list.GroupStatus("Alpha.Sub"), ASWBCB_HERE,
        "another group, even one whose name starts with this one's, is unaffected");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_GroupSuitePath_GivesEachGroupsSuite()
{
    // Arrange
    TGUITestList list;
    list.Load(SuiteTests());

    // Act and assert
    CheckEquals("Outer.Inner", list.GroupSuitePath("Outer.Inner.Deep"), ASWBCB_HERE,
        "a group in a nested suite gives the whole suite path");
    CheckEquals("Outer", list.GroupSuitePath("Outer.Mid"), ASWBCB_HERE, "a group in a top-level suite gives that suite");
    CheckEquals("", list.GroupSuitePath("Top"), ASWBCB_HERE, "a group in no suite gives an empty path");
    CheckEquals("", list.GroupSuitePath("Missing"), ASWBCB_HERE, "so does a group that isn't in the list");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_IndexOf_FindsTestsByGroupAndTestName()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());

    // Act
    TOptional<std::size_t> const found = list.IndexOf("Alpha.Sub", "Four");
    TOptional<std::size_t> const wrongGroup = list.IndexOf("Alpha", "Four");
    TOptional<std::size_t> const missing = list.IndexOf("Gamma", "One");

    // Assert
    CheckTrue(found.HasValue(), ASWBCB_HERE, "a test is found by its group and test names");
    CheckEquals(3, found.Value(), ASWBCB_HERE);
    CheckFalse(wrongGroup.HasValue(), ASWBCB_HERE, "a test name under the wrong group isn't found");
    CheckFalse(missing.HasValue(), ASWBCB_HERE, "nor is an unknown group");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_Load_ChecksEveryTestAndListsGroupsInOrder()
{
    // Arrange
    TGUITestList list;

    // Act
    list.Load(SampleTests());

    // Assert
    CheckEquals(4, list.Count(), ASWBCB_HERE, "every test is listed");
    CheckEquals(4, list.CheckedCount(), ASWBCB_HERE, "and checked");
    CheckEquals("Alpha;Beta;Alpha.Sub;", Joined(list.GroupNames()), ASWBCB_HERE,
        "each group is listed once, in the order its first test appears");
    CheckEquals("0;1;", Joined(list.TestsInGroup("Alpha")), ASWBCB_HERE,
        "a group's tests are listed by index, in order");
    CheckEquals("Three", list.Test(2).TestName, ASWBCB_HERE, "tests keep their order");
    CheckEquals(4, list.StatusCount(TGUITestStatus::NotRun), ASWBCB_HERE, "none has run yet");
    CheckEquals("", Joined(list.SuitePaths()), ASWBCB_HERE, "groups in no suite list no suites");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_Load_ListsEachSuiteAfterTheSuitesHoldingIt()
{
    // Arrange
    TGUITestList list;

    // Act
    list.Load(SuiteTests());

    // Assert
    CheckEquals("Outer;Outer.Inner;Other;Outermost;", Joined(list.SuitePaths()), ASWBCB_HERE,
        "each suite is listed once, after the suite holding it, in the order its first test appears, including "
        "\"Outer\", whose first test is in a suite inside it");

    TOptional<std::size_t> const inner = list.SuiteIndexOf("Outer.Inner");
    CheckTrue(inner.HasValue() && (inner.Value() == 1), ASWBCB_HERE, "a suite is found by its path");
    CheckFalse(list.SuiteIndexOf("Inner").HasValue(), ASWBCB_HERE, "but not by its own name alone");
    CheckFalse(list.SuiteIndexOf("Top").HasValue(), ASWBCB_HERE, "and a group isn't a suite");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_NameInSuite_DropsTheSuitePath()
{
    CheckEquals("Deep", NameInSuite("Outer.Inner.Deep", "Outer.Inner"), ASWBCB_HERE,
        "a group's own name, without its suite path");
    CheckEquals("Inner", NameInSuite("Outer.Inner", "Outer"), ASWBCB_HERE,
        "a suite's own name, without the path of the suite holding it");
    CheckEquals("Alpha.Sub", NameInSuite("Alpha.Sub", ""), ASWBCB_HERE,
        "the whole name when it isn't in a suite, even one containing '.'");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_ParentSuitePath_DropsTheLastSuiteName()
{
    CheckEquals("Outer.Inner", ParentSuitePath("Outer.Inner.Deep"), ASWBCB_HERE,
        "a nested suite's parent is the path before its last '.'");
    CheckEquals("Outer", ParentSuitePath("Outer.Inner"), ASWBCB_HERE, "likewise one level up");
    CheckEquals("", ParentSuitePath("Outer"), ASWBCB_HERE, "a top-level suite has no parent");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_ResetResults_SetsEveryTestBackToNotRun()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    list.SetStatus(0, TGUITestStatus::Passed);
    list.SetResult(2, Record("Beta", "Three", 0.001, TTestOutcome::Fail, "boom"), "log text");

    // Act
    list.ResetResults();

    // Assert
    CheckEquals(4, list.StatusCount(TGUITestStatus::NotRun), ASWBCB_HERE, "every test is back to not run");
    CheckNull(list.Result(2), ASWBCB_HERE, "and no longer has a result");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_SetAllChecked_ChecksOrUnchecksEveryTest()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());

    // Act
    list.SetAllChecked(false);
    std::size_t const checkedAfterNone = list.CheckedCount();
    list.SetAllChecked(true);

    // Assert
    CheckEquals(0, checkedAfterNone, ASWBCB_HERE, "none checked");
    CheckEquals(4, list.CheckedCount(), ASWBCB_HERE, "all checked again");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_SetChecked_UpdatesItsGroupsCheckState()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());

    // Act
    list.SetChecked(0, false);
    TGUICheckState::Type const oneUnchecked = list.GroupCheckState("Alpha");
    list.SetChecked(1, false);
    TGUICheckState::Type const bothUnchecked = list.GroupCheckState("Alpha");

    // Assert
    CheckEquals(TGUICheckState::Partial, oneUnchecked, ASWBCB_HERE, "some of a group's tests checked: partial");
    CheckEquals(TGUICheckState::Unchecked, bothUnchecked, ASWBCB_HERE, "none checked: unchecked");
    CheckEquals(TGUICheckState::Checked, list.GroupCheckState("Beta"), ASWBCB_HERE, "another group is unaffected");
    CheckEquals(TGUICheckState::Checked, list.GroupCheckState("Alpha.Sub"), ASWBCB_HERE,
        "including one whose name starts with the changed group's");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_SetGroupChecked_ChangesOnlyThatGroup()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());

    // Act
    list.SetGroupChecked("Alpha", false);

    // Assert
    CheckFalse(list.IsChecked(0), ASWBCB_HERE, "the group's first test is unchecked");
    CheckFalse(list.IsChecked(1), ASWBCB_HERE, "and its second");
    CheckTrue(list.IsChecked(2), ASWBCB_HERE, "another group's test isn't");
    CheckTrue(list.IsChecked(3), ASWBCB_HERE, "nor is one in a group whose name starts with this one's");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_SetResult_StoresTheResultAndSetsTheStatus()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    TGUITestResult const* const before = list.Result(2);

    // Act
    list.SetResult(2, Record("Beta", "Three", 0.25, TTestOutcome::Skip, "not on this platform"), "log");

    // Assert
    CheckNull(before, ASWBCB_HERE, "a test has no result before it finishes");
    TGUITestResult const* const after = list.Result(2);
    AssertNotNull(after, ASWBCB_HERE, "it has one after");
    CheckEquals("not on this platform", after->Record.Message, ASWBCB_HERE, "with its record");
    CheckEquals("log", after->Log, ASWBCB_HERE, "and its log");
    CheckEquals(TGUITestStatus::Skipped, list.Status(2), ASWBCB_HERE, "its status follows the outcome");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_SetSuiteChecked_ChangesTheSuiteAndTheSuitesInIt()
{
    // Arrange
    TGUITestList list;
    list.Load(SuiteTests());

    // Act
    list.SetSuiteChecked("Outer", false);

    // Assert
    CheckFalse(list.IsChecked(1) || list.IsChecked(2) || list.IsChecked(3), ASWBCB_HERE,
        "the suite's own group and the groups in the suite inside it are unchecked");
    CheckTrue(list.IsChecked(0) && list.IsChecked(4) && list.IsChecked(5), ASWBCB_HERE,
        "tests outside it, including a suite whose path only starts with its path, are unchanged");
    CheckEquals(TGUICheckState::Unchecked, list.SuiteCheckState("Outer"), ASWBCB_HERE, "the suite shows unchecked");
    CheckEquals(TGUICheckState::Unchecked, list.SuiteCheckState("Outer.Inner"), ASWBCB_HERE,
        "and so does the suite inside it");
    CheckEquals(TGUICheckState::Checked, list.SuiteCheckState("Outermost"), ASWBCB_HERE,
        "while a suite with a similar path stays checked");

    // Act and assert: one test inside the nested suite checked again
    list.SetChecked(1, true);
    CheckEquals(TGUICheckState::Partial, list.SuiteCheckState("Outer"), ASWBCB_HERE, "the outer suite is partly checked");
    CheckEquals(TGUICheckState::Partial, list.SuiteCheckState("Outer.Inner"), ASWBCB_HERE,
        "and so is the suite holding that test");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_SetVisibleFilter_HidesNonMatchingTests()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    TAllButFilter const allButTwo("Alpha.Two");

    // Act
    list.SetVisibleFilter(&allButTwo);
    bool const twoVisibleWhileFiltered = list.IsVisible(1);
    std::vector<std::size_t> const alphaWhileFiltered = list.TestsInGroup("Alpha");
    std::size_t const visibleWhileFiltered = list.VisibleCount();
    list.SetVisibleFilter(nullptr);

    // Assert
    CheckFalse(twoVisibleWhileFiltered, ASWBCB_HERE, "a test the filter doesn't match is hidden");
    CheckEquals("0;", Joined(alphaWhileFiltered), ASWBCB_HERE, "and left out of its group's tests");
    CheckEquals(3, visibleWhileFiltered, ASWBCB_HERE, "and the visible count");
    CheckEquals(4, list.VisibleCount(), ASWBCB_HERE, "no filter shows everything");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_SetVisibleFilter_LimitsCheckingToVisibleTests()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());
    TAllButFilter const allButTwo("Alpha.Two");
    list.SetVisibleFilter(&allButTwo);

    // Act
    list.SetAllChecked(false);
    bool const hiddenStillChecked = list.IsChecked(1);
    TGUITestNamesFilter const noneChecked = list.CheckedFilter();
    list.SetGroupChecked("Alpha", true);

    // Assert
    CheckTrue(hiddenStillChecked, ASWBCB_HERE, "unchecking everything leaves a hidden test alone");
    CheckFalse(noneChecked.Matches("Alpha.Two"), ASWBCB_HERE, "a hidden checked test isn't run");
    CheckEquals(TGUICheckState::Checked, list.GroupCheckState("Alpha"), ASWBCB_HERE,
        "a group's check state only counts its visible tests");
    CheckEquals(1, list.CheckedCount(), ASWBCB_HERE, "nor does the checked count");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_StatusCount_CountsTestsWithEachStatus()
{
    // Arrange
    TGUITestList list;
    list.Load(SampleTests());

    // Act
    list.SetStatus(0, TGUITestStatus::Passed);
    list.SetStatus(1, TGUITestStatus::Passed);
    list.SetStatus(2, TGUITestStatus::Failed);

    // Assert
    CheckEquals(2, list.StatusCount(TGUITestStatus::Passed), ASWBCB_HERE, "two passed");
    CheckEquals(1, list.StatusCount(TGUITestStatus::Failed), ASWBCB_HERE, "one failed");
    CheckEquals(0, list.StatusCount(TGUITestStatus::Skipped), ASWBCB_HERE, "none skipped");
    CheckEquals(1, list.StatusCount(TGUITestStatus::NotRun), ASWBCB_HERE, "one not run");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_StatusFromOutcome_MapsEachOutcome()
{
    CheckEquals(TGUITestStatus::Passed, StatusFromOutcome(TTestOutcome::Pass), ASWBCB_HERE, "Pass: Passed");
    CheckEquals(TGUITestStatus::Failed, StatusFromOutcome(TTestOutcome::Fail), ASWBCB_HERE, "Fail: Failed");
    CheckEquals(TGUITestStatus::Skipped, StatusFromOutcome(TTestOutcome::Skip), ASWBCB_HERE, "Skip: Skipped");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_StatusName_NamesEachStatus()
{
    CheckEquals("Not run", StatusName(TGUITestStatus::NotRun), ASWBCB_HERE, "NotRun");
    CheckEquals("Running", StatusName(TGUITestStatus::Running), ASWBCB_HERE, "Running");
    CheckEquals("Passed", StatusName(TGUITestStatus::Passed), ASWBCB_HERE, "Passed");
    CheckEquals("Failed", StatusName(TGUITestStatus::Failed), ASWBCB_HERE, "Failed");
    CheckEquals("Skipped", StatusName(TGUITestStatus::Skipped), ASWBCB_HERE, "Skipped");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_SuiteDetailText_CountsEachStatus()
{
    // Arrange
    TGUITestList list;
    list.Load(SuiteTests());
    list.SetStatus(1, TGUITestStatus::Failed);
    list.SetStatus(3, TGUITestStatus::Passed);

    // Act
    std::string const text = list.SuiteDetailText("Outer");

    // Assert
    CheckEquals("Outer\n3 tests: 1 passed, 1 failed, 0 skipped, 1 not run\n", text, ASWBCB_HERE,
        "the suite's path, then counts over its tests, including those in the suite inside it");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_SuiteStatus_IncludesTheSuitesInIt()
{
    // Arrange
    TGUITestList list;
    list.Load(SuiteTests());
    TGUITestStatus::Type const beforeRun = list.SuiteStatus("Outer");

    // Act, and assert as the suite's tests progress
    list.SetStatus(3, TGUITestStatus::Passed);
    CheckEquals(TGUITestStatus::Passed, list.SuiteStatus("Outer"), ASWBCB_HERE,
        "its own group's test passed, the others not run: passed");

    list.SetStatus(1, TGUITestStatus::Failed);
    CheckEquals(TGUITestStatus::Failed, list.SuiteStatus("Outer"), ASWBCB_HERE,
        "a failure in the suite inside it fails it too");
    CheckEquals(TGUITestStatus::Failed, list.SuiteStatus("Outer.Inner"), ASWBCB_HERE,
        "as it does the suite holding the failed test");

    list.SetStatus(2, TGUITestStatus::Running);
    CheckEquals(TGUITestStatus::Running, list.SuiteStatus("Outer"), ASWBCB_HERE, "running while any test in it is");

    CheckEquals(TGUITestStatus::NotRun, beforeRun, ASWBCB_HERE, "nothing run yet: not run");
    CheckEquals(TGUITestStatus::NotRun, list.SuiteStatus("Outermost"), ASWBCB_HERE,
        "a suite whose path only starts with this one's is unaffected");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_TestList::Test_TestsInSuite_IncludesNestedSuitesOnly()
{
    // Arrange
    TGUITestList list;
    list.Load(SuiteTests());

    // Act and assert
    CheckEquals("1;2;3;", Joined(list.TestsInSuite("Outer")), ASWBCB_HERE,
        "a suite's tests include those in the suite inside it, in order");
    CheckEquals("1;2;", Joined(list.TestsInSuite("Outer.Inner")), ASWBCB_HERE, "a nested suite's tests are only its own");
    CheckEquals("5;", Joined(list.TestsInSuite("Outermost")), ASWBCB_HERE,
        "a suite whose path starts with another's isn't inside it");
    CheckEquals("", Joined(list.TestsInSuite("Out")), ASWBCB_HERE, "a partial suite name matches nothing");

    TAllButFilter const filter("Outer.Inner.Deep.Two");
    list.SetVisibleFilter(&filter);
    CheckEquals("2;3;", Joined(list.TestsInSuite("Outer")), ASWBCB_HERE, "only visible tests are included");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_GUI_TestList)
