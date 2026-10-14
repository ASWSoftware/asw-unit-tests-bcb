# Porting from DUnit

DUnit and ASWUnitTestsBCB find tests the same way: a test is a `__published` method of a test class,
found through the RTL's type information. Porting a C++Builder DUnit test case is mostly renaming:
the base class, the test methods, the setup methods and the checks. This page covers DUnit as RAD
Studio ships it (`TestFramework.hpp`).

## A Test Case, Before and After

A DUnit test case:

```
#include <TestFramework.hpp>

class TTestConversions : public TTestCase
{
private:
    TStringList* m_List;

    void __fastcall ParseInvalid();

public:
    __fastcall virtual TTestConversions(System::String name)
        : TTestCase(name)
    {
    }

    virtual void __fastcall SetUp();
    virtual void __fastcall TearDown();

__published:
    void __fastcall TestStrToInt();
    void __fastcall TestStrToIntInvalid();
    void __fastcall TestSum();
};

void __fastcall TTestConversions::TestStrToInt()
{
    CheckEquals(42, StrToInt("42"), "decimal");
    CheckEquals(255, StrToInt("$FF"), "hexadecimal");
}

void __fastcall TTestConversions::TestStrToIntInvalid()
{
    CheckException(ParseInvalid, __classid(EConvertError), "not a number");
}

void __fastcall TTestConversions::TestSum()
{
    CheckEquals(0.3, 0.1 + 0.2, 1e-9);
}

static void registerTests()
{
    Testframework::RegisterTest(TTestConversions::Suite());
}
#pragma startup registerTests 33
```

The same test group in ASWUnitTestsBCB:

```
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_TestBase.h"

class TTestConversions : public ASWUnitTestsBCB::TTestGroupBase
{
private:
    TStringList* m_List;

    void __fastcall ParseInvalid();

protected:
    virtual void SetUp_Test(ASWUnitTestsBCB::ITestCase& testCase);
    virtual void TearDown_Test(ASWUnitTestsBCB::ITestCase& testCase);

public:
    __fastcall TTestConversions()
        : ASWUnitTestsBCB::TTestGroupBase("Conversions"),
          m_List(nullptr)
    {
    }

__published:
    void __fastcall Test_StrToInt();
    void __fastcall Test_StrToInt_Invalid();
    void __fastcall Test_Sum();
};

void __fastcall TTestConversions::Test_StrToInt()
{
    AssertEquals(42, StrToInt("42"), ASWBCB_HERE, "decimal");
    AssertEquals(255, StrToInt("$FF"), ASWBCB_HERE, "hexadecimal");
}

void __fastcall TTestConversions::Test_StrToInt_Invalid()
{
    CheckThrows<EConvertError>(ParseInvalid, ASWBCB_HERE, "not a number");
}

void __fastcall TTestConversions::Test_Sum()
{
    CheckNear(0.3, 0.1 + 0.2, 1e-9, ASWBCB_HERE);
}

ASWBCB_REGISTER_TEST_GROUP(TTestConversions)
```

## Step by Step

1. **Base class and constructor.** Derive from `ASWUnitTestsBCB::TTestGroupBase` instead of
   `TTestCase`, and give the class a default constructor that passes the group's name to
   `TTestGroupBase`. The name replaces the class name in test names, e.g. `Conversions.StrToInt`.
2. **Test methods.** Rename each published test method to `Test_<Name>`, keeping its
   `void __fastcall` signature. DUnit runs every published method; ASWUnitTestsBCB runs only those
   named `Test_...`, and leaves other published methods alone. Make sure every `Test_` method takes
   no parameters, since neither framework can check that.
3. **Setup and teardown.** DUnit's `SetUp()` and `TearDown()` become `SetUp_Test(ITestCase&)` and
   `TearDown_Test(ITestCase&)`, which aren't `__fastcall`. A `TTestSetup` decorator's one-time setup
   and teardown become `SetUp_Group()` and `TearDown_Group()`.
