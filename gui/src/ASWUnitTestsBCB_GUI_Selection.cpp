/* **************************************************************************
ASWUnitTestsBCB_GUI_Selection.cpp
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
#include "ASWUnitTestsBCB_GUI_Selection.h"
//---------------------------------------------------------------------------
#include <map>
#include <utility>
//---------------------------------------------------------------------------
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_Layout.h"
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

// The selection file's first line, describing the lines after it for anyone who opens it.
char const SelectionFileComment[] =
    "; ASWUnitTestsBCB VCL GUI test selection: \"+\" checked or \"-\" unchecked, then the group and test names, "
    "separated by a tab";

bool IsStorableName(std::string const& name);
std::string Plural(std::size_t count, char const* singular, char const* plural);

//---------------------------------------------------------------------------
// Whether 'name' fits on one line of the selection file, without its tab separator or line break.
bool IsStorableName(std::string const& name)
{
    return name.find_first_of("\t\r\n") == std::string::npos;
}
//---------------------------------------------------------------------------
// e.g. "1 test" or "3 tests"
std::string Plural(std::size_t count, char const* singular, char const* plural)
{
    return ASWUnitTestsBCB::UIntToString(static_cast<unsigned int>(count)) + " " + ((count == 1) ? singular : plural);
}
//---------------------------------------------------------------------------

} // namespace


namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TGUISavedTest
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TGUISavedTest::TGUISavedTest()
    : Checked(false)
{
}
//---------------------------------------------------------------------------
TGUISavedTest::TGUISavedTest(bool checked, std::string const& groupName, std::string const& testName)
    : Checked(checked),
      GroupName(groupName),
      TestName(testName)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TGUISelectionRestore
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TGUISelectionRestore::TGUISelectionRestore()
    : CheckedAll(false),
      CheckedCount(0),
      MissingCount(0),
      NewCheckedCount(0),
      NewCount(0)
{
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
std::vector<TGUISavedTest> CaptureGUISelection(TGUITestList const& testList)
{
    std::vector<TGUISavedTest> tests;
    tests.reserve(testList.Count());

    for (std::size_t i = 0; i < testList.Count(); ++i)
    {
        TTestId const& test = testList.Test(i);
        tests.push_back(TGUISavedTest(testList.IsChecked(i) && testList.IsVisible(i), test.GroupName, test.TestName));
    }

    return tests;
}
//---------------------------------------------------------------------------
std::string DescribeGUISelectionRestore(TGUISelectionRestore const& restore)
{
    if (restore.CheckedAll)
        return "None of the saved selection's checked tests exist any more, so every test is checked.";

    std::string text = "Restored the saved selection: " + Plural(restore.CheckedCount, "test", "tests") + " checked";

    if (restore.MissingCount > 0)
    {
        text += "; " + Plural(restore.MissingCount, "saved test no longer exists",
            "saved tests no longer exist");
    }

    if (restore.NewCount > 0)
    {
        text += "; " + Plural(restore.NewCount, "new or renamed test", "new or renamed tests") + " (" +
            UIntToString(static_cast<unsigned int>(restore.NewCheckedCount)) + " checked)";
    }

    return text + ".";
}
//---------------------------------------------------------------------------
std::string FormatGUISelection(std::vector<TGUISavedTest> const& tests)
{
    std::string text = std::string(SelectionFileComment) + "\r\n";

    for (std::vector<TGUISavedTest>::const_iterator it = tests.begin(); it != tests.end(); ++it)
    {
        if (IsStorableName(it->GroupName) && IsStorableName(it->TestName))
            text += (it->Checked ? "+" : "-") + it->GroupName + "\t" + it->TestName + "\r\n";
    }

    return text;
}
//---------------------------------------------------------------------------
UnicodeString GUISelectionFilePath()
{
    return ChangeFileExt(GUILayoutFilePath(), ".selection");
}
//---------------------------------------------------------------------------
std::vector<TGUISavedTest> ParseGUISelection(std::string const& text)
{
    std::vector<TGUISavedTest> tests;

    // Past a UTF-8 byte order mark, in case the file was edited with something that adds one.
    std::size_t start = (text.compare(0, 3, "\xEF\xBB\xBF") == 0) ? 3 : 0;

    while (start < text.size())
    {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos)
            end = text.size();

        std::string line = text.substr(start, end - start);
        start = end + 1;

        if (!line.empty() && (line[line.size() - 1] == '\r'))
            line.erase(line.size() - 1);

        std::size_t const tab = line.find('\t');
        if (line.empty() || ((line[0] != '+') && (line[0] != '-')) || (tab == std::string::npos))
            continue;

        tests.push_back(TGUISavedTest(line[0] == '+', line.substr(1, tab - 1), line.substr(tab + 1)));
    }

    return tests;
}
//---------------------------------------------------------------------------
TGUISelectionRestore RestoreGUISelection(TGUITestList& testList, std::vector<TGUISavedTest> const& saved)
{
    typedef std::map<std::pair<std::string, std::string>, bool> TSavedChecked;
    typedef std::map<std::string, bool> TGroupFullyChecked;

    TGUISelectionRestore restore;

    if (saved.empty())
        return restore;

    // By group and test name, so a test saved more than once counts once, with its last state.
    TSavedChecked savedChecked;
    for (std::vector<TGUISavedTest>::const_iterator it = saved.begin(); it != saved.end(); ++it)
        savedChecked[std::make_pair(it->GroupName, it->TestName)] = it->Checked;

    TGroupFullyChecked groupFullyChecked;
    bool allChecked = true;
    bool anyChecked = false;

    for (TSavedChecked::const_iterator it = savedChecked.begin(); it != savedChecked.end(); ++it)
    {
        bool const checked = it->second;
        TGroupFullyChecked::iterator const group =
            groupFullyChecked.insert(std::make_pair(it->first.first, true)).first;
        group->second = group->second && checked;
        allChecked = allChecked && checked;
        anyChecked = anyChecked || checked;
    }

    std::size_t foundCount = 0;

    for (std::size_t i = 0; i < testList.Count(); ++i)
    {
        TTestId const& test = testList.Test(i);
        TSavedChecked::const_iterator const savedTest = savedChecked.find(std::make_pair(test.GroupName,
            test.TestName));
        bool checked = false;

        if (savedTest != savedChecked.end())
        {
            checked = savedTest->second;
            ++foundCount;
        }
        else
        {
            TGroupFullyChecked::const_iterator const group = groupFullyChecked.find(test.GroupName);
            checked = (group != groupFullyChecked.end()) ? group->second : allChecked;
            ++restore.NewCount;

            if (checked)
                ++restore.NewCheckedCount;
        }

        testList.SetChecked(i, checked);

        if (checked)
            ++restore.CheckedCount;
    }

    restore.MissingCount = savedChecked.size() - foundCount;

    // An empty selection is no use when the saved one wasn't empty, e.g. after every checked test was renamed.
    if (anyChecked && (restore.CheckedCount == 0))
    {
        testList.CheckMatching(nullptr);
        restore.CheckedAll = true;
        restore.CheckedCount = testList.Count();
    }

    return restore;
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
