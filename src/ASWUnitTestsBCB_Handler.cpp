/* **************************************************************************
ASWUnitTestsBCB_Handler.cpp
Author: Anthony S. West - ASW Software

Creates the registered test groups and runs them.

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
#include "ASWUnitTestsBCB_Handler.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.Diagnostics.hpp>
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Console.h"
#include "ASWUnitTestsBCB_Exception.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
#include "ASWUnitTestsBCB_Version.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Diagnostics;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

// A child of a suite, or of the top level, in ShuffledGroupOrder()'s suite tree: a group, by its index into the
// suite paths given, or a suite, by its index into the tree's suites.
struct TSuiteChild
{
    bool IsSuite;
    std::size_t Index;
};

// A suite in ShuffledGroupOrder()'s suite tree.
struct TSuiteNode
{
    std::string Path; // Empty for the top level.
    std::vector<TSuiteChild> Children;
};

// One level of a group's sort key in CanonicalGroupOrder(): a suite's (order 0, its own name, kind 1) or the
// group's own (its order, its name, kind 0).
struct TSortLevel
{
    int Order;
    std::string Name;
    int Kind;
};

void AppendShuffled(std::vector<TSuiteNode> const& suites, std::size_t suiteIndex, unsigned int seed,
    std::vector<std::size_t>& order);
bool CharsMatch(char a, char b, bool ignoreCase);
bool SortKeyLess(std::vector<TSortLevel> const& a, std::vector<TSortLevel> const& b);

//---------------------------------------------------------------------------
// Appends to 'order' the groups in suite 'suiteIndex' of 'suites' and the suites inside it, visiting each suite's
// children in the order ShuffledIndices() gives: 'seed' for the top level, 'seed' plus the FNV-1a hash of its path
// for a suite.
void AppendShuffled(std::vector<TSuiteNode> const& suites, std::size_t suiteIndex, unsigned int seed,
    std::vector<std::size_t>& order)
{
    TSuiteNode const& suite = suites[suiteIndex];
    unsigned int const suiteSeed = suite.Path.empty() ? seed : (seed + ASWUnitTestsBCB::FNV1aHash(suite.Path));
    std::vector<std::size_t> const childOrder = ASWUnitTestsBCB::ShuffledIndices(suite.Children.size(), suiteSeed);

    for (std::vector<std::size_t>::const_iterator it = childOrder.begin(); it != childOrder.end(); ++it)
    {
        TSuiteChild const& child = suite.Children[*it];

        if (child.IsSuite)
            AppendShuffled(suites, child.Index, seed, order);
        else
            order.push_back(child.Index);
    }
}
//---------------------------------------------------------------------------
// Whether 'a' and 'b' are the same character, with 'ignoreCase' treating the ASCII letters A-Z as lowercase.
bool CharsMatch(char a, char b, bool ignoreCase)
{
    if (ignoreCase)
    {
        if ((a >= 'A') && (a <= 'Z'))
            a = static_cast<char>(a - 'A' + 'a');

        if ((b >= 'A') && (b <= 'Z'))
            b = static_cast<char>(b - 'A' + 'a');
    }

    return a == b;
}
//---------------------------------------------------------------------------
// Whether sort key 'a' comes before 'b', comparing level by level (order, then name, then kind), with a key that
// runs out first coming first.
bool SortKeyLess(std::vector<TSortLevel> const& a, std::vector<TSortLevel> const& b)
{
    for (std::size_t i = 0; (i < a.size()) && (i < b.size()); ++i)
    {
        if (a[i].Order != b[i].Order)
            return a[i].Order < b[i].Order;

        if (a[i].Name != b[i].Name)
            return a[i].Name < b[i].Name;

        if (a[i].Kind != b[i].Kind)
            return a[i].Kind < b[i].Kind;
    }

    return a.size() < b.size();
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TUncreatableTestGroup
//
// Stands in for a registered test group whose constructor threw: it has
// that class's tests, and fails each of them with 'failure' instead of
// running it, without calling any SetUp or TearDown method.
/////////////////////////////////////////////////////////////////////////////
class TUncreatableTestGroup : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

private:
    std::string m_Failure;
    TClass m_GroupClass;

protected:
    virtual TClass GetTestClass();

public:
    __fastcall TUncreatableTestGroup(TClass groupClass, TSuitePath const& suitePath, std::string const& failure);

    virtual void Run(ITestFilter const* filter, TOptional<unsigned int> shuffleSeed,
        TOptional<unsigned int> testTimeoutSeconds, bool catchCrashes);
};

//---------------------------------------------------------------------------
__fastcall TUncreatableTestGroup::TUncreatableTestGroup(TClass groupClass, TSuitePath const& suitePath,
    std::string const& failure)
    : inherited(suitePath, ToUTF8(groupClass->ClassName())),
      m_Failure(failure),
      m_GroupClass(groupClass)
{
}
//---------------------------------------------------------------------------
TClass TUncreatableTestGroup::GetTestClass()
{
    return m_GroupClass;
}
//---------------------------------------------------------------------------
void TUncreatableTestGroup::Run(ITestFilter const* filter, TOptional<unsigned int> shuffleSeed,
    TOptional<unsigned int> testTimeoutSeconds, bool catchCrashes)
{
    m_Results = TTestResults();
    RunTests(m_Failure, filter, shuffleSeed, testTimeoutSeconds, catchCrashes);
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TPatternTestFilter
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TPatternTestFilter::TPatternTestFilter(std::string const& pattern, bool ignoreCase)
    : m_IgnoreCase(ignoreCase),
      m_Pattern(pattern)
{
}
//---------------------------------------------------------------------------
bool TPatternTestFilter::Matches(std::string const& fullTestName) const
{
    return TTestHandler::WildcardMatch(m_Pattern, fullTestName, m_IgnoreCase);
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TTestId
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TTestId::TTestId()
{
}
//---------------------------------------------------------------------------
TTestId::TTestId(std::string const& groupName, std::string const& testName, std::string const& suitePath)
    : GroupName(groupName),
      TestName(testName),
      SuitePath(suitePath)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TTestGroupOrderKey
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TTestGroupOrderKey::TTestGroupOrderKey()
    : Order(0)
{
}
//---------------------------------------------------------------------------
TTestGroupOrderKey::TTestGroupOrderKey(std::string const& suitePath, std::string const& name, int order)
    : SuitePath(suitePath),
      Name(name),
      Order(order)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TTestHandler
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TTestHandler::TTestHandler()
    : m_RunObserver(nullptr)
{
}
//---------------------------------------------------------------------------
TTestHandler::~TTestHandler()
{
    DeleteTestGroups();
}
//---------------------------------------------------------------------------
/*
    TTestHandler::CanonicalGroupOrder

    Gives each group a sort key with one level per level of its path: one for each suite it's in, outermost first,
    then one for the group itself. A suite's level is (0, its suite name, 1) and the group's is (its order, its
    name, 0), so comparing two keys level by level compares the groups, or the suites holding them, at the first
    level where their paths differ. A group and a suite that tie on order and name at the same level sort the group
    first. Without suites, each key is a single (order, name, 0) level, which sorts exactly as order then name did
    before suites existed. The same order as asw-unit-tests gives; an insertion sort keeps tied groups in order,
    since bcc32 warns about std::stable_sort.
*/
std::vector<std::size_t> TTestHandler::CanonicalGroupOrder(std::vector<TTestGroupOrderKey> const& groups)
{
    std::vector<std::vector<TSortLevel> > sortKeys;
    sortKeys.reserve(groups.size());

    for (std::vector<TTestGroupOrderKey>::const_iterator it = groups.begin(); it != groups.end(); ++it)
    {
        TSuitePath const suitePath(it->SuitePath);

        if (!suitePath.IsValid())
            throw TExceptInvalidSuitePath(suitePath.GetPath());

        std::vector<std::string> const& suiteNames = suitePath.GetNames();
        std::vector<TSortLevel> sortKey;

        for (std::vector<std::string>::const_iterator nameIt = suiteNames.begin(); nameIt != suiteNames.end();
             ++nameIt)
        {
            TSortLevel const suiteLevel = { 0, *nameIt, 1 };
            sortKey.push_back(suiteLevel);
        }

        TSortLevel const groupLevel = { it->Order, it->Name, 0 };
        sortKey.push_back(groupLevel);
        sortKeys.push_back(sortKey);
    }

    std::vector<std::size_t> order;
    order.reserve(groups.size());

    for (std::size_t index = 0; index < groups.size(); ++index)
    {
        // After every group that sorts before it or equal to it
        std::size_t position = order.size();

        while ((position > 0) && SortKeyLess(sortKeys[index], sortKeys[order[position - 1]]))
            --position;

        order.insert(order.begin() + static_cast<std::ptrdiff_t>(position), index);
    }

    return order;
}
//---------------------------------------------------------------------------
TTestGroupBase* TTestHandler::CreateTestGroup(TTestGroupRegistrar const& registrar)
{
    std::string const registeredPath = registrar.SuitePath;
    std::unique_ptr<TTestGroupBase> group;
    std::string errorDescription;

    try
    {
        group.reset(registrar.Factory());
    }
    catch (std::exception const& stdEx)
    {
        errorDescription = DescribeException(stdEx);
    }
    catch (Exception& rtlEx)
    {
        errorDescription = DescribeException(rtlEx);
    }
    catch (...)
    {
        errorDescription = DescribeUnknownException();
    }

    if (group.get() != nullptr)
    {
        std::string const constructedPath = group->GetSuitePath();

        if (!registeredPath.empty() && (constructedPath != registeredPath))
        {
            if (!constructedPath.empty())
            {
                throw TExceptInvalidSuitePath::Conflicting(group->GetTestGroupName(), registeredPath,
                    constructedPath);
            }

            group->SetSuitePath(registeredPath);
        }

        return group.release();
    }

    std::string const failure = "Test group class \"" + ToUTF8(registrar.GroupClass->ClassName()) +
        "\" could not be created: " + errorDescription;

    Log("!!ERROR!!: " + failure);
    return new TUncreatableTestGroup(registrar.GroupClass, TSuitePath(registeredPath), failure);
}
//---------------------------------------------------------------------------
void TTestHandler::DeleteTestGroups()
{
    for (TTestGroups::iterator it = m_TestGroups.begin(); it != m_TestGroups.end(); ++it)
        delete *it;

    m_TestGroups.clear();
}
//---------------------------------------------------------------------------
std::vector<std::string> TTestHandler::GetAllTestFullNames()
{
    std::vector<TTestId> const tests = GetTests();
    std::vector<std::string> fullNames;
    fullNames.reserve(tests.size());

    for (std::vector<TTestId>::const_iterator it = tests.begin(); it != tests.end(); ++it)
        fullNames.push_back(it->GroupName + "." + it->TestName);

    return fullNames;
}
//---------------------------------------------------------------------------
std::string const& TTestHandler::GetProjectName() const
{
    return m_ProjectName;
}
//---------------------------------------------------------------------------
std::vector<TTestId> TTestHandler::GetTests()
{
    std::vector<TTestId> tests;

    for (TTestGroups::iterator it = m_TestGroups.begin(); it != m_TestGroups.end(); ++it)
    {
        TTestGroupBase::TestCallbackList const& groupTests = (*it)->GetTestCallbackList();

        for (TTestGroupBase::TestCallbackList::const_iterator testIt = groupTests.begin();
             testIt != groupTests.end(); ++testIt)
        {
            tests.push_back(TTestId((*it)->GetQualifiedTestGroupName(), (*testIt)->GetName(), (*it)->GetSuitePath()));
        }
    }

    return tests;
}
//---------------------------------------------------------------------------
std::string TTestHandler::GetVersionFullStr()
{
    return "ASWUnitTestsBCB - Version " + GetVersionStr();
}
//---------------------------------------------------------------------------
std::string TTestHandler::GetVersionStr()
{
    return ASWUNITTESTSBCB_VERSION_STRING;
}
//---------------------------------------------------------------------------
void TTestHandler::Initialize(std::string const& projectName)
{
    Initialize(projectName, TTestGroupRegistrar::First());
}
//---------------------------------------------------------------------------
void TTestHandler::Initialize(std::string const& projectName, TTestGroupRegistrar const* firstRegistrar)
{
    m_ProjectName = projectName;

    Log(GetVersionFullStr());

    Log("Initializing test handler and registering test groups for " + m_ProjectName + "...");
    RegisterTestGroups(firstRegistrar);

    Log("Test groups registered: " + UIntToString(static_cast<unsigned int>(m_TestGroups.size())));

    std::size_t totalTests = 0;

    for (TTestGroups::iterator it = m_TestGroups.begin(); it != m_TestGroups.end(); ++it)
        totalTests += (*it)->GetTestCallbackList().size();

    Log("Total tests registered: " + UIntToString(static_cast<unsigned int>(totalTests)));
}
//---------------------------------------------------------------------------
void TTestHandler::ListTests(ITestFilter const* filter, std::string const& filterDescription)
{
    if (filter != nullptr)
        Log("Listing tests, filtered: " + filterDescription);
    else
        Log("Listing tests, unfiltered.");

    unsigned int totalTests = 0;
    unsigned int matchedTests = 0;
    unsigned int const totalGroups = static_cast<unsigned int>(m_TestGroups.size());
    unsigned int matchedGroups = 0;

    for (TTestGroups::iterator it = m_TestGroups.begin(); it != m_TestGroups.end(); ++it)
    {
        TTestGroupBase& testGroup = **it;
        TTestGroupBase::TestCallbackList const& tests = testGroup.GetTestCallbackList();
        bool groupHasMatch = false;

        for (TTestGroupBase::TestCallbackList::const_iterator testIt = tests.begin(); testIt != tests.end();
             ++testIt)
        {
            std::string const fullName = testGroup.GetQualifiedTestGroupName() + "." + (*testIt)->GetName();

            ++totalTests;

            if ((filter == nullptr) || filter->Matches(fullName))
            {
                Log(fullName);
                ++matchedTests;
                groupHasMatch = true;
            }
        }

        if (groupHasMatch)
            ++matchedGroups;
    }

    if (filter != nullptr)
    {
        Log("Listed " + UIntToString(matchedTests) + " of " + UIntToString(totalTests) + " registered test(s), in " +
            UIntToString(matchedGroups) + " of " + UIntToString(totalGroups) +
            " registered group(s), matching the filter.");
    }
    else
    {
        Log("Listed " + UIntToString(totalTests) + " test(s) in " + UIntToString(totalGroups) + " group(s).");
    }
}
//---------------------------------------------------------------------------
void TTestHandler::Log(std::string const& msg)
{
    if (m_RunObserver != nullptr)
    {
        m_RunObserver->OnLog(TConsole::StripColor(msg) + "\n");
        return;
    }

    std::cout << msg << std::endl;
}
//---------------------------------------------------------------------------
void TTestHandler::LogAppend(std::string const& msg)
{
    if (m_RunObserver != nullptr)
    {
        m_RunObserver->OnLog(TConsole::StripColor(msg));
        return;
    }

    std::cout << msg;
}
//---------------------------------------------------------------------------
/*
    TTestHandler::RegisterTestGroups

    Creates every test group in the list that 'firstRegistrar' starts (normally those registered with
    ASWBCB_REGISTER_TEST_GROUP and its variants; see ASWUnitTestsBCB_Registry.h), and keeps them in their canonical
    order (see CanonicalGroupOrder()): by default alphabetical, with each suite's groups together, and not dependent
    on the order the linker put the test units in.
*/
void TTestHandler::RegisterTestGroups(TTestGroupRegistrar const* firstRegistrar)
{
    DeleteTestGroups();

    std::vector<TTestGroupOrderKey> orderKeys; // By index in m_TestGroups

    for (TTestGroupRegistrar const* registrar = firstRegistrar; registrar != nullptr;
         registrar = registrar->Next)
    {
        // Owned by m_TestGroups at once, so the groups already created are deleted if a later one throws.
        m_TestGroups.reserve(m_TestGroups.size() + 1);
        TTestGroupBase* const group = CreateTestGroup(*registrar);
        m_TestGroups.push_back(group);

        group->SetRunObserver(m_RunObserver);
        orderKeys.push_back(TTestGroupOrderKey(group->GetSuitePath(), group->GetTestGroupName(), registrar->Order));
    }

    std::vector<std::size_t> const order = CanonicalGroupOrder(orderKeys);
    TTestGroups ordered;
    ordered.reserve(order.size());

    for (std::vector<std::size_t>::const_iterator it = order.begin(); it != order.end(); ++it)
        ordered.push_back(m_TestGroups[*it]);

    m_TestGroups.swap(ordered);
}
//---------------------------------------------------------------------------
/*
    TTestHandler::Run

    Runs every group (see TTestGroupBase::Run()) and returns the combined results.

    With a 'filter', only the tests whose "GroupName.TestName" full names it matches run; a group with none isn't set
    up or logged, and the log numbers only the groups that run. The log always says whether a filter is active (and
    which, from 'filterDescription'), so redirected output still explains why fewer tests ran.

    With 'shuffle', the groups run in a random order that keeps each suite's groups together (see
    ShuffledGroupOrder()), and so do each group's tests, each group's from a seed made of the run's seed and the
    group's qualified name (so groups of the same size don't all shuffle alike). The run's seed is
    'shuffleSeed' if given, otherwise a new one, and is logged so a failure caused by the order can be reproduced. A
    seed gives the same order with every compiler, and the same as asw-unit-tests (see ShuffledIndices() and
    FNV1aHash()). The groups, like each group's tests, are shuffled before 'filter' is applied, so a filtered run
    (e.g. rerunning just the failures) keeps the relative order the same seed gives an unfiltered one.

    With 'testTimeoutSeconds', each test runs on a worker thread (see TTestGroupBase::RunWithTimeout()). A test that
    doesn't end in time is abandoned and recorded as failed, its group is torn down and counted, no further tests or
    groups run, and the returned TTestResults has TimedOut set.

    With 'catchCrashes', a test that crashes (see TTestGroupBase::RunCatchingCrashes()) is recorded as failed, and
    after a stack overflow the run ends the same way, with Crashed set.

    With an ITestRunObserver set (see SetRunObserver()), StopRequested() is checked before each group (and, by each
    group, before each test). Once it returns true, no further tests or groups run, a group that was part-way
    through is still torn down and counted, and the returned TTestResults has Stopped set.
*/
TTestResults TTestHandler::Run(ITestFilter const* filter, std::string const& filterDescription, bool shuffle,
    TOptional<unsigned int> shuffleSeed, TOptional<unsigned int> testTimeoutSeconds, bool catchCrashes)
{
    TTestResults testResults;
    TTestGroups groupsToRun; // Not owned

    if (filter != nullptr)
        Log("Filter: " + filterDescription);
    else
        Log("No filter: running all tests.");

    TOptional<unsigned int> resolvedSeed;
    std::vector<std::size_t> groupOrder;

    if (shuffle)
    {
        std::vector<std::string> suitePaths;
        suitePaths.reserve(m_TestGroups.size());

        for (TTestGroups::const_iterator it = m_TestGroups.begin(); it != m_TestGroups.end(); ++it)
            suitePaths.push_back((*it)->GetSuitePath());

        resolvedSeed = shuffleSeed.HasValue() ? shuffleSeed.Value() : NewShuffleSeed();
        groupOrder = ShuffledGroupOrder(suitePaths, resolvedSeed.Value());
    }
    else
    {
        groupOrder.resize(m_TestGroups.size());

        for (std::size_t i = 0; i < groupOrder.size(); ++i)
            groupOrder[i] = i;
    }

    for (std::vector<std::size_t>::const_iterator it = groupOrder.begin(); it != groupOrder.end(); ++it)
    {
        TTestGroupBase* const testGroup = m_TestGroups[*it];

        if ((filter == nullptr) || (testGroup->CountTests(filter) > 0))
            groupsToRun.push_back(testGroup);
    }

    if ((filter != nullptr) && groupsToRun.empty())
        Log("No registered tests matched the filter.");

    if (resolvedSeed.HasValue())
        Log("Shuffle: enabled (seed " + UIntToString(resolvedSeed.Value()) + ")");
    else
        Log("Shuffle: disabled");

    unsigned int const groupCount = static_cast<unsigned int>(groupsToRun.size());
    unsigned int groupNumber = 0;
    TStopwatch stopwatch = TStopwatch::StartNew();

    Log("\n[" + GetUTCTimeISO8601() + "] Tests started.\n");

    for (TTestGroups::iterator it = groupsToRun.begin(); it != groupsToRun.end(); ++it)
    {
        TTestGroupBase& testGroup = **it;

        if ((m_RunObserver != nullptr) && m_RunObserver->StopRequested())
        {
            testResults.Stopped = true;
            break;
        }

        Log("--------------------------------------------------------------------------------");
        Log("[" + GetUTCTimeISO8601() + "] Setting up group " + UIntToString(++groupNumber) + " of " +
            UIntToString(groupCount) + ": \"" + testGroup.GetQualifiedTestGroupName() + "\"");

        TOptional<unsigned int> groupSeed;

        if (resolvedSeed.HasValue())
            groupSeed = resolvedSeed.Value() + FNV1aHash(testGroup.GetQualifiedTestGroupName());

        testGroup.Run(filter, groupSeed, testTimeoutSeconds, catchCrashes);

        TTestResults const& testGroupResults = testGroup.Results();

        testResults.FailedCount += testGroupResults.FailedCount;
        testResults.SkippedCount += testGroupResults.SkippedCount;
        testResults.SuccessCount += testGroupResults.SuccessCount;
        testResults.AddMessages(testGroupResults.Messages);
        testResults.CaseRecords.insert(testResults.CaseRecords.end(), testGroupResults.CaseRecords.begin(),
            testGroupResults.CaseRecords.end());

        if (testGroupResults.TimedOut || testGroupResults.Crashed)
        {
            testResults.TimedOut = testGroupResults.TimedOut;
            testResults.Crashed = testGroupResults.Crashed;
            break;
        }

        if (testGroupResults.Stopped)
        {
            testResults.Stopped = true;
            break;
        }
    }

    Log("--------------------------------------------------------------------------------");

    if (testResults.Stopped)
        Log("Run stopped on request before every test ran.");

    Log("\n[" + GetUTCTimeISO8601() + "] Tests done: Totals: " +
        TConsole::Colorize("succeeded: " + UIntToString(testResults.SuccessCount), TLogKind::Pass) + ", " +
        TConsole::Colorize("failed: " + UIntToString(testResults.FailedCount), TLogKind::Fail) + ", " +
        TConsole::Colorize("skipped: " + UIntToString(testResults.SkippedCount), TLogKind::Skip));
    Log("Elapsed time: " + FormatElapsedTime(stopwatch.ElapsedMilliseconds));

    return testResults;
}
//---------------------------------------------------------------------------
void TTestHandler::SetRunObserver(ITestRunObserver* observer)
{
    m_RunObserver = observer;

    for (TTestGroups::iterator it = m_TestGroups.begin(); it != m_TestGroups.end(); ++it)
        (*it)->SetRunObserver(observer);
}
//---------------------------------------------------------------------------
/*
    TTestHandler::ShuffledGroupOrder

    Builds the suite tree from the groups' suite paths: each suite's children are its groups and the suites inside
    it, in the order they first appear in 'suitePaths', with the top level as the root. Then walks the tree,
    visiting each suite's children in an order from ShuffledIndices() (see AppendShuffled()), so each suite's
    groups come out together.
*/
std::vector<std::size_t> TTestHandler::ShuffledGroupOrder(std::vector<std::string> const& suitePaths,
    unsigned int seed)
{
    std::vector<TSuiteNode> suites(1); // suites[0] is the top level.
    std::map<std::string, std::size_t> suiteIndexByPath;

    for (std::size_t groupIndex = 0; groupIndex < suitePaths.size(); ++groupIndex)
    {
        TSuitePath const suitePath(suitePaths[groupIndex]);

        if (!suitePath.IsValid())
            throw TExceptInvalidSuitePath(suitePath.GetPath());

        std::vector<std::string> const& suiteNames = suitePath.GetNames();
        std::size_t parent = 0;
        std::string path;

        for (std::vector<std::string>::const_iterator it = suiteNames.begin(); it != suiteNames.end(); ++it)
        {
            path += (path.empty() ? std::string() : std::string(".")) + *it;

            std::map<std::string, std::size_t>::const_iterator const found = suiteIndexByPath.find(path);

            if (found != suiteIndexByPath.end())
            {
                parent = found->second;
                continue;
            }

            std::size_t const suiteIndex = suites.size();
            TSuiteChild const suiteChild = { true, suiteIndex };

            suites.push_back(TSuiteNode());
            suites.back().Path = path;
            suites[parent].Children.push_back(suiteChild);
            suiteIndexByPath[path] = suiteIndex;
            parent = suiteIndex;
        }

        TSuiteChild const groupChild = { false, groupIndex };
        suites[parent].Children.push_back(groupChild);
    }

    std::vector<std::size_t> order;
    order.reserve(suitePaths.size());
    AppendShuffled(suites, 0, seed, order);
    return order;
}
//---------------------------------------------------------------------------
bool TTestHandler::WildcardMatch(std::string const& pattern, std::string const& text, bool ignoreCase)
{
    // Matches greedily, going back only to the last '*' on a mismatch, which is enough when '*' is the only
    // wildcard that matches more than one character.
    std::size_t p = 0;
    std::size_t t = 0;
    std::size_t starPos = std::string::npos;
    std::size_t starMatchEnd = 0;

    while (t < text.size())
    {
        if ((p < pattern.size()) && (pattern[p] == '*'))
        {
            starPos = p++;
            starMatchEnd = t;
        }
        else if ((p < pattern.size()) && ((pattern[p] == '?') || CharsMatch(pattern[p], text[t], ignoreCase)))
        {
            ++p;
            ++t;
        }
        else if (starPos != std::string::npos)
        {
            p = starPos + 1;
            t = ++starMatchEnd;
        }
        else
        {
            return false;
        }
    }

    while ((p < pattern.size()) && (pattern[p] == '*'))
        ++p;

    return p == pattern.size();
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
