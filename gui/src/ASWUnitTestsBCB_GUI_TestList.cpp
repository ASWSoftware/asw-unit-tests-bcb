/* **************************************************************************
ASWUnitTestsBCB_GUI_TestList.cpp
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
#include "ASWUnitTestsBCB_GUI_TestList.h"
//---------------------------------------------------------------------------
#include <algorithm>
#include <sstream>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------
using namespace System;
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

//---------------------------------------------------------------------------
std::string NameInSuite(std::string const& qualifiedName, std::string const& suitePath)
{
    if (suitePath.empty())
        return qualifiedName;

    return qualifiedName.substr(std::min(suitePath.size() + 1, qualifiedName.size()));
}
//---------------------------------------------------------------------------
std::string ParentSuitePath(std::string const& suitePath)
{
    // A suite name can't contain '.' (see TSuitePath), so the last one ends the parent's path.
    std::string::size_type const dot = suitePath.rfind('.');
    return (dot == std::string::npos) ? std::string() : suitePath.substr(0, dot);
}
//---------------------------------------------------------------------------
TGUITestStatus StatusFromOutcome(TTestOutcome outcome)
{
    switch (outcome)
    {
        case TTestOutcome::Pass:
            return TGUITestStatus::Passed;

        case TTestOutcome::Fail:
            return TGUITestStatus::Failed;

        case TTestOutcome::Skip:
            return TGUITestStatus::Skipped;
    }

    return TGUITestStatus::Failed;
}
//---------------------------------------------------------------------------
std::string StatusName(TGUITestStatus status)
{
    switch (status)
    {
        case TGUITestStatus::NotRun:
            return "Not run";

        case TGUITestStatus::Running:
            return "Running";

        case TGUITestStatus::Passed:
            return "Passed";

        case TGUITestStatus::Failed:
            return "Failed";

        case TGUITestStatus::Skipped:
            return "Skipped";
    }

    return "Unknown";
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TGUITestResult
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TGUITestResult::TGUITestResult()
{
}
//---------------------------------------------------------------------------
TGUITestResult::TGUITestResult(TTestCaseRecord const& record, std::string const& log)
    : Record(record),
      Log(log)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TGUITestNamesFilter
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
void TGUITestNamesFilter::Add(std::string const& fullTestName)
{
    m_FullTestNames.insert(fullTestName);
}
//---------------------------------------------------------------------------
bool TGUITestNamesFilter::Matches(std::string const& fullTestName) const
{
    return m_FullTestNames.find(fullTestName) != m_FullTestNames.end();
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TGUITestList
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
std::size_t TGUITestList::CheckedCount() const
{
    std::size_t count = 0;

    for (std::size_t i = 0; i < m_Tests.size(); ++i)
    {
        if (m_Visible[i] && m_Checked[i])
            ++count;
    }

    return count;
}
//---------------------------------------------------------------------------
TGUITestNamesFilter TGUITestList::CheckedFilter() const
{
    TGUITestNamesFilter filter;

    for (std::size_t i = 0; i < m_Tests.size(); ++i)
    {
        if (m_Visible[i] && m_Checked[i])
            filter.Add(FullName(i));
    }

    return filter;
}
//---------------------------------------------------------------------------
void TGUITestList::CheckMatching(ITestFilter const* filter)
{
    for (std::size_t i = 0; i < m_Tests.size(); ++i)
        m_Checked[i] = (filter == nullptr) || filter->Matches(FullName(i));
}
//---------------------------------------------------------------------------
void TGUITestList::CheckOnlyFailed()
{
    for (std::size_t i = 0; i < m_Tests.size(); ++i)
    {
        if (m_Visible[i])
            m_Checked[i] = (m_Statuses[i] == TGUITestStatus::Failed);
    }
}
//---------------------------------------------------------------------------
TGUICheckState TGUITestList::CheckStateOf(std::vector<std::size_t> const& indexes) const
{
    std::size_t checked = 0;

    for (std::size_t i = 0; i < indexes.size(); ++i)
    {
        if (m_Checked[indexes[i]])
            ++checked;
    }

    if (checked == 0)
        return TGUICheckState::Unchecked;

    return (checked == indexes.size()) ? TGUICheckState::Checked : TGUICheckState::Partial;
}
//---------------------------------------------------------------------------
std::size_t TGUITestList::Count() const
{
    return m_Tests.size();
}
//---------------------------------------------------------------------------
std::size_t TGUITestList::CountMatching(ITestFilter const* filter) const
{
    if (filter == nullptr)
        return m_Tests.size();

    std::size_t count = 0;

    for (std::size_t i = 0; i < m_Tests.size(); ++i)
    {
        if (filter->Matches(FullName(i)))
            ++count;
    }

    return count;
}
//---------------------------------------------------------------------------
std::string TGUITestList::DetailText(std::size_t index) const
{
    TTestId const& test = m_Tests.at(index);
    std::string text = test.GroupName + "." + test.TestName + "\n" + "Result: " + StatusName(m_Statuses[index]);

    TGUITestResult const* const result = Result(index);
    if (result == nullptr)
        return text + "\n";

    text += " (" + FormatDurationMs(result->Record.DurationSeconds * 1000.0) + ")\n";

    if (!result->Record.Message.empty())
        text += "\n" + result->Record.Message + "\n";

    if (!result->Log.empty())
        text += "\nLog:\n" + result->Log;

    return text;
}
//---------------------------------------------------------------------------
std::string TGUITestList::DetailTextOf(std::string const& name, std::vector<std::size_t> const& indexes) const
{
    unsigned int counts[static_cast<std::size_t>(TGUITestStatus::Skipped) + 1] = { 0 }; // By TGUITestStatus

    for (std::size_t i = 0; i < indexes.size(); ++i)
        ++counts[static_cast<std::size_t>(m_Statuses[indexes[i]])];

    std::ostringstream text;
    text << name << "\n";
    text << indexes.size() << ((indexes.size() == 1) ? " test: " : " tests: ");
    text << counts[static_cast<std::size_t>(TGUITestStatus::Passed)] << " passed, ";
    text << counts[static_cast<std::size_t>(TGUITestStatus::Failed)] << " failed, ";
    text << counts[static_cast<std::size_t>(TGUITestStatus::Skipped)] << " skipped, ";
    text << counts[static_cast<std::size_t>(TGUITestStatus::NotRun)] << " not run";

    if (counts[static_cast<std::size_t>(TGUITestStatus::Running)] != 0)
        text << ", " << counts[static_cast<std::size_t>(TGUITestStatus::Running)] << " running";

    text << "\n";
    return text.str();
}
//---------------------------------------------------------------------------
TGUITestNamesFilter TGUITestList::FailedFilter() const
{
    TGUITestNamesFilter filter;

    for (std::size_t i = 0; i < m_Tests.size(); ++i)
    {
        if (m_Statuses[i] == TGUITestStatus::Failed)
            filter.Add(FullName(i));
    }

    return filter;
}
//---------------------------------------------------------------------------
// The test's "GroupName.TestName", as filters take it.
std::string TGUITestList::FullName(std::size_t index) const
{
    return m_Tests[index].GroupName + "." + m_Tests[index].TestName;
}
//---------------------------------------------------------------------------
TGUICheckState TGUITestList::GroupCheckState(std::string const& groupName) const
{
    return CheckStateOf(TestsInGroup(groupName));
}
//---------------------------------------------------------------------------
std::string TGUITestList::GroupDetailText(std::string const& groupName) const
{
    return DetailTextOf(groupName, TestsInGroup(groupName));
}
//---------------------------------------------------------------------------
std::vector<std::string> const& TGUITestList::GroupNames() const
{
    return m_GroupNames;
}
//---------------------------------------------------------------------------
TGUITestStatus TGUITestList::GroupStatus(std::string const& groupName) const
{
    return StatusOf(TestsInGroup(groupName));
}
//---------------------------------------------------------------------------
std::string TGUITestList::GroupSuitePath(std::string const& groupName) const
{
    std::map<std::string, std::string>::const_iterator const it = m_GroupSuitePaths.find(groupName);
    return (it == m_GroupSuitePaths.end()) ? std::string() : it->second;
}
//---------------------------------------------------------------------------
TOptional<std::size_t> TGUITestList::IndexOf(std::string const& groupName, std::string const& testName) const
{
    std::map<std::pair<std::string, std::string>, std::size_t>::const_iterator const it =
        m_IndexByName.find(std::make_pair(groupName, testName));

    if (it == m_IndexByName.end())
        return TOptional<std::size_t>();

    return it->second;
}
//---------------------------------------------------------------------------
bool TGUITestList::IsChecked(std::size_t index) const
{
    return m_Checked.at(index);
}
//---------------------------------------------------------------------------
bool TGUITestList::IsVisible(std::size_t index) const
{
    return m_Visible.at(index);
}
//---------------------------------------------------------------------------
void TGUITestList::Load(std::vector<TTestId> const& tests)
{
    m_Tests = tests;
    m_Checked.assign(m_Tests.size(), true);
    m_Visible.assign(m_Tests.size(), true);
    m_Statuses.assign(m_Tests.size(), TGUITestStatus::NotRun);
    m_Results.assign(m_Tests.size(), TOptional<TGUITestResult>());
    m_GroupNames.clear();
    m_GroupSuitePaths.clear();
    m_IndexByName.clear();
    m_SuiteIndexByPath.clear();
    m_SuitePaths.clear();

    for (std::size_t i = 0; i < m_Tests.size(); ++i)
    {
        TTestId const& test = m_Tests[i];

        if (std::find(m_GroupNames.begin(), m_GroupNames.end(), test.GroupName) == m_GroupNames.end())
        {
            m_GroupNames.push_back(test.GroupName);
            m_GroupSuitePaths[test.GroupName] = test.SuitePath;
        }

        m_IndexByName[std::make_pair(test.GroupName, test.TestName)] = i;

        // The suite itself, after each suite holding it, outermost first.
        TSuitePath const suitePath(test.SuitePath);
        std::vector<std::string> const& suiteNames = suitePath.GetNames();
        std::string path;

        for (std::vector<std::string>::const_iterator it = suiteNames.begin(); it != suiteNames.end(); ++it)
        {
            path += (path.empty() ? std::string() : std::string(".")) + *it;

            if (m_SuiteIndexByPath.insert(std::make_pair(path, m_SuitePaths.size())).second)
                m_SuitePaths.push_back(path);
        }
    }
}
//---------------------------------------------------------------------------
void TGUITestList::ResetResults()
{
    m_Statuses.assign(m_Tests.size(), TGUITestStatus::NotRun);
    m_Results.assign(m_Tests.size(), TOptional<TGUITestResult>());
}
//---------------------------------------------------------------------------
TGUITestResult const* TGUITestList::Result(std::size_t index) const
{
    TOptional<TGUITestResult> const& result = m_Results.at(index);
    return result.HasValue() ? &result.Value() : nullptr;
}
//---------------------------------------------------------------------------
void TGUITestList::SetAllChecked(bool checked)
{
    for (std::size_t i = 0; i < m_Tests.size(); ++i)
    {
        if (m_Visible[i])
            m_Checked[i] = checked;
    }
}
//---------------------------------------------------------------------------
void TGUITestList::SetChecked(std::size_t index, bool checked)
{
    m_Checked.at(index) = checked;
}
//---------------------------------------------------------------------------
void TGUITestList::SetGroupChecked(std::string const& groupName, bool checked)
{
    std::vector<std::size_t> const indexes = TestsInGroup(groupName);

    for (std::size_t i = 0; i < indexes.size(); ++i)
        m_Checked[indexes[i]] = checked;
}
//---------------------------------------------------------------------------
void TGUITestList::SetResult(std::size_t index, TTestCaseRecord const& record, std::string const& log)
{
    m_Results.at(index) = TGUITestResult(record, log);
    m_Statuses[index] = StatusFromOutcome(record.Outcome);
}
//---------------------------------------------------------------------------
void TGUITestList::SetStatus(std::size_t index, TGUITestStatus status)
{
    m_Statuses.at(index) = status;
}
//---------------------------------------------------------------------------
void TGUITestList::SetSuiteChecked(std::string const& suitePath, bool checked)
{
    std::vector<std::size_t> const indexes = TestsInSuite(suitePath);

    for (std::size_t i = 0; i < indexes.size(); ++i)
        m_Checked[indexes[i]] = checked;
}
//---------------------------------------------------------------------------
void TGUITestList::SetVisibleFilter(ITestFilter const* filter)
{
    for (std::size_t i = 0; i < m_Tests.size(); ++i)
        m_Visible[i] = (filter == nullptr) || filter->Matches(FullName(i));
}
//---------------------------------------------------------------------------
TGUITestStatus TGUITestList::Status(std::size_t index) const
{
    return m_Statuses.at(index);
}
//---------------------------------------------------------------------------
std::size_t TGUITestList::StatusCount(TGUITestStatus status) const
{
    return static_cast<std::size_t>(std::count(m_Statuses.begin(), m_Statuses.end(), status));
}
//---------------------------------------------------------------------------
TGUITestStatus TGUITestList::StatusOf(std::vector<std::size_t> const& indexes) const
{
    bool anyRunning = false;
    bool anyFailed = false;
    bool anySkipped = false;
    bool anyPassed = false;

    for (std::size_t i = 0; i < indexes.size(); ++i)
    {
        switch (m_Statuses[indexes[i]])
        {
            case TGUITestStatus::Running:
                anyRunning = true;
                break;

            case TGUITestStatus::Failed:
                anyFailed = true;
                break;

            case TGUITestStatus::Skipped:
                anySkipped = true;
                break;

            case TGUITestStatus::Passed:
                anyPassed = true;
                break;

            case TGUITestStatus::NotRun:
                break;
        }
    }

    if (anyRunning)
        return TGUITestStatus::Running;

    if (anyFailed)
        return TGUITestStatus::Failed;

    if (anySkipped)
        return TGUITestStatus::Skipped;

    if (anyPassed)
        return TGUITestStatus::Passed;

    return TGUITestStatus::NotRun;
}
//---------------------------------------------------------------------------
TGUICheckState TGUITestList::SuiteCheckState(std::string const& suitePath) const
{
    return CheckStateOf(TestsInSuite(suitePath));
}
//---------------------------------------------------------------------------
std::string TGUITestList::SuiteDetailText(std::string const& suitePath) const
{
    return DetailTextOf(suitePath, TestsInSuite(suitePath));
}
//---------------------------------------------------------------------------
TOptional<std::size_t> TGUITestList::SuiteIndexOf(std::string const& suitePath) const
{
    std::map<std::string, std::size_t>::const_iterator const it = m_SuiteIndexByPath.find(suitePath);

    if (it == m_SuiteIndexByPath.end())
        return TOptional<std::size_t>();

    return it->second;
}
//---------------------------------------------------------------------------
std::vector<std::string> const& TGUITestList::SuitePaths() const
{
    return m_SuitePaths;
}
//---------------------------------------------------------------------------
TGUITestStatus TGUITestList::SuiteStatus(std::string const& suitePath) const
{
    return StatusOf(TestsInSuite(suitePath));
}
//---------------------------------------------------------------------------
TTestId const& TGUITestList::Test(std::size_t index) const
{
    return m_Tests.at(index);
}
//---------------------------------------------------------------------------
std::vector<std::size_t> TGUITestList::TestsInGroup(std::string const& groupName) const
{
    std::vector<std::size_t> indexes;

    for (std::size_t i = 0; i < m_Tests.size(); ++i)
    {
        if (m_Visible[i] && (m_Tests[i].GroupName == groupName))
            indexes.push_back(i);
    }

    return indexes;
}
//---------------------------------------------------------------------------
std::vector<std::size_t> TGUITestList::TestsInSuite(std::string const& suitePath) const
{
    std::vector<std::size_t> indexes;
    std::string const nestedPrefix = suitePath + ".";

    for (std::size_t i = 0; i < m_Tests.size(); ++i)
    {
        std::string const& testSuitePath = m_Tests[i].SuitePath;

        if (m_Visible[i] && ((testSuitePath == suitePath) ||
                             (testSuitePath.compare(0, nestedPrefix.size(), nestedPrefix) == 0)))
        {
            indexes.push_back(i);
        }
    }

    return indexes;
}
//---------------------------------------------------------------------------
std::size_t TGUITestList::VisibleCount() const
{
    return static_cast<std::size_t>(std::count(m_Visible.begin(), m_Visible.end(), true));
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
