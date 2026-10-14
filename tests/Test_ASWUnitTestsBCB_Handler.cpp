/* **************************************************************************
Test_ASWUnitTestsBCB_Handler.cpp
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_Handler.h and ASWUnitTestsBCB_Registry.h, run on lists of unregistered groups.

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
#include "Test_ASWUnitTestsBCB_Handler.h"
//---------------------------------------------------------------------------
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Exception.h"
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
#include "ASWUnitTestsBCB_Version.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TRecordingObserver
//
// Records what a handler tells its observer, which also keeps the inner
// handlers' output off the console.
/////////////////////////////////////////////////////////////////////////////
class TRecordingObserver : public ASWUnitTestsBCB::ITestRunObserver
{
public:
    std::string Events; // "S:Group.Test;" when a test starts, "F:Group.Test=pass;" when it finishes
    std::string LogText;
    int StopAfterStarted; // Stop once this many tests have started; -1 never
    int Started;

public:
    TRecordingObserver()
        : StopAfterStarted(-1),
          Started(0)
    {
    }

    virtual void OnLog(std::string const& text)
    {
        LogText += text;
    }

    virtual void OnTestFinished(ASWUnitTestsBCB::TTestCaseRecord const& record)
    {
        char const* const outcome = (record.Outcome == ASWUnitTestsBCB::TTestOutcome::Pass) ? "pass" :
                (record.Outcome == ASWUnitTestsBCB::TTestOutcome::Fail) ? "fail" : "skip";
        Events += "F:" + record.GroupName + "." + record.TestName + "=" + outcome + ";";
    }

    virtual void OnTestStarted(std::string const& groupName, std::string const& testName)
    {
        ++Started;
        Events += "S:" + groupName + "." + testName + ";";
    }

    virtual bool StopRequested()
    {
        return (StopAfterStarted >= 0) && (Started >= StopAfterStarted);
    }
};

/////////////////////////////////////////////////////////////////////////////
// TNamesFilter
//
// Matches the tests named in a list such as "A_Group.One;B_Group.Pass".
/////////////////////////////////////////////////////////////////////////////
class TNamesFilter : public ASWUnitTestsBCB::ITestFilter
{
private:
    std::string m_Names; // ";" + the list + ";"

public:
    explicit TNamesFilter(std::string const& names)
        : m_Names(";" + names + ";")
    {
    }

    virtual bool Matches(std::string const& fullTestName) const
    {
        return m_Names.find(";" + fullTestName + ";") != std::string::npos;
    }
};

/////////////////////////////////////////////////////////////////////////////
// TCanonicalOrderOfInvalidPath, TShuffledOrderOfInvalidPath
//
// Order groups with an invalid suite path, for CheckThrows().
/////////////////////////////////////////////////////////////////////////////
struct TCanonicalOrderOfInvalidPath
{
    void operator()() const
    {
        std::vector<ASWUnitTestsBCB::TTestGroupOrderKey> const groups(1,
            ASWUnitTestsBCB::TTestGroupOrderKey("S..T", "G", 0));
        ASWUnitTestsBCB::TTestHandler::CanonicalGroupOrder(groups);
    }
};

struct TShuffledOrderOfInvalidPath
{
    void operator()() const
    {
        std::vector<std::string> suitePaths;
        suitePaths.push_back("");
        suitePaths.push_back("S.");
        ASWUnitTestsBCB::TTestHandler::ShuffledGroupOrder(suitePaths, 1);
    }
};

bool volatile g_KeepRecursing = true;

bool Contains(std::string const& text, std::string const& part);
std::string JoinIndices(std::vector<std::size_t> const& indices);
int Recurse(int depth);
std::string RunOrder(ASWUnitTestsBCB::TTestResults const& results);

//---------------------------------------------------------------------------
bool Contains(std::string const& text, std::string const& part)
{
    return text.find(part) != std::string::npos;
}
//---------------------------------------------------------------------------
// 'indices' joined with ",", so a whole order can be compared, and shown on failure, as one string.
std::string JoinIndices(std::vector<std::size_t> const& indices)
{
    std::string joined;

    for (std::size_t i = 0; i < indices.size(); ++i)
        joined += ((i == 0) ? std::string() : std::string(",")) +
            ASWUnitTestsBCB::UIntToString(static_cast<unsigned int>(indices[i]));

    return joined;
}
//---------------------------------------------------------------------------
// Recurses until the stack overflows, a kilobyte at a time. (g_KeepRecursing only keeps compilers from warning about
// endless recursion.)
int Recurse(int depth)
{
    char volatile buffer[1024];
    buffer[0] = static_cast<char>(depth);
    return (g_KeepRecursing ? Recurse(depth + 1) : 0) + buffer[0];
}
//---------------------------------------------------------------------------
// Returns the tests 'results' records, in the order they ran, as "Group.Test;Group.Test;".
std::string RunOrder(ASWUnitTestsBCB::TTestResults const& results)
{
    std::string order;

    for (std::size_t i = 0; i < results.CaseRecords.size(); ++i)
        order += results.CaseRecords[i].GroupName + "." + results.CaseRecords[i].TestName + ";";

    return order;
}
//---------------------------------------------------------------------------

} // namespace


namespace ASWUnitTestsBCB
{

// The inner groups the tests below run. None is registered, so the program's own run never includes them.

/////////////////////////////////////////////////////////////////////////////
// TInnerH_A, TInnerH_B, TInnerH_Many, TInnerH_First
/////////////////////////////////////////////////////////////////////////////
class TInnerH_A : public TTestGroupBase
{
public:
    __fastcall TInnerH_A()
        : TTestGroupBase("A_Group")
    {
    }

__published:
    void __fastcall Test_One();
};

void __fastcall TInnerH_A::Test_One()
{
}

class TInnerH_B : public TTestGroupBase
{
public:
    __fastcall TInnerH_B()
        : TTestGroupBase("B_Group")
    {
    }

__published:
    void __fastcall Test_Pass();
    void __fastcall Test_Fail();
};

void __fastcall TInnerH_B::Test_Pass()
{
}

void __fastcall TInnerH_B::Test_Fail()
{
    CheckEquals(1, 2, ASWBCB_HERE);
}

class TInnerH_Many : public TTestGroupBase
{
public:
    __fastcall TInnerH_Many()
        : TTestGroupBase("M_Group")
    {
    }

__published:
    void __fastcall Test_T1();
    void __fastcall Test_T2();
    void __fastcall Test_T3();
    void __fastcall Test_T4();
    void __fastcall Test_T5();
};

void __fastcall TInnerH_Many::Test_T1()
{
}

void __fastcall TInnerH_Many::Test_T2()
{
}

void __fastcall TInnerH_Many::Test_T3()
{
}

void __fastcall TInnerH_Many::Test_T4()
{
}

void __fastcall TInnerH_Many::Test_T5()
{
}

class TInnerH_First : public TTestGroupBase
{
public:
    __fastcall TInnerH_First()
        : TTestGroupBase("Z_First")
    {
    }

__published:
    void __fastcall Test_Z();
};

void __fastcall TInnerH_First::Test_Z()
{
}


/////////////////////////////////////////////////////////////////////////////
// TInnerH_Dotted: a group whose name contains '.'
/////////////////////////////////////////////////////////////////////////////
class TInnerH_Dotted : public TTestGroupBase
{
public:
    __fastcall TInnerH_Dotted()
        : TTestGroupBase("Dot.Group")
    {
    }

__published:
    void __fastcall Test_Name();
};

void __fastcall TInnerH_Dotted::Test_Name()
{
}


/////////////////////////////////////////////////////////////////////////////
// TInnerH_Hangs: a test that never ends
/////////////////////////////////////////////////////////////////////////////
class TInnerH_Hangs : public TTestGroupBase
{
public:
    __fastcall TInnerH_Hangs()
        : TTestGroupBase("H_Hangs")
    {
    }

__published:
    void __fastcall Test_Hangs();
};

void __fastcall TInnerH_Hangs::Test_Hangs()
{
    // For good: the abandoned thread never wakes up to touch the group, which is freed while it sleeps.
    ::Sleep(INFINITE);
}


/////////////////////////////////////////////////////////////////////////////
// TInnerH_Overflows: a stack overflow
/////////////////////////////////////////////////////////////////////////////
class TInnerH_Overflows : public TTestGroupBase
{
public:
    __fastcall TInnerH_Overflows()
        : TTestGroupBase("O_Overflows")
    {
    }

__published:
    void __fastcall Test_Overflows();
};

void __fastcall TInnerH_Overflows::Test_Overflows()
{
    Recurse(0);
}


/////////////////////////////////////////////////////////////////////////////
// TInnerH_ConstructorThrows, TInnerH_Duplicate, TInnerH_Fatal
/////////////////////////////////////////////////////////////////////////////
class TInnerH_ConstructorThrows : public TTestGroupBase
{
public:
    __fastcall TInnerH_ConstructorThrows()
        : TTestGroupBase("NeverCreated")
    {
        StrToInt(L"x");
    }

__published:
    void __fastcall Test_First();
    void __fastcall Test_Second();
};

void __fastcall TInnerH_ConstructorThrows::Test_First()
{
}

void __fastcall TInnerH_ConstructorThrows::Test_Second()
{
}

class TInnerH_Duplicate : public TTestGroupBase
{
public:
    __fastcall TInnerH_Duplicate()
        : TTestGroupBase("Duplicate")
    {
    }

__published:
    void __fastcall Test_X();
    void __fastcall Test_X(int value);
};

void __fastcall TInnerH_Duplicate::Test_X()
{
}

void __fastcall TInnerH_Duplicate::Test_X(int /*value*/)
{
}

