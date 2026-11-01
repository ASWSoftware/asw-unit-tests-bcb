# Porting from ASWUnitTests

ASWUnitTestsBCB keeps [ASWUnitTests](https://github.com/ASWSoftware/asw-unit-tests)' class names,
check names, argument order and failure texts, so most of a test's body ports unchanged. What
changes is how tests are declared (published methods instead of registration), the C++ the tests may
use (C++03, for bcc32), and a few names. [Differences from ASWUnitTests](Differences.md) lists every
difference; this page is the porting checklist.

## A Test Group, Before and After

An ASWUnitTests test group:

```
#include "ASWUnitTests_Registry.h"
#include "ASWUnitTests_TestBase.h"

namespace ASWUnitTests
{

struct TParseCase
{
    char const* Text;
    int Expected;
};

class TTest_TStack : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

private:
    std::vector<int> m_Stack;

protected:
    void SetUp_Test(ITestCase& testCase) override;

public:
    TTest_TStack();

    void Test_Push_Grows();
    void Test_Top_Empty();
    void Test_Parse(TParseCase const& row);
};

TTest_TStack::TTest_TStack()
    : inherited("TStack_Tests")
{
    RegisterTest(&TTest_TStack::Test_Push_Grows, "Push_Grows");
    RegisterTest(&TTest_TStack::Test_Top_Empty, "Top_Empty");
    RegisterTestCases(&TTest_TStack::Test_Parse, "Parse", std::vector<TParseCase>{ { "1", 1 }, { "22", 22 } });
}

void TTest_TStack::SetUp_Test(ITestCase& /*testCase*/)
{
    m_Stack.clear();
}

void TTest_TStack::Test_Parse(TParseCase const& row)
{
    CheckEquals(row.Expected, std::stoi(row.Text), __func__, __LINE__);
}

void TTest_TStack::Test_Push_Grows()
{
    m_Stack.push_back(1);
    CheckEquals(1u, m_Stack.size(), __func__, __LINE__, "one item");
}

void TTest_TStack::Test_Top_Empty()
{
    CheckThrows<std::out_of_range>([&] { m_Stack.at(0); }, __func__, __LINE__, "empty");
}

} // namespace ASWUnitTests

ASW_REGISTER_TEST_GROUP(ASWUnitTests::TTest_TStack)
```

The same group in ASWUnitTestsBCB:

```
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_TestBase.h"

namespace ASWUnitTestsBCB
{

struct TParseCase
{
    char const* Text;
    int Expected;
};

class TTest_TStack : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

private:
    std::vector<int> m_Stack;

    void __fastcall DoTopOfEmpty();
    void __fastcall Test_Parse(TParseCase const& row);

protected:
    virtual void SetUp_Test(ITestCase& testCase);

public:
    __fastcall TTest_TStack();

__published:
    void __fastcall Test_Push_Grows();
    void __fastcall Test_Top_Empty();
};

__fastcall TTest_TStack::TTest_TStack()
    : inherited("TStack_Tests")
{
    TParseCase const cases[] = { { "1", 1 }, { "22", 22 } };
    RegisterTestCases(&TTest_TStack::Test_Parse, "Parse",
        std::vector<TParseCase>(cases, cases + (sizeof(cases) / sizeof(cases[0]))));
}

void TTest_TStack::SetUp_Test(ITestCase& /*testCase*/)
{
    m_Stack.clear();
}

void __fastcall TTest_TStack::DoTopOfEmpty()
{
    m_Stack.at(0);
}

void __fastcall TTest_TStack::Test_Parse(TParseCase const& row)
{
    CheckEquals(row.Expected, StrToInt(row.Text), __func__, __LINE__);
}

void __fastcall TTest_TStack::Test_Push_Grows()
{
    m_Stack.push_back(1);
    CheckEquals(1u, m_Stack.size(), __func__, __LINE__, "one item");
}

void __fastcall TTest_TStack::Test_Top_Empty()
{
    CheckThrows<std::out_of_range>(DoTopOfEmpty, __func__, __LINE__, "empty");
}

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP(ASWUnitTestsBCB::TTest_TStack)
```

## Step by Step

1. **Names.** Include `ASWUnitTestsBCB_TestBase.h` and `ASWUnitTestsBCB_Registry.h` instead of
   their `ASWUnitTests_` forms, use the `ASWUnitTestsBCB` namespace, and register the group with
   the `ASWBCB_` form of its `ASW_REGISTER_TEST_GROUP` macro (`ASWBCB_REGISTER_TEST_GROUP`,
   `ASWBCB_REGISTER_TEST_GROUP_ORDERED` or their `_IN_SUITE` forms). `TTestGroupBase`,
   `ITestCase`, `SetUp_Group()`, `SetUp_Test()`, `TearDown_Test()` and `TearDown_Group()` keep their
   names.
2. **Constructor and destructor.** `TTestGroupBase` derives from `System::TObject`, so declare the
   group's constructor `__fastcall`, and its destructor, if it has one, `__fastcall virtual`. The
   name passed to `inherited(...)` stays the group name, and a `TSuitePath` before it stays too.
3. **Tests.** Move each test method into a `__published:` section, declared
   `void __fastcall Test_<Name>()`, and delete its `RegisterTest()` call. The test's name is the
   method's name without `Test_`, so rename a method whose registered name differed. An `ASW_TEST`
   test becomes an ordinary published method: `ASW_TEST(TTest_TStack, Push_Grows) { ... }` is
   `void __fastcall TTest_TStack::Test_Push_Grows() { ... }`, and the group derives from
   `TTestGroupBase` instead of `TAutoTestGroup<...>`.
4. **Test order.** Tests run in the order they're declared, after any the group inherits.
   ASWUnitTests runs `ASW_TEST` tests alphabetically, then registered ones in registration order, so
   reorder the declarations if the output order matters to you.
5. **Location arguments.** `__func__, __LINE__` still work. `ASWBCB_HERE` is shorter. A check
   without location arguments (the C++20 `std::source_location` forms) needs one added.
6. **C++03.** Rewrite test code that uses C++11 or later for bcc32: no `auto`, lambdas,
   range-based `for`, `override`, brace-initialized containers, raw string literals or
   `std::shared_ptr`/`std::function`. `nullptr` and `enum class` work. See
   [Notes for bcc32](../README.md#notes-for-bcc32).
7. **Lambdas passed to checks.** `CheckThrows`, `AssertThrows`, `CheckNoThrow` and `AssertNoThrow`
   take a closure, a function or a functor object instead of a lambda: move the lambda's body into a
   method of the group (`DoTopOfEmpty` above), which can use the group's members.
8. **Parameterized tests.** `RegisterTestCases()` takes the same arguments, with the method declared
   `void __fastcall` and not published. Build the rows with `push_back`, or from an array as above,
   and give a row-naming function as a plain function pointer,
   `std::string Name(TParam const& row)`.
9. **RTL support.** Drop `ASWUNITTESTS_RTL_EXCEPTIONS` from the project: RTL exceptions and RTL
   strings work everywhere, with every compiler.
10. **Command lines and CI scripts.** The options, exit codes and JUnit report are the same; change
    the executable's name, and note that `--project-name` defaults to `ASWUnitTestsBCB`.

## Checks That Behave Differently

Most checks port unchanged. These may need a look:

- `CheckMatches` and `CheckNotMatches` use PCRE syntax (the RTL's `TRegEx`), not ECMAScript. Common
  patterns read the same in both; there are no raw string literals in C++03, so double the
  backslashes (`"\\d{4}"`).
- `CheckEquals` of two `enum class` values compares them as plain enums, by their integer values, so
  an `enum class` also compares with an integer or another enum type, which ASWUnitTests rejects.
- `CheckNull`, `CheckSame` and `CheckIsType` don't take `std::shared_ptr` or `std::function`; pass a
  raw pointer or a `std::unique_ptr`.
- `CheckNear`, `CheckNotNear` and their `Assert` forms have `float` and `double` overloads only, so
  a call must have a `float` or a `double` argument and not both; the others convert to its type. A
  call that mixes the two, such as `CheckNear(2.0, aFloat, 0.01, ...)`, or has neither, such as
  `CheckNear(10, count, 1, ...)`, which ASWUnitTests takes since 1.5.0, is ambiguous here and
  doesn't compile: convert an argument, e.g.
  `CheckNear(2.0, static_cast<double>(aFloat), 0.01, ASWBCB_HERE)`.
- `SetExceptionExpected<T>()` with a class `T` that isn't a `std::exception` doesn't match a
  `std::exception` subclass derived from it, and `CheckThrows<T>` matches one without checking its
  `what()`. Expect the `std::exception` subclass itself, or `std::exception`, to check its message.
- A thrown object that is neither a `std::exception` nor an RTL exception is described as "an object
  that is neither a std::exception nor an RTL Exception", where ASWUnitTests says "a
  non-std::exception object" or names its type. Only a test or script that reads failure messages
  notices.

## What Has No Equivalent

- `RegisterTest()` with a lambda or any other callable: every test is a published method.
- `ASW_TEST` and `TAutoTestGroup`: published methods register themselves.
- The `std::source_location` check overloads.
- `TExceptRTLException`: an RTL exception that ends a run under `--test-timeout-seconds` arrives as
  a `TExceptFatalError` with the same text.
- CMake, `ASWUnitTests_Sources.cmake`, Linux and the non-RAD Studio compilers.
- The Win32 GUI runner: use the VCL GUI runner.
- `TTestHandler::GetAppBitsStr()` and `GetBuildOS()`: check `_WIN64`; the operating system is always
  Windows.
- `UTF8ToWide()`, `CommandLineArguments()`, `SystemErrorText()` and `TConsoleUTF8Output`: use
  `FromUTF8()`, `TCLIParser::SplitCommandLine(::GetCommandLineW())` and the RTL's
  `SysErrorMessage()`. The console runner sets the console's code page to UTF-8 itself.
