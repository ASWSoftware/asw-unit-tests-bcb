/* **************************************************************************
ASWUnitTestsBCB_Handler.h
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

#ifndef ASWUnitTestsBCB_HandlerH
#define ASWUnitTestsBCB_HandlerH
//---------------------------------------------------------------------------
#include <cstddef>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_TestBase.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TPatternTestFilter
//
// Matches the tests whose full names match a wildcard pattern (see
// TTestHandler::WildcardMatch()), as --filter does.
/////////////////////////////////////////////////////////////////////////////
class TPatternTestFilter : public ITestFilter
{
private:
    bool m_IgnoreCase;
    std::string m_Pattern;

public:
    TPatternTestFilter(std::string const& pattern, bool ignoreCase);

    virtual bool Matches(std::string const& fullTestName) const;
};


/////////////////////////////////////////////////////////////////////////////
// TTestId
//
// Identifies one registered test by its group and test names, kept separate
// rather than joined into a "GroupName.TestName" full name, since a group
// name may itself contain '.'. GroupName is the group's qualified name, and
// SuitePath its suite part on its own, as in TTestCaseRecord. Returned by
// TTestHandler::GetTests().
/////////////////////////////////////////////////////////////////////////////
struct TTestId
{
    std::string GroupName;
    std::string TestName;
    std::string SuitePath; // Empty when the group isn't in a suite.

    TTestId();
    TTestId(std::string const& groupName, std::string const& testName, std::string const& suitePath = std::string());
};


/////////////////////////////////////////////////////////////////////////////
// TTestGroupOrderKey
//
// What decides a registered test group's place in the canonical run order:
// its suite path, its own name (not its qualified name), and its registered
// order (see ASWBCB_REGISTER_TEST_GROUP_ORDERED). Passed to
// TTestHandler::CanonicalGroupOrder().
/////////////////////////////////////////////////////////////////////////////
struct TTestGroupOrderKey
{
    std::string SuitePath;
    std::string Name;
    int Order;

    TTestGroupOrderKey();
    TTestGroupOrderKey(std::string const& suitePath, std::string const& name, int order);
};


/////////////////////////////////////////////////////////////////////////////
// TTestHandler
//
// Creates every registered test group (see ASWBCB_REGISTER_TEST_GROUP) and
// runs them in their canonical order (see CanonicalGroupOrder()): by
// default alphabetical, with each suite's groups together.
/////////////////////////////////////////////////////////////////////////////
class TTestHandler
{
private:
    typedef std::vector<TTestGroupBase*> TTestGroups;

private:
    std::string m_ProjectName;
    ITestRunObserver* m_RunObserver; // Not owned; nullptr when none is set.
    TTestGroups m_TestGroups;        // Owned

private:
    TTestHandler(TTestHandler const&);            // Not copyable
    TTestHandler& operator=(TTestHandler const&); // Not copyable

    // Creates the group 'registrar' registers, in the suite its registration names, if any. If its constructor
    // throws, logs that and returns a stand-in group with the same tests, in that suite, each of which fails with
    // the constructor's exception. Throws TExceptInvalidSuitePath if the constructor names a suite other than the
    // registration's. (RegisterTestGroups() rejects invalid suite paths, through CanonicalGroupOrder().)
    TTestGroupBase* CreateTestGroup(TTestGroupRegistrar const& registrar);
    void DeleteTestGroups();
    void RegisterTestGroups(TTestGroupRegistrar const* firstRegistrar);

public:
    // Returns the indexes of 'groups' in their canonical (unshuffled) run order. Within each suite, and at the top
    // level, the groups and suites in it run in ascending order of their registered order, then by name, with a
    // suite treated as order 0 and named by its own suite name. A suite's groups, and the suites inside it, always
    // run together. Groups that tie on both keep their order in 'groups'. Throws TExceptInvalidSuitePath for an
    // invalid suite path (see TSuitePath).
    static std::vector<std::size_t> CanonicalGroupOrder(std::vector<TTestGroupOrderKey> const& groups);
    static std::string GetVersionFullStr();
    static std::string GetVersionStr();
    // Returns the indexes of the groups whose suite paths are 'suitePaths', given in canonical order, in a shuffled
    // order determined entirely by 'seed'. The groups and suites at the top level are shuffled with 'seed' itself,
    // so without suites the order is ShuffledIndices(suitePaths.size(), seed). Those in each suite are shuffled
    // with 'seed' plus the FNV-1a hash of the suite's path, and a suite's groups still run together. The same
    // order as asw-unit-tests gives. Throws TExceptInvalidSuitePath for an invalid suite path (see TSuitePath).
    static std::vector<std::size_t> ShuffledGroupOrder(std::vector<std::string> const& suitePaths, unsigned int seed);
    // Whether all of 'text' matches 'pattern', where '*' matches any run of characters (none too) and '?' exactly
    // one. With 'ignoreCase', the ASCII letters A-Z match their lowercase forms. --filter uses it.
    static bool WildcardMatch(std::string const& pattern, std::string const& text, bool ignoreCase = false);

public:
    TTestHandler();
    ~TTestHandler();

    // Returns every test's full name, "GroupName.TestName" with the group's qualified name (so a group in a suite
    // starts with "Suite.Path."), in the order an unshuffled run takes (the order --list shows). Call after
    // Initialize().
    std::vector<std::string> GetAllTestFullNames();
    std::string const& GetProjectName() const;
    // Returns every test's group and test names, in the same order as GetAllTestFullNames(), for a caller that needs
    // them apart (e.g. to build a tree of groups and tests); a full name can't be split reliably, since a group name
    // may contain '.'. Call after Initialize().
    std::vector<TTestId> GetTests();
    // Creates the registered test groups and finds their tests. Throws TExceptDuplicateTestName if a group class
    // publishes a test method name twice, and TExceptInvalidSuitePath for an invalid suite path (see
    // CreateTestGroup()).
    void Initialize(std::string const& projectName = "ASWUnitTestsBCB");
    // The same for the groups of the unregistered list that 'firstRegistrar' starts (see TTestGroupRegistrar), and
    // none of the registered ones.
    void Initialize(std::string const& projectName, TTestGroupRegistrar const* firstRegistrar);
    // Logs the full name ("GroupName.TestName") of each test 'filter' matches (every test with nullptr), in the order
    // an unshuffled run takes, then how many there were, without running any. 'filterDescription' says what the
    // filter is, for the log.
    void ListTests(ITestFilter const* filter = nullptr, std::string const& filterDescription = std::string());
    void Log(std::string const& msg);
    void LogAppend(std::string const& msg);
    // Runs the tests 'filter' matches (every test with nullptr; 'filterDescription' says what the filter is, for the
    // log) and returns the combined results. A group with no matching test isn't set up at all. With 'shuffle', the
    // groups and each group's tests run in an order 'shuffleSeed' (or a new, logged seed) determines. With
    // 'testTimeoutSeconds', each test runs on a worker thread, and a test that runs longer is abandoned and recorded
    // as failed, and ends the run with TTestResults::TimedOut set (see TTestGroupBase::Run()). With 'catchCrashes', a
    // stack overflow or an illegal instruction in a test fails it instead of ending the process, and a stack overflow
    // ends the run with TTestResults::Crashed set. Throws TExceptFatalError, ending the run, if a group's
    // SetUp_Test(), TearDown_Test() or TearDown_Group() throws.
    TTestResults Run(ITestFilter const* filter = nullptr, std::string const& filterDescription = std::string(),
        bool shuffle = false, TOptional<unsigned int> shuffleSeed = TOptional<unsigned int>(),
        TOptional<unsigned int> testTimeoutSeconds = TOptional<unsigned int>(), bool catchCrashes = false);
    // Sets (or, with nullptr, clears) the observer that receives this handler's and every test group's log output and
    // per-test events; see ITestRunObserver. Not owned: it must outlive its use here. Can be called before
    // Initialize(), so Initialize()'s own output reaches the observer too.
    void SetRunObserver(ITestRunObserver* observer);
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_HandlerH