class TInnerH_Fatal : public TTestGroupBase
{
public:
    __fastcall TInnerH_Fatal()
        : TTestGroupBase("Fatal_Group")
    {
    }

protected:
    virtual void SetUp_Test(ITestCase& /*testCase*/)
    {
        throw std::runtime_error("setup failed");
    }

__published:
    void __fastcall Test_A();
};

void __fastcall TInnerH_Fatal::Test_A()
{
}


/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_Handler
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_Handler::TTest_ASWUnitTestsBCB_Handler()
    : inherited("ASWUnitTestsBCB_Handler_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_CanonicalGroupOrder_KeepsSuitesTogether()
{
    // Arrange: groups at the top level, in suite "S", and in suite "S.T", plus a group named "T" in "S" beside suite
    // "S.T", given out of order.
    std::vector<TTestGroupOrderKey> groups;
    groups.push_back(TTestGroupOrderKey("", "Zed", 0));     // 0
    groups.push_back(TTestGroupOrderKey("S", "G2", 0));     // 1
    groups.push_back(TTestGroupOrderKey("", "Alpha", 0));   // 2
    groups.push_back(TTestGroupOrderKey("S.T", "G3", 0));   // 3
    groups.push_back(TTestGroupOrderKey("S", "G1", 0));     // 4
    groups.push_back(TTestGroupOrderKey("", "Early", -1));  // 5
    groups.push_back(TTestGroupOrderKey("S", "Late", 1));   // 6
    groups.push_back(TTestGroupOrderKey("S", "T", 0));      // 7
    groups.push_back(TTestGroupOrderKey("S", "First", -5)); // 8

    // Act
    std::vector<std::size_t> const order = TTestHandler::CanonicalGroupOrder(groups);

    // Assert
    CheckEquals("5,2,8,4,1,7,3,6,0", JoinIndices(order), ASWBCB_HERE,
        "Early (order -1) first, then Alpha, then all of suite S before Zed; within S, First (order -5), G1, G2, "
        "group T before suite T's G3, then Late (order 1). An order only sorts a group among its siblings, so First "
        "doesn't move S ahead of Alpha");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_CanonicalGroupOrder_RejectsInvalidSuitePath()
{
    CheckThrows<TExceptInvalidSuitePath>(TCanonicalOrderOfInvalidPath(), ASWBCB_HERE,
        "a suite path with an empty suite name", "Suite path \"S..T\" has an empty suite name");
    CheckThrows<TExceptInvalidSuitePath>(TShuffledOrderOfInvalidPath(), ASWBCB_HERE,
        "ShuffledGroupOrder() checks suite paths the same way", "Suite path \"S.\" has an empty suite name");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_CanonicalGroupOrder_WithoutSuites_SortsByOrderThenName()
{
    // Arrange
    std::vector<TTestGroupOrderKey> groups;
    groups.push_back(TTestGroupOrderKey("", "Bravo", 0));    // 0
    groups.push_back(TTestGroupOrderKey("", "Alpha", 0));    // 1
    groups.push_back(TTestGroupOrderKey("", "Charlie", -1)); // 2
    groups.push_back(TTestGroupOrderKey("", "Alpha", 0));    // 3
    groups.push_back(TTestGroupOrderKey("", "Delta", 1));    // 4

    // Act
    std::vector<std::size_t> const order = TTestHandler::CanonicalGroupOrder(groups);

    // Assert
    CheckEquals("2,1,3,0,4", JoinIndices(order), ASWBCB_HERE,
        "by order, then name, with tied groups kept in registration order, as before suites existed");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_GetAllTestFullNames()
{
    // Arrange
    TTestGroupRegistrar const b(CreateTestGroup<TInnerH_B>, __classid(TInnerH_B), "TInnerH_B", "", 0, nullptr);
    TTestGroupRegistrar const first(CreateTestGroup<TInnerH_First>, __classid(TInnerH_First), "TInnerH_First", "", -1,
        &b);
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, &first);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);

    // Act
    std::vector<std::string> const names = handler.GetAllTestFullNames();

    // Assert: in the order the tests run
    std::string joined;

    for (std::size_t i = 0; i < names.size(); ++i)
        joined += names[i] + ";";

    CheckEquals("Z_First.Z;A_Group.One;B_Group.Pass;B_Group.Fail;", joined, ASWBCB_HERE);
    CheckEquals("", observer.Events, ASWBCB_HERE, "nothing ran");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_GetTests_NamesApart()
{
    // Arrange
    TTestGroupRegistrar const b(CreateTestGroup<TInnerH_B>, __classid(TInnerH_B), "TInnerH_B", "", 0, nullptr);
    TTestGroupRegistrar const dotted(CreateTestGroup<TInnerH_Dotted>, __classid(TInnerH_Dotted), "TInnerH_Dotted", "",
        0, &b);
    TTestGroupRegistrar const first(CreateTestGroup<TInnerH_First>, __classid(TInnerH_First), "TInnerH_First", "", -1,
        &dotted);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &first);

    // Act
    std::vector<TTestId> const tests = handler.GetTests();

    // Assert: in the order the tests run, the same as GetAllTestFullNames()
    std::string joined;

    for (std::size_t i = 0; i < tests.size(); ++i)
        joined += "[" + tests[i].GroupName + "][" + tests[i].TestName + "]";

    CheckEquals("[Z_First][Z][B_Group][Pass][B_Group][Fail][Dot.Group][Name]", joined, ASWBCB_HERE);

    std::vector<std::string> const names = handler.GetAllTestFullNames();
    AssertEquals(tests.size(), names.size(), ASWBCB_HERE);

    for (std::size_t i = 0; i < tests.size(); ++i)
        CheckEquals(tests[i].GroupName + "." + tests[i].TestName, names[i], ASWBCB_HERE);

    CheckEquals("", observer.Events, ASWBCB_HERE, "nothing ran");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Initialize_ConstructorThrows_FailsItsTests()
{
    // Arrange
    TTestGroupRegistrar const throws(CreateTestGroup<TInnerH_ConstructorThrows>, __classid(TInnerH_ConstructorThrows),
        "TInnerH_ConstructorThrows", "", 0, nullptr);
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, &throws);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    std::string const failure = "Test group class \"TInnerH_ConstructorThrows\" could not be created: EConvertError: "
        "'x' is not a valid integer value";

    // Act
    handler.Initialize("Inner", &a);
    TTestResults const results = handler.Run();

    // Assert
    CheckTrue(Contains(observer.LogText, "!!ERROR!!: " + failure + "\n"), ASWBCB_HERE, "logged at Initialize()");
    CheckTrue(Contains(observer.LogText, "Test groups registered: 2\nTotal tests registered: 3\n"), ASWBCB_HERE,
        "its tests are still counted, from its class");
    CheckEquals("S:A_Group.One;F:A_Group.One=pass;S:TInnerH_ConstructorThrows.First;"
        "F:TInnerH_ConstructorThrows.First=fail;S:TInnerH_ConstructorThrows.Second;"
        "F:TInnerH_ConstructorThrows.Second=fail;", observer.Events, ASWBCB_HERE,
        "named after its class, and run in that name's place");
    AssertEquals(3u, static_cast<unsigned int>(results.CaseRecords.size()), ASWBCB_HERE);
    CheckEquals(failure, results.CaseRecords[1].Message, ASWBCB_HERE);
    CheckEquals(failure, results.CaseRecords[2].Message, ASWBCB_HERE);
    CheckEquals(2u, results.FailedCount, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Initialize_DuplicateTestNameThrows()
{
    // Arrange
    TTestGroupRegistrar const duplicate(CreateTestGroup<TInnerH_Duplicate>, __classid(TInnerH_Duplicate),
        "TInnerH_Duplicate", "", 0, nullptr);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);

    // Act and assert: stops the run before any test runs
    SetExceptionExpected<TExceptDuplicateTestName>(ASWBCB_HERE, "", "\"TInnerH_Duplicate\" publishes more than one "
        "method named \"Test_X\"");
    handler.Initialize("Inner", &duplicate);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Initialize_LogLines()
{
    // Arrange
    TTestGroupRegistrar const b(CreateTestGroup<TInnerH_B>, __classid(TInnerH_B), "TInnerH_B", "", 0, nullptr);
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, &b);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer); // Before Initialize(), so its output reaches the observer too

    // Act
    handler.Initialize("InnerProject", &a);

    // Assert
    CheckEquals("ASWUnitTestsBCB - Version " ASWUNITTESTSBCB_VERSION_STRING "\nInitializing test handler and "
        "registering test groups for InnerProject...\nTest groups registered: 2\nTotal tests registered: 3\n",
        observer.LogText, ASWBCB_HERE);
    CheckEquals("InnerProject", handler.GetProjectName(), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Initialize_NoGroups()
{
    // Arrange
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);

    // Act
    handler.Initialize("Empty", nullptr);
    TTestResults const results = handler.Run();

    // Assert
    CheckTrue(Contains(observer.LogText, "Test groups registered: 0\nTotal tests registered: 0\n"), ASWBCB_HERE);
    CheckTrue(Contains(observer.LogText, "] Tests done: Totals: succeeded: 0, failed: 0, skipped: 0\n"), ASWBCB_HERE);
    CheckEquals(0u, static_cast<unsigned int>(results.CaseRecords.size()), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Initialize_OrderThenName()
{
    // Arrange: a list in no particular order
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, nullptr);
    TTestGroupRegistrar const first(CreateTestGroup<TInnerH_First>, __classid(TInnerH_First), "TInnerH_First", "", -1,
        &a);
    TTestGroupRegistrar const b(CreateTestGroup<TInnerH_B>, __classid(TInnerH_B), "TInnerH_B", "", 0, &first);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);

    // Act
    handler.Initialize("Inner", &b);
    handler.Run();

    // Assert
    CheckEquals("S:Z_First.Z;F:Z_First.Z=pass;S:A_Group.One;F:A_Group.One=pass;S:B_Group.Pass;F:B_Group.Pass=pass;"
        "S:B_Group.Fail;F:B_Group.Fail=fail;", observer.Events, ASWBCB_HERE,
        "the ordered group first, then the rest alphabetically by group name");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_ListTests_Filtered()
{
    // Arrange
    TTestGroupRegistrar const b(CreateTestGroup<TInnerH_B>, __classid(TInnerH_B), "TInnerH_B", "", 0, nullptr);
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, &b);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);
    observer.LogText.clear();
    TNamesFilter const filter("B_Group.Fail");

    // Act
    handler.ListTests(&filter, "the description");

    // Assert: the same lines as asw-unit-tests
    CheckEquals("Listing tests, filtered: the description\nB_Group.Fail\nListed 1 of 3 registered test(s), in 1 of 2 "
        "registered group(s), matching the filter.\n", observer.LogText, ASWBCB_HERE);
    CheckEquals("", observer.Events, ASWBCB_HERE, "nothing ran");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_ListTests_Unfiltered()
{
    // Arrange
    TTestGroupRegistrar const b(CreateTestGroup<TInnerH_B>, __classid(TInnerH_B), "TInnerH_B", "", 0, nullptr);
    TTestGroupRegistrar const first(CreateTestGroup<TInnerH_First>, __classid(TInnerH_First), "TInnerH_First", "", -1,
        &b);
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, &first);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);
    observer.LogText.clear();

    // Act
    handler.ListTests();

    // Assert: in the order the tests run
    CheckEquals("Listing tests, unfiltered.\nZ_First.Z\nA_Group.One\nB_Group.Pass\nB_Group.Fail\nListed 4 test(s) in "
        "3 group(s).\n", observer.LogText, ASWBCB_HERE);
    CheckEquals("", observer.Events, ASWBCB_HERE, "nothing ran");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_PatternTestFilter()
{
    TPatternTestFilter const caseSensitive("*Parse?rg*", false);
    TPatternTestFilter const ignoreCase("*Parse?rg*", true);

    CheckTrue(caseSensitive.Matches("CLI.ParseArguments_Help"), ASWBCB_HERE);
    CheckFalse(caseSensitive.Matches("CLI.parseArguments_Help"), ASWBCB_HERE, "case-sensitive");
    CheckFalse(caseSensitive.Matches("CLI.ParseOptions"), ASWBCB_HERE);
    CheckTrue(ignoreCase.Matches("CLI.parseARGUMENTS_Help"), ASWBCB_HERE, "case-insensitive");
    CheckFalse(ignoreCase.Matches("CLI.ParseOptions"), ASWBCB_HERE);
    CheckFalse(TPatternTestFilter("Group", false).Matches("Group.Test"), ASWBCB_HERE, "the whole name must match");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Registry_HasThisProgramsGroups()
{
    TTestGroupRegistrar const* found = nullptr;

    for (TTestGroupRegistrar const* registrar = TTestGroupRegistrar::First(); registrar != nullptr;
         registrar = registrar->Next)
    {
        if (registrar->GroupClass == __classid(TTest_ASWUnitTestsBCB_Handler))
            found = registrar;
    }

    AssertTrue(found != nullptr, ASWBCB_HERE, "ASWBCB_REGISTER_TEST_GROUP registered this group");
    CheckEquals("ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_Handler", found->GroupClassName, ASWBCB_HERE,
        "as written in the macro");
    CheckEquals(0, found->Order, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Run_CatchCrashes_StackOverflowEndsTheRun()
{
    // Arrange: Z_First, then O_Overflows, then A_Group
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 1, nullptr);
    TTestGroupRegistrar const overflows(CreateTestGroup<TInnerH_Overflows>, __classid(TInnerH_Overflows),
        "TInnerH_Overflows", "", 0, &a);
    TTestGroupRegistrar const first(CreateTestGroup<TInnerH_First>, __classid(TInnerH_First), "TInnerH_First", "", -1,
        &overflows);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &first);

    // Act: on worker threads under a timeout, so this thread's stack doesn't overflow.
    TTestResults const results = handler.Run(nullptr, std::string(), false, TOptional<unsigned int>(), 30u, true);

    // Assert
    CheckTrue(results.Crashed, ASWBCB_HERE);
    CheckFalse(results.TimedOut, ASWBCB_HERE);
    CheckEquals("S:Z_First.Z;F:Z_First.Z=pass;S:O_Overflows.Overflows;F:O_Overflows.Overflows=fail;", observer.Events,
        ASWBCB_HERE, "the group after it didn't run");
    CheckEquals(1u, results.SuccessCount, ASWBCB_HERE);
    CheckEquals(1u, results.FailedCount, ASWBCB_HERE, "the crashed test is counted");
    AssertEquals(2u, static_cast<unsigned int>(results.CaseRecords.size()), ASWBCB_HERE);
    CheckEquals("Test crashed: stack overflow.", results.CaseRecords[1].Message, ASWBCB_HERE, "and recorded");
    CheckTrue(Contains(observer.LogText, "Tearing down group: \"O_Overflows\"\n"), ASWBCB_HERE,
        "and its group torn down");
    CheckFalse(Contains(observer.LogText, "\"A_Group\""), ASWBCB_HERE, "the next group was never set up");
    CheckTrue(Contains(observer.LogText, "Tests done: Totals: succeeded: 1, failed: 1, skipped: 0\n"), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Run_CombinesResults()
{
    // Arrange
    TTestGroupRegistrar const b(CreateTestGroup<TInnerH_B>, __classid(TInnerH_B), "TInnerH_B", "", 0, nullptr);
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, &b);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);

    // Act
    TTestResults const results = handler.Run();

    // Assert
    CheckEquals(2u, results.SuccessCount, ASWBCB_HERE);
    CheckEquals(1u, results.FailedCount, ASWBCB_HERE);
    CheckEquals(0u, results.SkippedCount, ASWBCB_HERE);
    AssertEquals(3u, static_cast<unsigned int>(results.CaseRecords.size()), ASWBCB_HERE);
    CheckEquals("A_Group", results.CaseRecords[0].GroupName, ASWBCB_HERE);
    CheckEquals("Fail", results.CaseRecords[2].TestName, ASWBCB_HERE);
    CheckEquals(2u, static_cast<unsigned int>(results.Messages.size()), ASWBCB_HERE,
        "the check failure and the failed test");
    CheckFalse(results.Stopped, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Run_FatalErrorEndsTheRun()
{
    // Arrange
    TTestGroupRegistrar const fatal(CreateTestGroup<TInnerH_Fatal>, __classid(TInnerH_Fatal), "TInnerH_Fatal", "", 0,
        nullptr);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &fatal);

    // Act and assert
    SetExceptionExpected<TExceptFatalError>(ASWBCB_HERE, "", "SetUp_Test() exception for \"Fatal_Group.A\": setup "
        "failed");
    handler.Run();
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Run_Filter_MatchesNothing()
{
    // Arrange
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, nullptr);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);
    observer.LogText.clear();
    TNamesFilter const filter("A_Group.Missing");

    // Act
    TTestResults const results = handler.Run(&filter, "\"A_Group.Missing\"");

    // Assert
    std::string const& log = observer.LogText;
    CheckEquals("", observer.Events, ASWBCB_HERE);
    CheckEquals(0u, static_cast<unsigned int>(results.CaseRecords.size()), ASWBCB_HERE);
    CheckTrue(Contains(log, "Filter: \"A_Group.Missing\"\nNo registered tests matched the filter.\nShuffle: "
        "disabled\n"), ASWBCB_HERE, log);
    CheckFalse(Contains(log, "Setting up group"), ASWBCB_HERE);
    CheckTrue(Contains(log, "] Tests done: Totals: succeeded: 0, failed: 0, skipped: 0\n"), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Run_Filter_OnlyMatchingTests()
{
    // Arrange
    TTestGroupRegistrar const b(CreateTestGroup<TInnerH_B>, __classid(TInnerH_B), "TInnerH_B", "", 0, nullptr);
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, &b);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);
    observer.LogText.clear();
    TNamesFilter const filter("B_Group.Fail");

    // Act
    TTestResults const results = handler.Run(&filter, "the description");

    // Assert
    std::string const& log = observer.LogText;
    CheckEquals("S:B_Group.Fail;F:B_Group.Fail=fail;", observer.Events, ASWBCB_HERE);
    CheckEquals(1u, results.FailedCount, ASWBCB_HERE);
    CheckEquals(0u, results.SuccessCount, ASWBCB_HERE);
    CheckTrue(Contains(log, "Filter: the description\nShuffle: disabled\n\n["), ASWBCB_HERE, log);
    CheckTrue(Contains(log, "] Setting up group 1 of 1: \"B_Group\"\nRunning 1 test.\nRunning test: B_Group.Fail\n"),
        ASWBCB_HERE, "only the groups that run are counted, and only the tests that run");
    CheckFalse(Contains(log, "A_Group"), ASWBCB_HERE, "a group without a matching test isn't set up");
    CheckFalse(Contains(log, "No registered tests matched the filter."), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Run_Filter_UncreatableGroup()
{
    // Arrange
    TTestGroupRegistrar const throws(CreateTestGroup<TInnerH_ConstructorThrows>, __classid(TInnerH_ConstructorThrows),
        "TInnerH_ConstructorThrows", "", 0, nullptr);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &throws);
    TNamesFilter const filter("TInnerH_ConstructorThrows.Second");

    // Act
    TTestResults const results = handler.Run(&filter, "");

    // Assert
    CheckEquals("S:TInnerH_ConstructorThrows.Second;F:TInnerH_ConstructorThrows.Second=fail;", observer.Events,
        ASWBCB_HERE, "the stand-in group applies the filter too");
    CheckEquals(1u, results.FailedCount, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Run_LogLines()
{
    // Arrange
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, nullptr);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);
    observer.LogText.clear();

    // Act
    handler.Run();

    // Assert: the same lines as asw-unit-tests
    std::string const& log = observer.LogText;
    std::string const separator = "--------------------------------------------------------------------------------";
    CheckTrue(Contains(log, "No filter: running all tests.\nShuffle: disabled\n\n["), ASWBCB_HERE, log);
    CheckTrue(Contains(log, "] Tests started.\n\n" + separator + "\n["), ASWBCB_HERE);
    CheckTrue(Contains(log, "] Setting up group 1 of 1: \"A_Group\"\nRunning 1 test.\nRunning test: A_Group.One\n"
        "Finished test: \"A_Group.One\" - passed ("), ASWBCB_HERE);
    CheckTrue(Contains(log, "Done. Succeeded: 1, failed: 0, skipped: 0\n["), ASWBCB_HERE);
    CheckTrue(Contains(log, "] Tearing down group: \"A_Group\"\n" + separator + "\n\n["), ASWBCB_HERE);
    CheckTrue(Contains(log, "] Tests done: Totals: succeeded: 1, failed: 0, skipped: 0\nElapsed time: 00h:00m:"),
        ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Run_Shuffle_KeepsOrderWhenFiltered()
{
    // Arrange: the subset drops a whole group and one test from another, as rerunning failures would.
    TTestGroupRegistrar const many(CreateTestGroup<TInnerH_Many>, __classid(TInnerH_Many), "TInnerH_Many", "", 0,
        nullptr);
    TTestGroupRegistrar const b(CreateTestGroup<TInnerH_B>, __classid(TInnerH_B), "TInnerH_B", "", 0, &many);
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, &b);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);
    TNamesFilter const subset("B_Group.Pass;B_Group.Fail;M_Group.T1;M_Group.T2;M_Group.T4;M_Group.T5");

    // Several seeds, so the subset matching the full run's order by chance can't hide a regression.
    for (unsigned int seed = 1; seed <= 8; ++seed)
    {
        // Act
        std::string const fullOrder = RunOrder(handler.Run(nullptr, "", true, seed));
        std::string const subsetOrder = RunOrder(handler.Run(&subset, "subset", true, seed));

        // Assert: the full run's order without the tests the subset drops
        std::string expected = fullOrder;
        char const* const dropped[] = { "A_Group.One;", "M_Group.T3;" };

        for (std::size_t i = 0; i < (sizeof(dropped) / sizeof(dropped[0])); ++i)
            expected.erase(expected.find(dropped[i]), std::strlen(dropped[i]));

        CheckEquals(expected, subsetOrder, ASWBCB_HERE, "seed " + UIntToString(seed));
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Run_Shuffle_LogsTheSeed()
{
    // Arrange
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, nullptr);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);

    // Act and assert
    observer.LogText.clear();
    handler.Run(nullptr, "", true, 42u);
    CheckTrue(Contains(observer.LogText, "No filter: running all tests.\nShuffle: enabled (seed 42)\n\n["), ASWBCB_HERE,
        observer.LogText);

    observer.LogText.clear();
    handler.Run(nullptr, "", true);
    std::string::size_type const seedPos = observer.LogText.find("Shuffle: enabled (seed ");
    AssertTrue(seedPos != std::string::npos, ASWBCB_HERE, "a new seed: " + observer.LogText);
    std::string const seed = observer.LogText.substr(seedPos + 23, observer.LogText.find(')', seedPos) - seedPos - 23);
    CheckMatches(seed, "[0-9]+", ASWBCB_HERE, "a new seed, logged");

    observer.LogText.clear();
    handler.Run(nullptr, "", false, 42u);
    CheckTrue(Contains(observer.LogText, "\nShuffle: disabled\n"), ASWBCB_HERE, "a seed without shuffle does nothing");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Run_Shuffle_OrderFromTheSeed()
{
    // Arrange
    TTestGroupRegistrar const many(CreateTestGroup<TInnerH_Many>, __classid(TInnerH_Many), "TInnerH_Many", "", 0,
        nullptr);
    TTestGroupRegistrar const b(CreateTestGroup<TInnerH_B>, __classid(TInnerH_B), "TInnerH_B", "", 0, &many);
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, &b);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);

    // The groups and tests in their unshuffled order
    char const* const groupNames[] = { "A_Group", "B_Group", "M_Group" };
    std::vector<std::string> groupTests[3];
    groupTests[0].push_back("One");
    groupTests[1].push_back("Pass");
    groupTests[1].push_back("Fail");
    groupTests[2].push_back("T1");
    groupTests[2].push_back("T2");
    groupTests[2].push_back("T3");
    groupTests[2].push_back("T4");
    groupTests[2].push_back("T5");
    std::string const unshuffled = RunOrder(handler.Run());
    bool anyReordered = false;

    for (unsigned int seed = 1; seed <= 8; ++seed)
    {
        // Act
        std::string const actual = RunOrder(handler.Run(nullptr, "", true, seed));

        // Assert: the groups in ShuffledIndices()' order for the seed, and each group's tests in its order for the
        // seed plus the FNV-1a hash of the group's name, as asw-unit-tests does it
        std::string expected;
        std::vector<std::size_t> const groupOrder = ShuffledIndices(3, seed);

        for (std::size_t g = 0; g < groupOrder.size(); ++g)
        {
            std::size_t const group = groupOrder[g];
            std::vector<std::size_t> const testOrder = ShuffledIndices(groupTests[group].size(),
                seed + FNV1aHash(groupNames[group]));

            for (std::size_t t = 0; t < testOrder.size(); ++t)
                expected += std::string(groupNames[group]) + "." + groupTests[group][testOrder[t]] + ";";
        }

        CheckEquals(expected, actual, ASWBCB_HERE, "seed " + UIntToString(seed));
        anyReordered = anyReordered || (actual != unshuffled);
    }

    CheckTrue(anyReordered, ASWBCB_HERE, "some seed changes the order");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_Run_Timeout_EndsTheRun()
{
    // Arrange: Z_First, then H_Hangs, then A_Group
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 1, nullptr);
    TTestGroupRegistrar const hangs(CreateTestGroup<TInnerH_Hangs>, __classid(TInnerH_Hangs), "TInnerH_Hangs", "", 0,
        &a);
    TTestGroupRegistrar const first(CreateTestGroup<TInnerH_First>, __classid(TInnerH_First), "TInnerH_First", "", -1,
        &hangs);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &first);

    // Act
    TTestResults const results = handler.Run(nullptr, std::string(), false, TOptional<unsigned int>(), 1u);

    // Assert
    CheckTrue(results.TimedOut, ASWBCB_HERE);
    CheckFalse(results.Stopped, ASWBCB_HERE);
    CheckEquals("S:Z_First.Z;F:Z_First.Z=pass;S:H_Hangs.Hangs;F:H_Hangs.Hangs=fail;", observer.Events, ASWBCB_HERE,
        "the group after it didn't run");
    CheckEquals(1u, results.SuccessCount, ASWBCB_HERE);
    CheckEquals(1u, results.FailedCount, ASWBCB_HERE, "the timed-out test is counted");
    AssertEquals(2u, static_cast<unsigned int>(results.CaseRecords.size()), ASWBCB_HERE);
    CheckEquals("Test exceeded its 1 second timeout and was abandoned.", results.CaseRecords[1].Message,
        ASWBCB_HERE, "and recorded");
    CheckTrue(Contains(observer.LogText, "Tearing down group: \"H_Hangs\"\n"), ASWBCB_HERE, "and its group torn down");
    CheckFalse(Contains(observer.LogText, "\"A_Group\""), ASWBCB_HERE, "the next group was never set up");
    CheckTrue(Contains(observer.LogText, "Tests done: Totals: succeeded: 1, failed: 1, skipped: 0\n"), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_SetRunObserver_StopsBetweenGroups()
{
    // Arrange
    TTestGroupRegistrar const b(CreateTestGroup<TInnerH_B>, __classid(TInnerH_B), "TInnerH_B", "", 0, nullptr);
    TTestGroupRegistrar const a(CreateTestGroup<TInnerH_A>, __classid(TInnerH_A), "TInnerH_A", "", 0, &b);
    TTestHandler handler;
    TRecordingObserver observer;
    observer.StopAfterStarted = 1; // Exactly A_Group's one test, then stop
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);

    // Act
    TTestResults const results = handler.Run();

    // Assert
    CheckTrue(results.Stopped, ASWBCB_HERE);
    CheckEquals(1u, static_cast<unsigned int>(results.CaseRecords.size()), ASWBCB_HERE, "only the first group ran");
    CheckFalse(Contains(observer.LogText, "\"B_Group\""), ASWBCB_HERE, "the second group was never set up");
    CheckTrue(Contains(observer.LogText, "Run stopped on request before every test ran.\n"), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_ShuffledGroupOrder_KeepsSuitesTogether()
{
    // Arrange: groups in canonical order, with suite "A" (holding suite "A.B") and suite "C" among top-level ones.
    char const* const suitePathList[] = { "", "A", "A", "A.B", "A.B", "A", "", "C", "C" };
    std::vector<std::string> const suitePaths(suitePathList, suitePathList + 9);
    char const* const suites[] = { "A", "A.B", "C" };
    std::vector<std::string> const oneSuite(5, "A");
    bool anyReordered = false;

    for (unsigned int seed = 1; seed <= 20; ++seed)
    {
        std::string const seedText = " (seed " + UIntToString(seed) + ")";

        // Act
        std::vector<std::size_t> const order = TTestHandler::ShuffledGroupOrder(suitePaths, seed);

        // Assert
        std::vector<bool> seen(suitePaths.size(), false);

        for (std::size_t position = 0; position < order.size(); ++position)
        {
            if (order[position] < seen.size())
                seen[order[position]] = true;
        }

        CheckEquals(suitePaths.size(), order.size(), ASWBCB_HERE, "one index per group" + seedText);
        CheckEquals(0u, static_cast<unsigned int>(std::count(seen.begin(), seen.end(), false)), ASWBCB_HERE,
            "every group appears exactly once" + seedText);
        CheckEquals(JoinIndices(order), JoinIndices(TTestHandler::ShuffledGroupOrder(suitePaths, seed)), ASWBCB_HERE,
            "the same seed gives the same order" + seedText);

        for (std::size_t s = 0; s < (sizeof(suites) / sizeof(suites[0])); ++s)
        {
            std::string const suite = suites[s];
            std::size_t first = order.size();
            std::size_t last = 0;
            std::size_t count = 0;

            for (std::size_t position = 0; position < order.size(); ++position)
            {
                std::string const& path = suitePaths[order[position]];

                if ((path == suite) || (path.compare(0, suite.size() + 1, suite + ".") == 0))
                {
                    first = (position < first) ? position : first;
                    last = (position > last) ? position : last;
                    ++count;
                }
            }

            CheckEquals(count, last - first + 1, ASWBCB_HERE, "suite " + suite + "'s groups run together" + seedText);
        }

        if (JoinIndices(order) != "0,1,2,3,4,5,6,7,8")
            anyReordered = true;

        CheckEquals(JoinIndices(ShuffledIndices(oneSuite.size(), seed + FNV1aHash("A"))),
            JoinIndices(TTestHandler::ShuffledGroupOrder(oneSuite, seed)), ASWBCB_HERE,
            "a suite's groups are shuffled with the seed plus the FNV-1a hash of its path" + seedText);
    }

    CheckTrue(anyReordered, ASWBCB_HERE, "some seed actually shuffles the groups");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_ShuffledGroupOrder_WithoutSuites_MatchesShuffledIndices()
{
    std::vector<std::string> const suitePaths(7, std::string());

    for (unsigned int seed = 1; seed <= 8; ++seed)
    {
        CheckEquals(JoinIndices(ShuffledIndices(suitePaths.size(), seed)),
            JoinIndices(TTestHandler::ShuffledGroupOrder(suitePaths, seed)), ASWBCB_HERE,
            "without suites, a seed gives the same group order as before suites existed (seed " +
            UIntToString(seed) + ")");
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Handler::Test_WildcardMatch()
{
    CheckTrue(TTestHandler::WildcardMatch("Group.Test", "Group.Test"), ASWBCB_HERE, "no wildcards");
    CheckFalse(TTestHandler::WildcardMatch("Group.Test", "Group.Tests"), ASWBCB_HERE, "the whole text must match");
    CheckFalse(TTestHandler::WildcardMatch("Group.Tests", "Group.Test"), ASWBCB_HERE, "the whole pattern too");
    CheckTrue(TTestHandler::WildcardMatch("", ""), ASWBCB_HERE, "empty");
    CheckFalse(TTestHandler::WildcardMatch("", "a"), ASWBCB_HERE, "empty pattern");

    CheckTrue(TTestHandler::WildcardMatch("*", ""), ASWBCB_HERE, "'*' matches nothing");
    CheckTrue(TTestHandler::WildcardMatch("*", "Group.Test"), ASWBCB_HERE, "'*' matches everything");
    CheckTrue(TTestHandler::WildcardMatch("*Str*", "Group.String_Test"), ASWBCB_HERE, "a substring");
    CheckTrue(TTestHandler::WildcardMatch("a*b*c", "aXXbYYc"), ASWBCB_HERE);
    CheckTrue(TTestHandler::WildcardMatch("*ab", "aab"), ASWBCB_HERE, "going back after a partial match");
    CheckTrue(TTestHandler::WildcardMatch("*a*b", "xaybzb"), ASWBCB_HERE, "the last '*' takes more");
    CheckFalse(TTestHandler::WildcardMatch("*a*b", "xaybzc"), ASWBCB_HERE);
    CheckTrue(TTestHandler::WildcardMatch("a**", "a"), ASWBCB_HERE, "trailing stars");
    CheckTrue(TTestHandler::WildcardMatch("*", "*abc"), ASWBCB_HERE, "a '*' in the text");
    CheckTrue(TTestHandler::WildcardMatch("*c", "*abc"), ASWBCB_HERE, "a '*' in the text, then more");

    CheckTrue(TTestHandler::WildcardMatch("a?c", "abc"), ASWBCB_HERE, "'?' matches one character");
    CheckFalse(TTestHandler::WildcardMatch("a?c", "ac"), ASWBCB_HERE, "'?' doesn't match nothing");
    CheckFalse(TTestHandler::WildcardMatch("?", ""), ASWBCB_HERE);

    CheckFalse(TTestHandler::WildcardMatch("group.*", "Group.Test"), ASWBCB_HERE, "case-sensitive by default");
    CheckTrue(TTestHandler::WildcardMatch("group.*", "Group.Test", true), ASWBCB_HERE, "ignoring case");
    CheckTrue(TTestHandler::WildcardMatch("GROUP.T?ST", "group.test", true), ASWBCB_HERE, "ignoring case");
    CheckFalse(TTestHandler::WildcardMatch("[", "{", true), ASWBCB_HERE, "only A-Z: '[' isn't '{' lowercased");
    CheckFalse(TTestHandler::WildcardMatch("caf\xC3\x89", "caf\xC3\xA9", true), ASWBCB_HERE, "only ASCII letters");
    CheckTrue(TTestHandler::WildcardMatch("caf\xC3\xA9", "caf\xC3\xA9"), ASWBCB_HERE, "UTF-8 bytes as they are");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_Handler, "SelfTests")