4. **Registration.** Replace the `registerTests()` function and its `#pragma startup` with
   `ASWBCB_REGISTER_TEST_GROUP(ClassName)` at file scope in the group's `.cpp` file. A test
   registered in a suite, with `RegisterTest("Parsers.Numbers", ...)`, becomes
   `ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ClassName, "Parsers.Numbers")` (see
   [Organizing Groups into Suites](../README.md#organizing-groups-into-suites)); write a path DUnit
   separates with `\` or `/` with `.` instead.
5. **Checks.** Add `ASWBCB_HERE` after each check's values, before its message, and rename them as
   the table below shows. Decide for each test whether its checks should stop it (see below).
6. **The program.** Replace the DUnit project's `main()` or `WinMain()`, which starts
   `TextTestRunner` or `GUITestRunner`, with ASWUnitTestsBCB's console runner (`src\main.cpp`) or
   its [VCL GUI runner](../README.md#vcl-gui-runner).

## One Instance per Group

DUnit creates a new instance of the test case for each test method, so members start fresh for every
test. ASWUnitTestsBCB creates each group once, and runs all its tests on that one instance. Set up
per-test state in `SetUp_Test()`, as in the example, rather than relying on the constructor, or a
test can see what the previous one left behind. State that's expensive to build, and safe to share,
can move to `SetUp_Group()`.

## Checks Stop the Test in DUnit

A failed DUnit check raises an exception, so the test stops at its first failure. In ASWUnitTestsBCB
a failed `Check*` records the failure and lets the test go on, so one run shows every failed check,
and only an `Assert*` stops the test. For the same behavior as DUnit, port DUnit's `CheckX` to
`AssertX`, as the example does for checks whose failure makes the rest of the test pointless. Port
them to `CheckX` where seeing every failure helps more.

## Checks

| DUnit | ASWUnitTestsBCB |
|---|---|
| `Check(condition, msg)`, `CheckTrue`, `CheckFalse` | `CheckTrue(condition, ASWBCB_HERE, msg)`, `CheckFalse` |
| `CheckEquals(expected, actual, msg)` for integers, `bool`, strings | `CheckEquals(expected, actual, ASWBCB_HERE, msg)`, any integer and string types |
| `CheckEqualsString`, `CheckEqualsWideString` | `CheckEquals` |
| `CheckEquals(expected, actual, delta, msg)` for floating-point values | `CheckNear(expected, actual, delta, ASWBCB_HERE, msg)`, `float` or `double` |
| `CheckEquals(expected, actual)` for floating-point values, without a delta | `CheckNear` with a tolerance; there's no exact floating-point comparison |
| `CheckNotEquals` | `CheckNotEquals`, or `CheckNotNear` for floating-point values |
| `CheckEqualsMem`, `CheckNotEqualsMem` | The same, with `ASWBCB_HERE` after the size |
| `CheckEqualsHex`, `CheckEqualsBin` | `CheckEquals`; failures show decimal numbers |
| `CheckEquals` for a `GUID` | `CheckEqualsMem(&expected, &actual, sizeof(GUID), ASWBCB_HERE)` |
| `CheckNull`, `CheckNotNull` | The same, for pointers, `std::unique_ptr` and interface references |
| `CheckSame` | `CheckSame` |
| `CheckIs(object, __classid(TFoo))` | `CheckIsType<TFoo>(object, ASWBCB_HERE)` |
| `CheckEquals(__classid(TFoo), cls)` for classes | `CheckEquals(__classid(TFoo), cls, ASWBCB_HERE)`, compared as pointers |
| `CheckInherits(__classid(TBase), cls)` | `CheckTrue(cls->InheritsFrom(__classid(TBase)), ASWBCB_HERE)` |
| `CheckException(Method, __classid(EFoo), msg)` | `CheckThrows<EFoo>(Method, ASWBCB_HERE, msg)` |
| `StartExpectingException(__classid(EFoo))`, or setting `ExpectedException` | `SetExceptionExpected<EFoo>(ASWBCB_HERE)`, for the rest of the test |
| `StopExpectingException()` | No equivalent: put the code that should throw in a method and use `CheckThrows` |
| `Fail(msg)` | `Fail(ASWBCB_HERE, msg)` |
| `Status(msg)` | `Log(msg)` |

ASWUnitTestsBCB has checks DUnit lacks, such as `CheckContains`, `CheckMatches`, `CheckEmpty` and
`CheckGreaterThan`, and `Skip()` to report a test as skipped. See
[Checks and Asserts](../README.md#checks-and-asserts).

## What Has No Equivalent

- **Repeated tests** (`TRepeatedTest`, `RunCount`). Use a
  [parameterized test](../README.md#parameterized-tests), or a loop inside the test.
- **Failing a test that runs no checks** (DUnit's `FailsOnNoChecksExecuted`), and DUnit's memory
  leak reporting.
- **Per-test-method instances.** See [One Instance per Group](#one-instance-per-group).
