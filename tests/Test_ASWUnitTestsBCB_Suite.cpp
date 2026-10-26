/* **************************************************************************
Test_ASWUnitTestsBCB_Suite.cpp
Author: Anthony S. West - ASW Software

Tests for test suites: TSuitePath, a group's suite path and qualified name, suites named by a group's registration,
and how suites reach records, filters, observers, the run order and the handler.

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
#include "Test_ASWUnitTestsBCB_Suite.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <memory>
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
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TRecordingObserver
//
// Records what a group or handler tells its observer, which also keeps the
// inner groups' output off the console.
/////////////////////////////////////////////////////////////////////////////
class TRecordingObserver : public ASWUnitTestsBCB::ITestRunObserver
{
public:
    std::string Events; // "S:Group.Test;" when a test starts, "F:Group.Test@SuitePath;" when it finishes
    std::string LogText;

public:
    virtual void OnLog(std::string const& text)
    {
        LogText += text;
    }

    virtual void OnTestFinished(ASWUnitTestsBCB::TTestCaseRecord const& record)
    {
        Events += "F:" + record.GroupName + "." + record.TestName + "@" + record.SuitePath + ";";
    }

    virtual void OnTestStarted(std::string const& groupName, std::string const& testName)
    {
        Events += "S:" + groupName + "." + testName + ";";
    }

    virtual bool StopRequested()
    {
        return false;
    }
};

//---------------------------------------------------------------------------

bool Contains(std::string const& text, std::string const& part);
std::string JoinNames(std::vector<std::string> const& names);
std::string RunOrder(ASWUnitTestsBCB::TTestResults const& results);

//---------------------------------------------------------------------------
bool Contains(std::string const& text, std::string const& part)
{
    return text.find(part) != std::string::npos;
}
//---------------------------------------------------------------------------
/*
    JoinNames

    'names' joined with "|", so a whole list can be compared, and shown on failure, as one string.
*/
std::string JoinNames(std::vector<std::string> const& names)
{
    std::string joined;

    for (std::size_t i = 0; i < names.size(); ++i)
        joined += ((i == 0) ? std::string() : std::string("|")) + names[i];

    return joined;
}
//---------------------------------------------------------------------------
/*
    RunOrder

    Returns the tests 'results' records, in the order they ran, as "Group.Test;Group.Test;".
*/
std::string RunOrder(ASWUnitTestsBCB::TTestResults const& results)
{
    std::string order;

    for (std::size_t i = 0; i < results.CaseRecords.size(); ++i)
        order += results.CaseRecords[i].GroupName + "." + results.CaseRecords[i].TestName + ";";

    return order;
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

// The inner groups the tests below run. None is registered, so the program's own run never includes them.

/////////////////////////////////////////////////////////////////////////////
// TInnerS_Plain, TInnerS_ZTop: groups outside any suite
/////////////////////////////////////////////////////////////////////////////
class TInnerS_Plain : public TTestGroupBase
{
public:
    __fastcall TInnerS_Plain()
        : TTestGroupBase("S_Plain")
    {
    }

__published:
    void __fastcall Test_One();
};

void __fastcall TInnerS_Plain::Test_One()
{
}

class TInnerS_ZTop : public TTestGroupBase
{
public:
    __fastcall TInnerS_ZTop()
        : TTestGroupBase("Z_Top")
    {
    }

__published:
    void __fastcall Test_One();
};

void __fastcall TInnerS_ZTop::Test_One()
{
}


/////////////////////////////////////////////////////////////////////////////
// TInnerS_Nested: in suite "Inner" inside suite "Outer", with a test that fails
/////////////////////////////////////////////////////////////////////////////
class TInnerS_Nested : public TTestGroupBase
{
public:
    __fastcall TInnerS_Nested()
        : TTestGroupBase(TSuitePath("Outer.Inner"), "S_Nested")
    {
    }

__published:
    void __fastcall Test_Alpha();
    void __fastcall Test_Beta();
};

void __fastcall TInnerS_Nested::Test_Alpha()
{
}

void __fastcall TInnerS_Nested::Test_Beta()
{
    CheckTrue(false, ASWBCB_HERE, "fails");
}


/////////////////////////////////////////////////////////////////////////////
// TInnerS_InOuter: in suite "Outer", by its constructor
/////////////////////////////////////////////////////////////////////////////
class TInnerS_InOuter : public TTestGroupBase
{
public:
    __fastcall TInnerS_InOuter()
        : TTestGroupBase(TSuitePath("Outer"), "S_InOuter")
    {
    }

__published:
    void __fastcall Test_One();
};

void __fastcall TInnerS_InOuter::Test_One()
{
}


/////////////////////////////////////////////////////////////////////////////
// TInnerS_Unsuited: no suite of its own; the tests register it in one
/////////////////////////////////////////////////////////////////////////////
class TInnerS_Unsuited : public TTestGroupBase
{
public:
    __fastcall TInnerS_Unsuited()
        : TTestGroupBase("S_Registered")
    {
    }

__published:
    void __fastcall Test_One();
};

void __fastcall TInnerS_Unsuited::Test_One()
{
}


/////////////////////////////////////////////////////////////////////////////
// TInnerS_ConstructorThrows, TInnerS_InvalidPath, TInnerS_Duplicate
/////////////////////////////////////////////////////////////////////////////
class TInnerS_ConstructorThrows : public TTestGroupBase
{
public:
    __fastcall TInnerS_ConstructorThrows()
        : TTestGroupBase(TSuitePath("Outer"), "NeverCreated")
    {
        StrToInt(L"x");
    }

__published:
    void __fastcall Test_First();
};

void __fastcall TInnerS_ConstructorThrows::Test_First()
{
}

class TInnerS_InvalidPath : public TTestGroupBase
{
public:
    __fastcall TInnerS_InvalidPath()
        : TTestGroupBase(TSuitePath("Outer..Inner"), "S_InvalidPath")
    {
    }

__published:
    void __fastcall Test_One();
};

void __fastcall TInnerS_InvalidPath::Test_One()
{
}

std::string InnerS_SameName(int const& /*row*/)
{
    return "same";
}

class TInnerS_Duplicate : public TTestGroupBase
{
public:
    __fastcall TInnerS_Duplicate()
        : TTestGroupBase(TSuitePath("Outer"), "S_Duplicate")
    {
        std::vector<int> rows(2, 0);
        RegisterTestCases(Test_Row, "Row", rows, InnerS_SameName);
    }

    void __fastcall Test_Row(int const& /*row*/)
    {
    }
};


/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_Suite
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_Suite::TTest_ASWUnitTestsBCB_Suite()
    : inherited("ASWUnitTestsBCB_Suite_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_Constructor_InvalidSuitePathKept()
{
    // Act
    std::unique_ptr<TInnerS_InvalidPath> const group(new TInnerS_InvalidPath());

    // Assert
    CheckEquals("Outer..Inner", group->GetSuitePath(), ASWBCB_HERE,
        "kept as given, for TTestHandler::Initialize() to reject (see TSuitePath)");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_DuplicateName_ReportsQualifiedName()
{
    // Arrange
    std::unique_ptr<TInnerS_Duplicate> group(new TInnerS_Duplicate());
    std::string message;

    // Act
    try
    {
        group->GetTestCallbackList();
    }
    catch (TExceptDuplicateTestName const& duplicateEx)
    {
        message = duplicateEx.what();
    }

    // Assert
    CheckEquals("Test group \"Outer.S_Duplicate\" registers more than one test named \"Row[same]\"", message,
        ASWBCB_HERE, "the group is named by its qualified name");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_GroupInSuite_FilterMatchesQualifiedName()
{
    // Arrange
    std::unique_ptr<TInnerS_Nested> byFullName(new TInnerS_Nested());
    std::unique_ptr<TInnerS_Nested> bySuite(new TInnerS_Nested());
    std::unique_ptr<TInnerS_Nested> byOwnName(new TInnerS_Nested());
    TRecordingObserver observer;
    byFullName->SetRunObserver(&observer);
    bySuite->SetRunObserver(&observer);
    byOwnName->SetRunObserver(&observer);
    TPatternTestFilter const fullName("Outer.Inner.S_Nested.Beta", false);
    TPatternTestFilter const suite("Outer.*", false);
    TPatternTestFilter const ownName("S_Nested.*", false);

    // Act
    byFullName->Run(&fullName);
    bySuite->Run(&suite);
    byOwnName->Run(&ownName);

    // Assert
    CheckEquals("Outer.Inner.S_Nested.Beta;", RunOrder(byFullName->Results()), ASWBCB_HERE,
        "a test's full name starts with its suite path");
    CheckEquals("Outer.Inner.S_Nested.Alpha;Outer.Inner.S_Nested.Beta;", RunOrder(bySuite->Results()), ASWBCB_HERE,
        "a pattern on the outer suite selects every test in it");
    CheckEquals("", RunOrder(byOwnName->Results()), ASWBCB_HERE,
        "the group's own name without its suite path doesn't match");
    CheckEquals(1u, byFullName->CountTests(&fullName), ASWBCB_HERE, "CountTests() matches the full name too");
    CheckEquals(0u, byOwnName->CountTests(&ownName), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_GroupInSuite_ReportsQualifiedName()
{
    // Arrange
    std::unique_ptr<TInnerS_Nested> group(new TInnerS_Nested());
    TRecordingObserver observer;
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    CheckEquals("S_Nested", group->GetTestGroupName(), ASWBCB_HERE, "GetTestGroupName() is still the group's own name");
    CheckEquals("Outer.Inner", group->GetSuitePath(), ASWBCB_HERE, "the path it was constructed with");
    CheckEquals("Outer.Inner.S_Nested", group->GetQualifiedTestGroupName(), ASWBCB_HERE,
        "the suite path, then the group's own name");
    CheckEquals("S:Outer.Inner.S_Nested.Alpha;F:Outer.Inner.S_Nested.Alpha@Outer.Inner;"
        "S:Outer.Inner.S_Nested.Beta;F:Outer.Inner.S_Nested.Beta@Outer.Inner;", observer.Events, ASWBCB_HERE,
        "OnTestStarted() and the records name the group by its qualified name, and the records carry the suite path");
    CheckTrue(Contains(observer.LogText, "Running test: Outer.Inner.S_Nested.Alpha\n"), ASWBCB_HERE, observer.LogText);
    CheckTrue(Contains(observer.LogText, "Finished test: \"Outer.Inner.S_Nested.Alpha\" - passed ("), ASWBCB_HERE);
    CheckTrue(Contains(observer.LogText, "***Test failed: \"Outer.Inner.S_Nested.Beta\""), ASWBCB_HERE);
    CheckTrue(Contains(observer.LogText, "] Tearing down group: \"Outer.Inner.S_Nested\"\n"), ASWBCB_HERE);
    AssertEquals(2u, static_cast<unsigned int>(group->Results().Messages.size()), ASWBCB_HERE,
        "the failed check, then the failed test");
    CheckStartsWith(group->Results().Messages[1], "***Test failed: \"Outer.Inner.S_Nested.Beta\"", ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_GroupWithoutSuite_KeepsItsOwnName()
{
    // Arrange
    std::unique_ptr<TInnerS_Plain> group(new TInnerS_Plain());
    TRecordingObserver observer;
    group->SetRunObserver(&observer);

    // Act
    group->Run();

    // Assert
    CheckEquals("", group->GetSuitePath(), ASWBCB_HERE, "the name-only constructor puts the group in no suite");
    CheckEquals("S_Plain", group->GetQualifiedTestGroupName(), ASWBCB_HERE,
        "without a suite, the qualified name is just the group's own name");
    CheckEquals("S:S_Plain.One;F:S_Plain.One@;", observer.Events, ASWBCB_HERE,
        "the record's group name is unchanged from before suites existed, with no suite path");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_Handler_ListsTestsByFullName()
{
    // Arrange
    TTestGroupRegistrar const plain(CreateTestGroup<TInnerS_Plain>, __classid(TInnerS_Plain), "TInnerS_Plain", "", 0,
        nullptr);
    TTestGroupRegistrar const nested(CreateTestGroup<TInnerS_Nested>, __classid(TInnerS_Nested), "TInnerS_Nested", "",
        0, &plain);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &nested);
    TPatternTestFilter const filter("Outer.*", false);

    // Act
    std::vector<TTestId> const tests = handler.GetTests();
    std::vector<std::string> const fullNames = handler.GetAllTestFullNames();
    observer.LogText.clear();
    handler.ListTests(&filter, "Outer.*");

    // Assert
    CheckEquals("Outer.Inner.S_Nested.Alpha|Outer.Inner.S_Nested.Beta|S_Plain.One", JoinNames(fullNames),
        ASWBCB_HERE, "full names start with the suite path");
    AssertEquals(3u, static_cast<unsigned int>(tests.size()), ASWBCB_HERE);
    CheckEquals("Outer.Inner.S_Nested", tests[0].GroupName, ASWBCB_HERE, "the qualified name");
    CheckEquals("Alpha", tests[0].TestName, ASWBCB_HERE);
    CheckEquals("Outer.Inner", tests[0].SuitePath, ASWBCB_HERE, "the suite path on its own");
    CheckEquals("S_Plain", tests[2].GroupName, ASWBCB_HERE);
    CheckEquals("", tests[2].SuitePath, ASWBCB_HERE);
    CheckEquals("Listing tests, filtered: Outer.*\nOuter.Inner.S_Nested.Alpha\nOuter.Inner.S_Nested.Beta\n"
        "Listed 2 of 3 registered test(s), in 1 of 2 registered group(s), matching the filter.\n", observer.LogText,
        ASWBCB_HERE, "--list shows a test by its full name, suite path first");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_Handler_OrderKeepsSuitesTogether()
{
    // Arrange: a list in no particular order. At the top level, S_Plain (order -1), suite Outer (as order 0, by its
    // name) and Z_Top; in Outer, suite Inner, S_Registered (registered in Outer), and S_InOuter (order 1).
    TTestGroupRegistrar const zTop(CreateTestGroup<TInnerS_ZTop>, __classid(TInnerS_ZTop), "TInnerS_ZTop", "", 0,
        nullptr);
    TTestGroupRegistrar const inOuter(CreateTestGroup<TInnerS_InOuter>, __classid(TInnerS_InOuter), "TInnerS_InOuter",
        "", 1, &zTop);
    TTestGroupRegistrar const plain(CreateTestGroup<TInnerS_Plain>, __classid(TInnerS_Plain), "TInnerS_Plain", "", -1,
        &inOuter);
    TTestGroupRegistrar const registered(CreateTestGroup<TInnerS_Unsuited>, __classid(TInnerS_Unsuited),
        "TInnerS_Unsuited", "Outer", 0, &plain);
    TTestGroupRegistrar const nested(CreateTestGroup<TInnerS_Nested>, __classid(TInnerS_Nested), "TInnerS_Nested", "",
        0, &registered);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);

    // Act
    handler.Initialize("Inner", &nested);
    TTestResults const results = handler.Run();

    // Assert
    CheckEquals("S_Plain.One;Outer.Inner.S_Nested.Alpha;Outer.Inner.S_Nested.Beta;Outer.S_Registered.One;"
        "Outer.S_InOuter.One;Z_Top.One;", RunOrder(results), ASWBCB_HERE,
        "suite Outer's groups together, in its place by name; S_InOuter's order 1 only places it in Outer");
    CheckTrue(Contains(observer.LogText, "Setting up group 2 of 5: \"Outer.Inner.S_Nested\"\n"), ASWBCB_HERE,
        "the log names the group by its qualified name");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_Handler_RunsSuiteByFilter()
{
    // Arrange
    TTestGroupRegistrar const plain(CreateTestGroup<TInnerS_Plain>, __classid(TInnerS_Plain), "TInnerS_Plain", "", 0,
        nullptr);
    TTestGroupRegistrar const inOuter(CreateTestGroup<TInnerS_InOuter>, __classid(TInnerS_InOuter), "TInnerS_InOuter",
        "", 0, &plain);
    TTestGroupRegistrar const nested(CreateTestGroup<TInnerS_Nested>, __classid(TInnerS_Nested), "TInnerS_Nested", "",
        0, &inOuter);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &nested);
    TPatternTestFilter const outer("Outer.*", false);
    TPatternTestFilter const inner("Outer.Inner.*", false);

    // Act
    std::string const outerOrder = RunOrder(handler.Run(&outer, "Outer.*"));
    std::string const innerOrder = RunOrder(handler.Run(&inner, "Outer.Inner.*"));

    // Assert
    CheckEquals("Outer.Inner.S_Nested.Alpha;Outer.Inner.S_Nested.Beta;Outer.S_InOuter.One;", outerOrder, ASWBCB_HERE,
        "a suite's tests, including those in the suites inside it");
    CheckEquals("Outer.Inner.S_Nested.Alpha;Outer.Inner.S_Nested.Beta;", innerOrder, ASWBCB_HERE,
        "just the inner suite's tests");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_Handler_ShuffleKeepsSuitesTogether()
{
    // Arrange: in canonical order, S_Nested (Outer.Inner), S_InOuter (Outer), S_Registered (Outer), S_Plain, Z_Top
    TTestGroupRegistrar const zTop(CreateTestGroup<TInnerS_ZTop>, __classid(TInnerS_ZTop), "TInnerS_ZTop", "", 0,
        nullptr);
    TTestGroupRegistrar const inOuter(CreateTestGroup<TInnerS_InOuter>, __classid(TInnerS_InOuter), "TInnerS_InOuter",
        "", 0, &zTop);
    TTestGroupRegistrar const plain(CreateTestGroup<TInnerS_Plain>, __classid(TInnerS_Plain), "TInnerS_Plain", "", 0,
        &inOuter);
    TTestGroupRegistrar const registered(CreateTestGroup<TInnerS_Unsuited>, __classid(TInnerS_Unsuited),
        "TInnerS_Unsuited", "Outer", 0, &plain);
    TTestGroupRegistrar const nested(CreateTestGroup<TInnerS_Nested>, __classid(TInnerS_Nested), "TInnerS_Nested", "",
        0, &registered);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &nested);

    char const* const groupNames[] = { "Outer.Inner.S_Nested", "Outer.S_InOuter", "Outer.S_Registered", "S_Plain",
                                       "Z_Top" };
    char const* const suitePathList[] = { "Outer.Inner", "Outer", "Outer", "", "" };
    std::vector<std::string> const suitePaths(suitePathList, suitePathList + 5);
    std::string const unshuffled = RunOrder(handler.Run());
    bool anyReordered = false;

    for (unsigned int seed = 1; seed <= 8; ++seed)
    {
        // Act
        std::string const actual = RunOrder(handler.Run(nullptr, "", true, seed));

        // Assert: the groups in ShuffledGroupOrder()'s order for the seed, and S_Nested's two tests in their order
        // for the seed plus the FNV-1a hash of the group's qualified name, as asw-unit-tests does it
        std::string expected;
        std::vector<std::size_t> const groupOrder = TTestHandler::ShuffledGroupOrder(suitePaths, seed);

        for (std::size_t g = 0; g < groupOrder.size(); ++g)
        {
            std::string const groupName = groupNames[groupOrder[g]];

            if (groupOrder[g] != 0)
            {
                expected += groupName + ".One;";
                continue;
            }

            char const* const nestedTests[] = { "Alpha", "Beta" };
            std::vector<std::size_t> const testOrder = ShuffledIndices(2, seed + FNV1aHash(groupName));
            expected += groupName + "." + nestedTests[testOrder[0]] + ";" + groupName + "." +
                nestedTests[testOrder[1]] + ";";
        }

        CheckEquals(expected, actual, ASWBCB_HERE, "seed " + UIntToString(seed));
        anyReordered = anyReordered || (actual != unshuffled);
    }

    CheckTrue(anyReordered, ASWBCB_HERE, "some seed changes the order");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_Initialize_ConstructorInvalidSuitePathThrows()
{
    // Arrange
    TTestGroupRegistrar const invalid(CreateTestGroup<TInnerS_InvalidPath>, __classid(TInnerS_InvalidPath),
        "TInnerS_InvalidPath", "", 0, nullptr);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);

    // Act and assert: stops the run before any test runs, rather than failing the group's tests
    SetExceptionExpected<TExceptInvalidSuitePath>(ASWBCB_HERE, "", "Suite path \"Outer..Inner\" has an empty suite "
        "name; separate its names with single '.' characters, with none at the start or end");
    handler.Initialize("Inner", &invalid);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_Initialize_RegistrationConflictingSuiteThrows()
{
    // Arrange
    TTestGroupRegistrar const conflicting(CreateTestGroup<TInnerS_InOuter>, __classid(TInnerS_InOuter),
        "TInnerS_InOuter", "Other", 0, nullptr);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);

    // Act and assert
    SetExceptionExpected<TExceptInvalidSuitePath>(ASWBCB_HERE, "", "Test group \"S_InOuter\" is registered in suite "
        "\"Other\", but its constructor puts it in suite \"Outer\"");
    handler.Initialize("Inner", &conflicting);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_Initialize_RegistrationInvalidSuitePathThrows()
{
    // Arrange
    TTestGroupRegistrar const invalid(CreateTestGroup<TInnerS_Unsuited>, __classid(TInnerS_Unsuited),
        "TInnerS_Unsuited", "Outer.", 0, nullptr);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);

    // Act and assert
    SetExceptionExpected<TExceptInvalidSuitePath>(ASWBCB_HERE, "", "Suite path \"Outer.\" has an empty suite name");
    handler.Initialize("Inner", &invalid);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_Registration_PutsGroupInSuite()
{
    // Arrange
    TTestGroupRegistrar const registered(CreateTestGroup<TInnerS_Unsuited>, __classid(TInnerS_Unsuited),
        "TInnerS_Unsuited", "Outer.Registered", 0, nullptr);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);

    // Act
    handler.Initialize("Inner", &registered);
    std::vector<TTestId> const tests = handler.GetTests();
    handler.Run();

    // Assert
    AssertEquals(1u, static_cast<unsigned int>(tests.size()), ASWBCB_HERE);
    CheckEquals("Outer.Registered.S_Registered", tests[0].GroupName, ASWBCB_HERE,
        "the registration's suite path, then the group's own name");
    CheckEquals("Outer.Registered", tests[0].SuitePath, ASWBCB_HERE);
    CheckEquals("S:Outer.Registered.S_Registered.One;F:Outer.Registered.S_Registered.One@Outer.Registered;",
        observer.Events, ASWBCB_HERE, "the group runs in the registration's suite");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_Registration_SameSuiteAsConstructor()
{
    // Arrange
    TTestGroupRegistrar const registered(CreateTestGroup<TInnerS_InOuter>, __classid(TInnerS_InOuter),
        "TInnerS_InOuter", "Outer", 0, nullptr);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);

    // Act
    handler.Initialize("Inner", &registered);
    handler.Run();

    // Assert
    CheckEquals("S:Outer.S_InOuter.One;F:Outer.S_InOuter.One@Outer;", observer.Events, ASWBCB_HERE,
        "the registration and the constructor may both name the same suite");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_Registration_UncreatableGroupStaysInSuite()
{
    // Arrange: one stand-in registered in suite Outer, one not
    TTestGroupRegistrar const unsuited(CreateTestGroup<TInnerS_ConstructorThrows>,
        __classid(TInnerS_ConstructorThrows), "TInnerS_ConstructorThrows", "", 0, nullptr);
    TTestGroupRegistrar const registered(CreateTestGroup<TInnerS_ConstructorThrows>,
        __classid(TInnerS_ConstructorThrows), "TInnerS_ConstructorThrows", "Outer", 0, &unsuited);
    TTestHandler handler;
    TRecordingObserver observer;
    handler.SetRunObserver(&observer);
    TPatternTestFilter const filter("Outer.*", false);

    // Act
    handler.Initialize("Inner", &registered);
    TTestResults const results = handler.Run(&filter, "Outer.*");

    // Assert
    CheckEquals("S:Outer.TInnerS_ConstructorThrows.First;F:Outer.TInnerS_ConstructorThrows.First@Outer;",
        observer.Events, ASWBCB_HERE,
        "the stand-in is in the registration's suite, so the suite's filter runs it; the other isn't in a suite");
    CheckEquals(1u, results.FailedCount, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_ThisGroup_IsInItsRegisteredSuite()
{
    CheckEquals("SelfTests.Nested.Suites", GetSuitePath(), ASWBCB_HERE,
        "ASWBCB_REGISTER_TEST_GROUP_IN_SUITE put this group in its suite");
    CheckEquals("SelfTests.Nested.Suites.ASWUnitTestsBCB_Suite_Tests", GetQualifiedTestGroupName(), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_TSuitePath_EmptyNamesAreInvalid()
{
    char const* const invalidPaths[] = { ".", "..", ".Framework", "Framework.", "Framework..Console" };

    for (std::size_t i = 0; i < (sizeof(invalidPaths) / sizeof(invalidPaths[0])); ++i)
    {
        TSuitePath const suitePath(invalidPaths[i]);

        CheckFalse(suitePath.IsValid(), ASWBCB_HERE, "suite path \"" + suitePath.GetPath() + "\" has an empty suite "
            "name");
    }

    CheckTrue(TSuitePath("").IsValid(), ASWBCB_HERE, "an empty path, for no suite");
    CheckTrue(TSuitePath("Framework.Console").IsValid(), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Suite::Test_TSuitePath_SplitsNames()
{
    // Arrange
    TSuitePath const empty("");
    TSuitePath const single("Framework");
    TSuitePath const nested("Framework.Console.Colors");
    TSuitePath const spaced("My Suite.Sub Suite");

    // Assert
    CheckEquals("", empty.GetPath(), ASWBCB_HERE, "an empty path is allowed, for no suite");
    CheckEquals(0u, static_cast<unsigned int>(empty.GetNames().size()), ASWBCB_HERE,
        "an empty path has no suite names");
    CheckEquals("Framework", JoinNames(single.GetNames()), ASWBCB_HERE, "a path without '.' is one suite name");
    CheckEquals("Framework.Console.Colors", nested.GetPath(), ASWBCB_HERE, "GetPath() is the path as given");
    CheckEquals("Framework|Console|Colors", JoinNames(nested.GetNames()), ASWBCB_HERE,
        "a nested path splits at each '.', outermost first");
    CheckEquals("My Suite|Sub Suite", JoinNames(spaced.GetNames()), ASWBCB_HERE, "only '.' separates suite names");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_Suite, "SelfTests.Nested.Suites")
