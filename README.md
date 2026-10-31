# ASWUnitTestsBCB - https://github.com/ASWSoftware/asw-unit-tests-bcb

ASWUnitTestsBCB is a unit-testing framework for C++Builder (RAD Studio) projects. It is written in
the C++03 subset that RAD Studio's classic Win32 compiler (bcc32) accepts, so it tests code that
must still build with that compiler, and the same tests build unchanged with RAD Studio's
Clang-based compilers. It needs only the RTL.

A test group is a class derived from `TTestGroupBase`, and every method in its `__published` section
named `Test_<Name>` runs as a test, found through the RTL's own type information, as DUnit finds
them. There's no list of tests to keep up to date, so a test can't be written but left unregistered.

It is a sibling of [ASWUnitTests](https://github.com/ASWSoftware/asw-unit-tests), a C++17/C++20
unit-testing framework for Windows and Linux. The two share concepts, names, command-line options,
output and report formats, so tests, habits and CI scripts move between them with few changes, but
they share no code. Use ASWUnitTests for code that only needs C++17 or later compilers.
[Differences from ASWUnitTests](docs/Differences.md) compares them feature by feature.
To move existing tests over, see [Porting from DUnit](docs/PortingFromDUnit.md) and
[Porting from ASWUnitTests](docs/PortingFromASWUnitTests.md).

Requires RAD Studio 13.1 or later.

# Features

## Writing Tests

- **[Published test methods](#writing-a-test-group)** - Every `void __fastcall Test_<Name>()` in a
  group's `__published` section runs as the test `<Name>`, in declaration order, after the tests the
  group inherits.
- **[Self-registering test groups](#registering-test-groups)** - `ASWBCB_REGISTER_TEST_GROUP` adds
  a group from its own `.cpp` file, without editing any framework file.
- **[Test suites](#organizing-groups-into-suites)** - `TSuitePath` puts groups in nested suites, so
  `--filter "Framework.*"` selects a whole suite, and the VCL GUI runner shows suites in its tree.
- **[Check and Assert methods](#checks-and-asserts)** - `Check*` records a failure and lets the test
  go on; `Assert*` ends it. Covers `Equals`/`NotEquals`, `True`/`False`, `Null`/`NotNull`,
  `Same`/`NotSame`, `Empty`/`NotEmpty`, `IsType`/`IsNotType`, `Near`/`NotNear`,
  `Contains`/`NotContains`, `StartsWith`/`EndsWith` (and their `Not` forms), `Matches`/`NotMatches`
  (regular expression), `EqualsMem`/`NotEqualsMem` (bytes), and `GreaterThan`/`LessThan` (and their
  `OrEqual` forms), plus case-insensitive `IC` forms of the string `Equals`, `Contains`,
  `StartsWith` and `EndsWith` methods.
- **Every string type** - The string checks take C strings, `std::string` (UTF-8),
  `std::wstring`, `UnicodeString` (`String`), `AnsiString` and `UTF8String`, in any combination.
- **[Expected exceptions](#expecting-an-exception)** - `SetExceptionExpected()`, with an optional
  check of the exception's type (RTL, `std::exception` or any other) and message, and
  [`CheckThrows()`/`CheckNoThrow()`](#checking-that-a-call-throws) for a single call.
- **[Parameterized tests](#parameterized-tests)** - `RegisterTestCases()` registers one named test
  per row of data.
- **[Failing](#failing-a-test) and [skipping](#skipping-a-test) tests** - `Fail()` fails a test
  unconditionally, and `Skip()` reports it as skipped.
- **Setup and teardown hooks** - `SetUp_Group()`/`TearDown_Group()` run around each group, and
  `SetUp_Test()`/`TearDown_Test()` around each test.

## Running Tests

- **Filtering and listing** - `--filter` selects tests by wildcard pattern; `--list` previews the
  matches without running them.
- **Shuffled order** - `--shuffle` runs tests in a random order to expose hidden coupling between
  them; `--shuffle-seed` reproduces an order, the same one ASWUnitTests gives the same seed.
- **Parallel partitions** - `--partition-index`/`--partition-count` split the suite across separate
  processes, e.g. a CI job matrix.
- **Hang protection** - `--test-timeout-seconds` ends the run if a single test doesn't finish in
  time, instead of blocking forever.
- **Crash protection** - `--catch-crashes` records a stack overflow or an illegal instruction in a
  test as a failure instead of losing the whole process.
- **Unexpected exceptions** - An exception a test throws without expecting it, an access violation
  included, fails only that test, and the run carries on.
- **[VCL GUI runner](#vcl-gui-runner)** - A VCL application that runs the same tests, with a check
  box tree to choose them, live pass/fail/skip status, and each failure's details and log.

## Reporting

- **JUnit XML report** - `--report-junit` writes a report that most CI systems read natively.
- **Colored console output** - Configurable pass/fail/skip colors, with automatic console detection
  and `NO_COLOR` support.
- **Per-test timing** - Every test logs its duration, so slow tests are easy to spot.
- **[Meaningful exit codes](#command-line-options)** - They tell failed tests, run-ending
  exceptions, invalid arguments, timeouts and crashes apart.

## Compilers

| Compiler | Platform | Status |
|---|---|---|
| bcc32 (classic) | Win32 | Supported |
| bcc64x (Clang, modern) | Win64x | Supported |
| bcc32c (Clang, C++17) | Win32 | Supported |
| bcc64 (Clang, legacy Win64) | Win64 | Not supported |

Every release is built without warnings and its self-tests pass on bcc32, bcc32c and bcc64x, in
Debug and optimized Release builds. There's no CI service with RAD Studio, so this is done locally.

# Donations:

If you find ASWUnitTestsBCB helpful, donations are always appreciated:

PayPal: donate@aswsoftware.com

Bitcoin: 15rKqL1numHJyE36ottMbhs5cmCjJkuowV

# How to Use

Important! Run tests outside the debugger, unless you really want to stop at every exception, since
unit tests throw and catch plenty of them on purpose.

The repository's folders:

- `src` - the framework, and the console runner (`main.cpp`).
- `gui` - the [VCL GUI runner](#vcl-gui-runner).
- `tests` - the framework's own tests, written with it, and an example test group,
  `tests\Test_ASWExample_WordCounter.cpp`, for the example code in `toTest`.
- `rad370` - the RAD Studio project that builds the console runner with those tests, its build
  scripts, and a project group that also holds the GUI runner's project.

To use the framework in your own projects, see
[Adding ASWUnitTestsBCB to Your Projects](#adding-aswunittestsbcb-to-your-projects).

`rad370\ASWUnitTestsBCB.cbproj` builds `ASWUnitTestsBCB.exe` for every supported compiler. Its Win32
platform uses bcc32, as the IDE builds it. Each compiler and configuration also has a build script,
which works from any folder once the `Rad370` environment variable points to the RAD Studio
installation (e.g. `C:\Program Files (x86)\Embarcadero\Studio\37.0`):

| Compiler | Scripts | Executable |
|---|---|---|
| bcc32 | `rad370\Build_Win32_Debug.bat`, `rad370\Build_Win32_Release.bat` | `rad370\Win32\<Config>\ASWUnitTestsBCB.exe` |
| bcc32c | `rad370\Build_Win32Clang_Debug.bat`, `rad370\Build_Win32Clang_Release.bat` | `rad370\Win32Clang\<Config>\ASWUnitTestsBCB.exe` |
| bcc64x | `rad370\Build_Win64x_Debug.bat`, `rad370\Build_Win64x_Release.bat` | `rad370\Win64x\<Config>\ASWUnitTestsBCB.exe` |

## Writing a Test Group

A test group is a class derived from `TTestGroupBase` (in `src\ASWUnitTestsBCB_TestBase.h`), which
derives from `System::TObject`. Its tests are the methods in its `__published` sections declared
`void __fastcall Test_<Name>()`, and each runs as the test `<Name>`:

```
// Test_TStringList.h
#include <System.Classes.hpp>
#include "ASWUnitTestsBCB_TestBase.h"

namespace ASWUnitTestsBCB
{

class TTest_TStringList : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

private:
    System::Classes::TStringList* m_List;

protected:
    virtual void SetUp_Test(ITestCase& testCase);
    virtual void TearDown_Test(ITestCase& testCase);

public:
    __fastcall TTest_TStringList();
    __fastcall virtual ~TTest_TStringList();

__published: // Tests: every "void __fastcall Test_<Name>()" here runs as the test <Name>
    void __fastcall Test_Add_KeepsOrder();
    void __fastcall Test_Delete_OutOfRange();
    void __fastcall Test_Sorted_IgnoresCase();
};

} // namespace ASWUnitTestsBCB
```

```
// Test_TStringList.cpp
#include <System.hpp>
#pragma hdrstop

#include "Test_TStringList.h"
#include <System.SysUtils.hpp>
#include "ASWUnitTestsBCB_Registry.h"

using namespace System;
using namespace System::Classes;
using namespace System::Sysutils;

namespace ASWUnitTestsBCB
{

__fastcall TTest_TStringList::TTest_TStringList()
    : inherited("TStringList_Tests"),
      m_List(nullptr)
{
}

__fastcall TTest_TStringList::~TTest_TStringList()
{
    delete m_List;
}

void TTest_TStringList::SetUp_Test(ITestCase& /*testCase*/)
{
    m_List = new TStringList();
}

void TTest_TStringList::TearDown_Test(ITestCase& /*testCase*/)
{
    delete m_List;
    m_List = nullptr;
}

void __fastcall TTest_TStringList::Test_Add_KeepsOrder()
{
    m_List->Add("pear");
    m_List->Add("apple");

    AssertEquals(2, m_List->Count, ASWBCB_HERE); // Ends the test if it fails
    CheckEquals("pear", m_List->Strings[0], ASWBCB_HERE, "first in, first listed");
    CheckEquals(L"apple", m_List->Strings[1], ASWBCB_HERE);
}

void __fastcall TTest_TStringList::Test_Delete_OutOfRange()
{
    SetExceptionExpected<EStringListError>(ASWBCB_HERE, "an empty list", "out of bounds");

    m_List->Delete(0);
}

void __fastcall TTest_TStringList::Test_Sorted_IgnoresCase()
{
    m_List->Sorted = true;
    m_List->Add("b");
    m_List->Add("A");

    CheckEquals("A", m_List->Strings[0], ASWBCB_HERE);
}

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP(ASWUnitTestsBCB::TTest_TStringList)
```

The string given to the constructor names the group, so these tests are
`TStringList_Tests.Add_KeepsOrder` and so on, the full names `--filter`, `--list` and the reports
use. A group in a [suite](#organizing-groups-into-suites) has the suite path in front. The namespace
is up to you.

How a group runs:

- All groups are created when the run starts, each with `new` and its default constructor, and
  destroyed when it ends.
- `SetUp_Group()` runs before the group's first test, and `TearDown_Group()` after its last. A group
  with no test to run (for example, none matching `--filter`) isn't set up at all.
- Each test runs between `SetUp_Test()` and `TearDown_Test()`, which get the test as an `ITestCase`
  (`testCase.GetName()` is its name). All of a group's tests run on the one instance, so state a
  test leaves in members carries over to the next unless `SetUp_Test()` resets it.
- Tests run in the order they're declared in. A group derived from another group runs the
  inherited tests first, then its own. A test method a descendant publishes again (an override, or a
  method that hides it) runs in the ancestor's test's place.
- Only published methods whose names start with `Test_` are tests; other published methods are left
  alone. Publishing two methods with the same `Test_` name in one class (overloads) stops the run at
  startup with an error naming the class and the method, since the reports couldn't tell them apart.
- If a group's constructor or `SetUp_Group()` throws, each of its tests fails with that exception,
  and the run carries on with the next group. An exception from `SetUp_Test()`, `TearDown_Test()` or
  `TearDown_Group()` ends the run, with exit code `2` (an RTL or `std::exception`) or `3` (any other
  type).

**Declare every test `void __fastcall Test_<Name>()` exactly.** The RTL's type information names a
published method but doesn't describe its parameters, so the framework can't check them: a published
`Test_` method with parameters or another calling convention would be called wrongly.

Base groups can share tests. Here, every registered descendant of `TTest_TShapeBase` runs
`Area_Positive` against its own `Area()`; the base itself isn't registered:

```
class TTest_TShapeBase : public TTestGroupBase
{
public:
    __fastcall TTest_TShapeBase(std::string const& name)
        : TTestGroupBase(name)
    {
    }

    virtual double Area() = 0;

__published:
    void __fastcall Test_Area_Positive();
};

void __fastcall TTest_TShapeBase::Test_Area_Positive()
{
    CheckGreaterThan(Area(), 0.0, ASWBCB_HERE);
}

class TTest_TSquare : public TTest_TShapeBase
{
public:
    __fastcall TTest_TSquare()
        : TTest_TShapeBase("TSquare_Tests")
    {
    }

    virtual double Area()
    {
        return 4.0;
    }

__published:
    void __fastcall Test_Area_SideSquared();
};

void __fastcall TTest_TSquare::Test_Area_SideSquared()
{
    CheckNear(4.0, Area(), 1e-9, ASWBCB_HERE);
}
```

A group can write to the log with `Log("text")`. The text goes to the console, to the test's own
log in the [GUI runner](#vcl-gui-runner), and to an `ITestRunObserver`.

See `tests\Test_ASWExample_WordCounter.cpp` for a complete example.

## Registering Test Groups

A group registers itself with `ASWBCB_REGISTER_TEST_GROUP` (declared in
`src\ASWUnitTestsBCB_Registry.h`), at file scope in its own `.cpp` file, as above. No file in `src`
ever needs to change to add, remove or rename a test group.

By default, groups run in alphabetical order by group name, the same with every compiler and linker.
To run a group earlier or later, register it with `ASWBCB_REGISTER_TEST_GROUP_ORDERED(ClassName,
order)` instead. Groups run in ascending order (the default is `0`), with ties broken
alphabetically:

```
ASWBCB_REGISTER_TEST_GROUP_ORDERED(ASWUnitTestsBCB::TTest_TSquare, -1) // Runs before the alphabetical block
```

Run order is for readable, reproducible output only. A group's `SetUp_Group()`/`TearDown_Group()`
should make no assumption about which other groups have already run.

Nothing refers to a test group's `.cpp` file except its own registration, so a linker could leave
the file out if the tests were built into a static library. Compile the test files into the test
program itself, as a RAD Studio project does with the files it lists.

### Organizing Groups into Suites

To organize many groups, put each in a suite by passing a `TSuitePath` to its base class
constructor, ahead of the group's own name. A suite path names nested suites, outermost first,
separated by `.`:

```
__fastcall TTest_TConsole::TTest_TConsole()
    : inherited(TSuitePath("Framework.Console"), "TConsole_Tests")
{
}
```

That puts group `TConsole_Tests` in suite `Console`, inside suite `Framework`. The group's
qualified name, `Framework.Console.TConsole_Tests`, starts all of its tests' full names, so:

- `--filter "Framework.*"` runs every test in suite `Framework`, including those in `Console`, and
  `--filter "Framework.Console.*"` runs just those in `Console`.
- `--list`, the console log and the `--report-junit` report show its tests as
  `Framework.Console.TConsole_Tests.TestName`, and the [VCL GUI runner](#vcl-gui-runner)'s test
  tree shows each suite as a node, whose check box checks or unchecks all of its tests.

The registration can name the suite instead, leaving the constructor as it is:

```
ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_TConsole, "Framework.Console")
```

`ASWBCB_REGISTER_TEST_GROUP_IN_SUITE_ORDERED(ClassName, suitePath, order)` takes an order too. The
registration's suite also holds the tests of a group whose constructor throws: they fail under the
group's class name, since the constructor never got to name the group, in the suite the
registration names, so a suite's `--filter` still reports them. A group whose registration and
constructor name different suites stops the run at startup.

A suite's groups always run together. Within a suite, or at the top level, groups and suites run in
alphabetical order by their own names. The order given to `ASWBCB_REGISTER_TEST_GROUP_ORDERED`
only places a group among the groups and suites beside it, where a suite counts as order `0`, so it
never moves a group out of its suite. `--shuffle` shuffles each suite's contents separately, so they
still run together.

Suites only organize groups: they have no `SetUp_Group()`/`TearDown_Group()` of their own. A suite
name can't be empty, so a path can't start or end with `.` or contain `..`; a group with such a
path stops the run at startup with exit code `2` (`TTestHandler::Initialize()` throws
`TExceptInvalidSuitePath`). Moving a group into a suite changes its tests' full names, as renaming
the group would, so update any `--filter` pattern that names them; the GUI runner's saved selection
of them is lost too.

## Versions

ASWUnitTestsBCB follows [Semantic Versioning](https://semver.org), shown by `--version` and the GUI
runner's caption. Releases are tagged on `main` (e.g. `v1.0.0`). Between releases, the `develop`
branch carries the next planned version with a pre-release, e.g. `1.1.0-dev.1`, which comes before
`1.1.0`. `src\ASWUnitTestsBCB_Version.h` has the version as macros, for code that supports several
versions:

```
#include "ASWUnitTestsBCB_Version.h"

#if ASWUNITTESTSBCB_VERSION_MAJOR > 1 || (ASWUNITTESTSBCB_VERSION_MAJOR == 1 && ASWUNITTESTSBCB_VERSION_MINOR >= 1)
    // Uses something added in 1.1 (also present in 1.1.0-dev.N builds)
#endif
```

## Command Line Options

```
Usage: ASWUnitTestsBCB [options]

Options:
  --filter <pattern>  Run only tests whose full name matches <pattern>.
                      A full name is "GroupName.TestName", prefixed by
                      "Suite.Path." for a group in a suite. '*' matches
                      any sequence of characters (including none); '?'
                      matches exactly one character. The whole name must
                      match, e.g. "*String*" for a substring search.
  --filter-ignore-case
                      Match --filter's <pattern> case-insensitively.
                      Has no effect without --filter.
  --shuffle           Run groups, and each group's tests, in a randomized
                      order instead of the default deterministic order.
                      The seed used is logged so a failure caused by
                      order can be reproduced via --shuffle-seed.
  --shuffle-seed <N>  Shuffle (implies --shuffle) using an explicit
                      unsigned integer seed, to reproduce a previous
                      --shuffle run's order.
  --partition-index <N>
                      1-based index of this run's partition, from 1 to
                      --partition-count. Requires --partition-count.
  --partition-count <N>
                      Splits the full test suite into <N> roughly-equal
                      partitions by each test's position in the canonical
                      registration order (the same order --list shows),
                      so every test runs in exactly one partition
                      regardless of which group it's in. Run <N> separate
                      invocations (e.g. one per CI job), each with its
                      own --partition-index, to run the suite in parallel
                      with no coordination between processes. Requires
                      --partition-index.
  --test-timeout-seconds <N>
                      Abort the run if any single test does not finish
                      within <N> seconds. The offending test is recorded
                      as failed (with a message explaining why) and no
                      further tests or groups run afterward. There is no
                      default; a hung test runs indefinitely unless this
                      is given.
  --catch-crashes     Catch a native crash in a test that would otherwise
                      end the process (a stack overflow or an illegal
                      instruction) and record the test as failed. A
                      stack overflow aborts the run afterward, the same
                      way --test-timeout-seconds does. Off by default.
                      An access violation always just fails its test.
  --color <mode>      One of "auto" (default; color only on an
                      interactive terminal that supports it, and only
                      if the NO_COLOR environment variable isn't set),
                      "always", or "never".
  --no-color          Shorthand for --color never.
  --color-pass/--color-fail/--color-skip <color>
                      Set the color used for passed/failed/skipped
                      status text. <color> is one of: default, black,
                      red, green, yellow, blue, magenta, cyan, white,
                      or bright-<name> for the bright variant (e.g.
                      bright-red). Defaults: pass=green, fail=red,
                      skip=yellow.
  --report-junit <path>
                      Write a JUnit-style XML test report to <path>, in
                      addition to the normal console output. Recognized
                      by most CI systems (GitHub Actions, GitLab CI,
                      Jenkins, Azure DevOps, CircleCI) for native test
                      result reporting.
  --project-name <name>
                      Set the name this run is identified by: shown in
                      the console's "Initializing..." line and, if
                      --report-junit is also given, used as the report's
                      <testsuites name="..."> attribute (default:
                      "ASWUnitTestsBCB").
  --list              List all registered tests by full name (see
                      --filter) and exit, without running anything.
  --pause             Prompt "Press any key to continue" before exiting
                      after a --list command or test run.
  --version           Print the framework version and exit.
  --help              Show this message and exit.
```

An option's value can also follow an `=`, as in `--filter=*String*`, and `-h` is short for
`--help`. The arguments are read from the program's wide command line, so non-ASCII text in them,
such as a filter or a report path, arrives intact.

The console output always states whether a filter is active (and its pattern) before running or
listing tests, and `--list` reports how many tests and groups matched out of the total registered,
so output redirected to a file records why fewer tests ran or were listed than expected. Likewise,
it always states whether shuffling is on and, if so, the seed in use. The runner sets the console's
output code page to UTF-8 while it runs, so non-ASCII text in test names and messages reads
correctly, and restores it afterwards.

`--shuffle` is a check against hidden coupling between tests and groups. The deterministic default
order is for readable, reproducible output day to day, while `--shuffle` breaks it on purpose, to
surface tests that secretly depend on running in a particular order (e.g. through shared static or
global state). If `--shuffle` causes a failure, rerun with the logged seed via `--shuffle-seed` to
reproduce it exactly while debugging. A seed gives the same order with every compiler, and the same
order ASWUnitTests gives the same tests. Adding `--filter` keeps the matching tests in the order
that seed gives the full suite, so you can narrow the rerun down to the tests involved. A
[suite](#organizing-groups-into-suites)'s groups are shuffled among themselves, so they still run
together.

`--partition-index`/`--partition-count` split the suite for parallel runs in separate processes.
Each process still runs its tests one at a time and writes its own console and JUnit output.
Partitioning is by each test's position in the unshuffled order, not by group, so one large group
doesn't fill a single partition, and a test's partition never depends on `--shuffle`. A typical
setup is a CI job matrix, e.g. 4 jobs each running
`--partition-index <1..4> --partition-count 4 --report-junit results-<index>.xml`. Most CI systems
merge the JUnit reports of parallel jobs themselves. Combine the partition options with `--list` to
preview which tests land in a partition.

`--test-timeout-seconds` guards against a test that hangs (e.g. in an endless loop). Each test then
runs on a worker thread (a `TThread`) while the thread that started the run waits. A test that
finishes in time, whether it passes, fails or throws, behaves exactly as without the option, and
when the run was started from the main thread, the test's `TThread::Synchronize()` and
`TThread::Queue()` calls still reach the main thread, which runs them while it waits. A test that
doesn't finish in time is recorded as failed, with a message naming it and the timeout it exceeded,
its group is torn down, and the run stops there, with exit code `5`. A JUnit report is still
written, with every test that finished plus the timed-out one. A thread can't be stopped safely, so
the abandoned test's thread is left running; if it finishes after all, it changes no results. Since
that thread may still be using its group, the runner then ends the process without destroying
anything.

`--catch-crashes` guards against the two faults that would otherwise end the whole process: a stack
overflow and an illegal instruction. A test that causes one is recorded as failed, e.g.
`Test crashed: stack overflow.`, and the run carries on with the next test, except after a stack
overflow, which ends the run with exit code `6` (the program ends without destroying anything, as
after a timeout). The crashed test is abandoned where it faulted: its `TearDown_Test()` and the
destructors of the objects on its stack don't run. The guard is a vectored exception handler that
jumps back to just before the test, on the thread the test runs on. Other faults, such as an access
violation, are left to the RTL, which turns them into exceptions (`EAccessViolation`) with or
without the option, so they fail just the test, and a test can expect them. What happens to a fault
in a test:

| Fault | Without `--catch-crashes` | With it |
|---|---|---|
| Access violation, privileged instruction | The RTL's exception fails the test | The same |
| Stack overflow | bcc32: the first one fails the test (`EStackOverflow`), a second one on the same thread ends the process; bcc64x: ends the process; bcc32c: may end it | Fails the test and ends the run (exit code `6`) |
| Illegal instruction | bcc32: ends the process; bcc32c, bcc64x: fails the test (`EExternalException`) | Fails the test |
| `std::terminate()`, `abort()` | Ends the process | The same |

Pass, fail and skip status text is colored when the output is a console that shows ANSI colors.
Output redirected to a file or a pipe gets plain text, unless `--color always` forces colors (e.g.
for a CI system whose log viewer shows them). The [NO_COLOR](https://no-color.org) environment
variable is respected in the default `auto` mode.

Every test logs a `Finished test: "GroupName.TestName" - passed/failed/skipped (N.NNN ms)` line,
timed from just before `SetUp_Test()` to just after `TearDown_Test()`, so slow tests are easy to
spot.

`--report-junit` writes a standard `<testsuites>`/`<testsuite>`/`<testcase>` report, with one
`<testsuite>` per test group, and `<failure>`/`<skipped>` elements carrying the same detail shown on
the console. A group in a [suite](#organizing-groups-into-suites) is named by its qualified name
(e.g. `Framework.Console.TConsole_Tests`), in its `<testsuite>`'s `name` and each `<testcase>`'s
`classname`; suites don't get `<testsuite>` elements of their own. The file is UTF-8 with LF line
ends, its path may hold any characters, and its XML is the same as ASWUnitTests' for the same
results. A report that can't be written is noted in the log and doesn't change the exit code.
`TJUnitReportWriter` (`src\ASWUnitTestsBCB_JUnitReport.h`) builds or writes the same report for
other runners.

Exit codes:

| Code | Meaning |
|---|---|
| `0` | Every test that ran passed or was skipped, or `--help`, `--version` or `--list` completed |
| `1` | A test failed |
| `2` | An RTL exception or a `std::exception` from `SetUp_Test()`, `TearDown_Test()` or `TearDown_Group()` ended the run, or a test group's definition stopped the run at startup (a test name used twice, or an invalid or conflicting suite path) |
| `3` | An exception of any other type from one of those ended the run |
| `4` | Invalid command line arguments |
| `5` | A test exceeded `--test-timeout-seconds`, and the run was aborted |
| `6` | A test overflowed the stack with `--catch-crashes` given, and the run was aborted |

An exception a test throws without expecting it fails only that test, with `Unexpected exception: `
and the exception's description as its detail, and the run carries on, so it's an ordinary test
failure (exit code `1`). An RTL exception is described as `ClassName: Message`, e.g.
`EConvertError: 'abc' is not a valid integer value`. Skipped tests never affect the exit code.

## VCL GUI Runner

`gui\rad370\ASWUnitTestsBCB_GUI.cbproj` builds a VCL application that runs the same tests as the
console runner, in a window for choosing tests and reading their results. It runs this repository's
self-tests, and its `gui\rad370\Build_<Platform>_<Config>.bat` scripts write the executable to
`gui\rad370\<Platform>\<Config>`. `rad370\ASWUnitTestsBCB_Group.groupproj` opens it together with
the console project.

The window shows:

- **Test tree** - Every registered test under its group, and each group under its
  [suite](#organizing-groups-into-suites), if any, with a check box to choose what runs (all are
  checked at first) and a colored dot for its status: gray not run, blue running, green passed, red
  failed, amber skipped. A group's or suite's check box and dot summarize its tests, including those
  in the suites inside it. **View > Expand All** and **View > Collapse All** expand every suite and
  group, or collapse the tree to its top-level ones; changing the filter expands it all again.
- **Detail pane** - The selected test's result, duration, failure or skip detail, and its own log
  output, or how many of a selected group's or suite's tests have each status.
- **Failures & Skips** - Every failed or skipped test in the latest run. Double-click one to select
  it in the tree.
- **Log** - The run's full output, with failures in red and skips in amber.
- **Progress and status bars** - Progress (red once anything fails), counts, elapsed time, and the
  run's status.

The toolbar, whose commands are also in the Run and Tests menus:

- **Run Selected** (F9) runs the checked tests, and **Run Failed** reruns the latest run's failures.
- **Stop** ends the run after the current test. Closing the window during a run does the same, then
  closes.
- **Select All** and **Select None** check or uncheck every shown test, and **Select Failed** checks
  only the shown tests that failed in the latest run.
- **Copy Details** (Ctrl+Shift+C) copies the detail pane to the clipboard.
- **Filter** (Ctrl+F) shows only the tests whose `Group.Test` name contains its text, ignoring case,
  with the same `*` and `?` wildcards as `--filter`. While it hides tests, checking them and Run
  Selected only apply to the shown ones, and hidden tests keep their check marks.

**File > Export JUnit Report** saves the latest run's results as the same JUnit XML report
`--report-junit` writes. The Help menu shows the command line options (the same text as `--help`)
and the framework version.

**Options > Run in Shuffled Order** runs the groups, and each group's tests, in a random order, like
`--shuffle`. **Options > Shuffle Seed...** chooses between a new random seed for each run and one
seed for every run, like `--shuffle-seed`, and clicking OK also turns shuffling on. The status bar
shows the seed: the one every run uses, or the latest run's. Shuffle Seed... offers that seed, so
choosing it reruns tests in the same order, Run Failed included, to track down a failure that
depends on the order. Both settings start from the command line and aren't saved.

When the window closes, it saves its size, position, maximized state, and panel sizes to
`%APPDATA%\ASWUnitTestsBCB\<exe name>.ini`, and restores them the next time it opens. A position on
a monitor that's no longer connected is ignored. An `--exit` run doesn't save the layout. **View >
Reset Layout** restores the default layout right away; `--layout-ignore` and `--layout-reset`
(below) control this from the command line.

**View > Theme** chooses a light or dark look. **Use Windows Setting**, the default, follows
Windows' app mode setting (Settings > Personalization > Colors), and switches as soon as it
changes, but stays light while high contrast is on, so the window shows high contrast's own colors.
**Light** and **Dark** keep that look whatever Windows is set to. Dark is one of RAD Studio's dark
VCL styles, Carbon unless **Dark Theme** (below) chooses another, and light the standard Windows
look. A choice applies right away and is saved in the layout file at once, so the next session
starts with it; **View > Reset Layout** leaves it as it is. The Theme commands are disabled during
a run, and a change of the Windows setting during one applies when it ends.

**View > Theme > Dark Theme** chooses the dark look's colors, used whenever the look is dark, with
Dark or with Use Windows Setting: one of five of RAD Studio's dark VCL styles, Carbon (the default),
Charcoal Dark Slate, Cobalt XEMedia, Onyx Blue and Tablet Dark, or a VCL style file of your own. It
lists the styles the executable links in, and Carbon always, then the style files in
`%APPDATA%\ASWUnitTestsBCB\Themes`, which **Open Themes Folder** opens, each by its file name. The
choice is saved in the layout file with the theme, and a problem, such as a missing file, is
reported in the Log tab, with Carbon used in its place.

A style file, `<Name>.vsf`, is made with RAD Studio's Bitmap Style Designer, and the Themes folder
is shared by every test app. Each file needs a style name of its own, not that of a style the
executable already has, such as Carbon. A style file is loaded once per session, so a change to it
shows after a restart.

The window also remembers which tests are checked, in
`%APPDATA%\ASWUnitTestsBCB\<exe name>.selection`, as long as the command line doesn't choose them
itself (no `--run`, `--filter`, or partition options). What it saves is what Run Selected would run,
the checked tests that are shown; the filter box itself starts empty. Since tests come and go
between sessions, it adapts:

- A test that's been removed is skipped.
- A new test (or a renamed one, which looks the same) is checked if its group was entirely checked,
  and otherwise isn't. A test in a new group is checked only if every test was.
- If none of the checked tests still exist, every test is checked instead.

The log notes what was restored. So **Select Failed**, closing the window, fixing the code, and
reopening it leaves just those tests checked for Run Selected.

Tests run on the main (VCL) thread, so a test can create forms and controls.
`--test-timeout-seconds` runs each test on a worker thread instead, so it can't be combined with
tests like that; the log says so when it's given. After a test times out, or a stack overflow caught
by `--catch-crashes` ends a run, the GUI runs no more tests until it's restarted (the status bar
says so), since the abandoned test may still be running, and it then exits without destroying
anything, as the console runner does.

A thread's stack survives one overflow at most. Without `--catch-crashes`, a bcc32 build reports a
test's stack overflow as an `EStackOverflow` failure and carries on, but the main thread's stack is
then left without its guard page, so a later overflow on it, in the same run or a later one, ends
the GUI.

The GUI takes the same [command line options](#command-line-options) as the console runner, with
these differences:

- `--filter` and `--partition-index`/`--partition-count` choose which tests start out checked,
  instead of the saved selection, and `--filter`'s pattern also fills in the filter box. The box
  shows every test the pattern matches (and maybe a few more, left unchecked, since the box ignores
  case and matches anywhere in the name).
- `--project-name` is also shown in the window's caption.
- `--shuffle` and `--shuffle-seed` set the Options menu's shuffle settings, which can then be
  changed.
- `--report-junit` writes the report after every run, including Run Failed.
- `--list`, `--pause`, and the color options are ignored, with a note in the log.
- `--help`, `--version`, and argument errors are shown in a dialog, and the GUI then exits without
  opening.
- `--run` (GUI-only) runs the checked tests as soon as the window opens.
- `--exit` (GUI-only, requires `--run`) closes the window when that run finishes, and exits with the
  same code the console runner would; a run ended with Stop exits with `1`. Without `--exit`, the
  GUI always exits with `0`.
- `--layout-ignore` (GUI-only) neither loads nor saves the window layout or the theme, so the
  window opens with the defaults and the saved ones are left as they are. A theme chosen in the View
  menu then lasts only until the window closes.
- `--layout-reset` (GUI-only) deletes the saved window layout, and with it the theme, and opens
  with the defaults, which is a way to recover if a saved layout ever causes a problem. The layout
  is saved again when the window closes.

A GUI process doesn't make a Windows command prompt wait for it, so a script that needs `--exit`'s
exit code should start it with `start /wait` (or its own language's equivalent):

```
start /wait ASWUnitTestsBCB_GUI.exe --filter "*String*" --run --exit --report-junit results.xml
echo %ERRORLEVEL%
```

The GUI is built on `ITestRunObserver` (in `src\ASWUnitTestsBCB_TestBase.h`), set with
`TTestHandler::SetRunObserver()`. An observer receives the log output that would otherwise go to
`std::cout`, without color codes, is told as each test starts and finishes, and can stop a run
between tests, so it can drive other kinds of runners too.

## Checks and Asserts

`TTestGroupBase` provides the checks as protected methods. Each comes in two forms: a `Check*`
failure records the failure and lets the test go on, failing it when it ends, and an `Assert*`
failure ends the test at once. Use an `Assert*` method where the rest of the test makes no sense
after a failure, e.g. before reading an element whose count was just checked.

Every check takes the location after its values, as `ASWBCB_HERE` or explicitly as
`__FUNC__, __LINE__`, and then an optional message, of any C++ or RTL string type. A failure shows
the method's bare name with every compiler:

```
CheckEquals(2, m_List->Count, ASWBCB_HERE, "two items added");
// Check failed for: "Test_Fail_Equals" (318): Expected "2" but was "1". two items added
```

For the full list, see the protected sections of `TTestGroupBase` in
`src\ASWUnitTestsBCB_TestBase.h`, where each family's comment says what it takes.

### Comparing Values

`CheckEquals`/`AssertEquals` and `CheckNotEquals`/`AssertNotEquals` take the expected value first,
then the actual one. They compare:

- two `bool` values;
- integers of any two types (`int`, `__int64`, `DWORD`, `char`, ...), by value, so `-1` never equals
  an unsigned value, unlike with the built-in `==`, and with no need for a suffix or cast to pick an
  overload. A `char` is shown as a number;
- enums, plain or `enum class`, with each other or with integers, by their integer values, shown as
  numbers;
- pointers, by address;
- strings of any C++ or RTL string types, in any combination: `char const*`, `char` arrays and
  `std::string` (read as UTF-8), `wchar_t const*`, `wchar_t` arrays and `std::wstring`,
  `UnicodeString` (`String`), `AnsiString` and `UTF8String`. Two narrow or two wide C++ strings are
  compared as they are; any other pair is compared as Unicode text. C strings are compared by
  content, and a null C string equals only another null one, shown as `(null)`.

```
__int64 total = 5;
CheckEquals(5, total, ASWBCB_HERE, "int and __int64");
CheckEquals(L"caf\x00E9", std::string("caf\xC3\xA9"), ASWBCB_HERE, "wide and UTF-8 text");
String const caption = "Ready";
CheckEquals("Ready", caption, ASWBCB_HERE);
TAlignment const alignment = taCenter;
CheckEquals(taCenter, alignment, ASWBCB_HERE);
CheckEquals(-1, 4294967295u, ASWBCB_HERE, "fails, where -1 == 4294967295u is true");
// Check failed for: "Test_Fail_EqualsIntegers" (323): Expected "-1" but was "4294967295". fails, where -1 == 4294967295u is true
```

A failed `CheckNotEquals` shows the value both share, e.g. `Both values equal: "old".` Values of
other types, or of two kinds that don't mix, such as a `bool` and an integer, don't compile. There
are no floating-point overloads; use [`CheckNear`](#comparing-floating-point-values).

### Comparing Strings, Ignoring Case

`CheckEqualsIC`/`AssertEqualsIC` and `CheckNotEqualsIC`/`AssertNotEqualsIC` compare two strings like
`CheckEquals`/`CheckNotEquals`, but ignoring case. A failure shows both:

```
CheckEqualsIC("Content-Type", "Content-Length", ASWBCB_HERE, "header names ignore case");
// Check failed for: "Test_Fail_EqualsIC" (333): Expected "Content-Type" but was "Content-Length" (ignoring case). header names ignore case
```

Only the ASCII letters `A`-`Z` and `a`-`z` match regardless of case, so the result is the same in
every locale. Other letters, such as an accented capital and small E, still have to match exactly.
The substring, prefix and suffix checks' `IC` forms ignore case the same way.

### Checking for a Substring

`CheckContains`/`AssertContains` pass when a text contains a given substring, and
`CheckNotContains`/`AssertNotContains` when it doesn't. The text comes first, then the substring,
both strings of any types. Unlike `CheckTrue(Pos(...) > 0, ...)`, a failure shows both:

```
String const log = "Connected to server";
CheckContains(log, "timeout", ASWBCB_HERE, "logs the timeout");
// Check failed for: "Test_Fail_Contains" (339): Expected "Connected to server" to contain "timeout". logs the timeout
```

The comparison is case-sensitive, and every text contains the empty string.
`CheckContainsIC`/`AssertContainsIC` and `CheckNotContainsIC`/`AssertNotContainsIC` ignore the case
of ASCII letters. A null C string, as either argument, neither contains nor is contained in
anything, so `CheckContains` fails for it and `CheckNotContains` passes. The prefix and suffix
checks treat it the same way.

### Checking the Start or End of a String

`CheckStartsWith`/`AssertStartsWith` pass when a text starts with a given prefix, and
`CheckEndsWith`/`AssertEndsWith` when it ends with a given suffix. `CheckNotStartsWith`,
`CheckNotEndsWith` and their `Assert` forms pass when it doesn't, and the `IC` forms
(`CheckStartsWithIC`, ...) ignore the case of ASCII letters. They take strings as the
[substring checks](#checking-for-a-substring) do:

```
CheckEndsWith(L"logs\\server.txt", ".log", ASWBCB_HERE, "writes a log file");
// Check failed for: "Test_Fail_EndsWith" (344): Expected "logs\server.txt" to end with ".log". writes a log file
```

### Matching a Regular Expression

`CheckMatches`/`AssertMatches` pass when the *whole* of a text matches a regular expression, and
`CheckNotMatches`/`AssertNotMatches` when it doesn't. To match part of the text, start or end the
pattern with `.*`. The text and the pattern are strings of any types, and the syntax is PCRE's, as
the RTL's `TRegEx` takes it (ASWUnitTests uses ECMAScript's). A failure shows the text and the
pattern, and an invalid pattern fails the check, showing why:

```
CheckMatches("2026-1-7", "\\d{4}-\\d{2}-\\d{2}", ASWBCB_HERE, "an ISO 8601 date");
// Check failed for: "Test_Fail_Matches" (349): Expected "2026-1-7" to match "\d{4}-\d{2}-\d{2}". an ISO 8601 date
```

### Comparing Floating-Point Values

Exact comparison of floating-point values is unreliable (e.g. `0.1f + 0.2f != 0.3f`), so
`CheckNear`/`AssertNear` pass when two values differ by at most a tolerance, and
`CheckNotNear`/`AssertNotNear` when they differ by more:

```
float const sum = 0.1f + 0.2f;
CheckNear(0.3f, sum, 0.0001f, ASWBCB_HERE, "close to 0.3");
CheckNear(0.3, 0.35, 0.01, ASWBCB_HERE, "close to 0.3");
// Check failed for: "Test_Fail_Near" (361): Expected "0.300000 (tolerance 0.010000)" but was "0.350000 (diff 0.050000)". close to 0.3
```

Equal values, infinities included, are near; a NaN is near nothing. There's no default tolerance,
since a sensible one depends on the values and the computation. The three arguments must be all
`float` or all `double`: a mix, such as `CheckNear(2.0, aFloat, 0.1)`, or an integer argument is
ambiguous and doesn't compile.

### Comparing Greater Than and Less Than

`CheckGreaterThan`, `CheckGreaterThanOrEqual`, `CheckLessThan` and `CheckLessThanOrEqual`, and their
`Assert` forms, compare a value with a bound. The value comes first, then the bound, read as
"value >= bound", the other way round from `CheckEquals`, whose expected value comes first. Unlike
`CheckTrue(count >= 1, ...)`, a failure shows both:

```
CheckGreaterThanOrEqual(count, 1, ASWBCB_HERE, "at least one item");
// Check failed for: "Test_Fail_Ordering" (356): Expected 0 to be >= 1. at least one item
```

They take any two integer or floating-point types except `bool` and enums. Two integers are compared
by value, as [`CheckEquals`](#comparing-values) does, so `-1` is less than any unsigned value. When
either value is floating point, both are compared as their common type, as the built-in operators
do, and a NaN fails every check.

### Checking for Empty

`CheckEmpty`/`AssertEmpty` and `CheckNotEmpty`/`AssertNotEmpty` take a string of any type, or a
container: any class with `empty()` and `size()` members, such as a `std::vector`, a `std::map` or a
`TBytes` (`System::DynamicArray`). A failure shows the text, or the container's element count:

```
CheckEmpty(errors, ASWBCB_HERE, "no errors reported");
// Check failed for: "Test_Fail_Empty" (369): Expected empty but had 2 elements. no errors reported
CheckEmpty(String("low disk space"), ASWBCB_HERE, "no warning");
// Check failed for: "Test_Fail_EmptyText" (374): Expected empty but was "low disk space". no warning
```

A null C string is neither empty nor not empty, so both checks fail for it.

### Checking for Null

`CheckNull`/`AssertNull` and `CheckNotNull`/`AssertNotNull` compare a raw pointer, a
`std::unique_ptr` or an interface reference (`System::DelphiInterface`, e.g. `_di_IXMLNode`) with
`nullptr`:

```
std::unique_ptr<TStringList> list(new TStringList());
CheckNotNull(list, ASWBCB_HERE);
CheckNull(static_cast<TObject*>(nullptr), ASWBCB_HERE);
```

A C string is checked as a pointer, so an empty string such as `""` isn't null. `nullptr` itself, an
array or an integer doesn't compile.

### Comparing Identity

`CheckSame`/`AssertSame` and `CheckNotSame`/`AssertNotSame` check that two values do or don't point
to the same object. They take raw pointers, arrays, `std::unique_ptr` and interface references, in
any mix, and compare C strings by address too, unlike `CheckEquals`. A failure shows both addresses:

```
std::unique_ptr<TStringList> list(new TStringList());
TObject* object = list.get();
CheckSame(list, object, ASWBCB_HERE);
std::unique_ptr<TStringList> a(new TStringList());
std::unique_ptr<TStringList> b(new TStringList());
CheckSame(a, b, ASWBCB_HERE, "one shared list");
// Check failed for: "Test_Fail_Same" (394): Expected the same object as "0x161a3b0" but was "0x161a450". one shared list
```

### Checking an Object's Type

`CheckIsType<TType>`/`AssertIsType<TType>` check that an object is a `TType`, and
`CheckIsNotType`/`AssertIsNotType` that it isn't. They take a raw pointer, a `std::unique_ptr` or an
object, of a polymorphic class (any `TObject` descendant is one). They use `dynamic_cast`, so a
descendant of `TType` counts as one, and a null pointer is never one. A failure shows the expected
class and the object's class, by `ClassName()` for a `TObject` descendant:

```
std::unique_ptr<TStream> stream(new TMemoryStream());
CheckIsType<TStringStream>(stream, ASWBCB_HERE, "a string stream");
// Check failed for: "Test_Fail_IsType" (400): Expected type "TStringStream" but was "TMemoryStream". a string stream
```

With bcc32, `TType` and the pointer's class must be related, one derived from the other, since bcc32
rejects a `dynamic_cast` between unrelated RTL classes (E2031). A check between unrelated RTL
classes could never pass anyway; cast the pointer to `TObject*` first if you need one.

### Comparing Memory

`CheckEqualsMem`/`AssertEqualsMem` compare a number of bytes at two addresses, and
`CheckNotEqualsMem`/`AssertNotEqualsMem` check that they differ. A failure shows the offset of the
first differing byte, and up to 16 bytes from there in hexadecimal:

```
CheckEqualsMem(expected, actual, sizeof(expected), ASWBCB_HERE, "encoded header");
// Check failed for: "Test_Fail_Mem" (381): Bytes differ at offset 2 of 4: expected "03 04" but was "FF 04". encoded header
```

Zero bytes, or the same pointer twice, are always equal, without reading either. Exactly one null
pointer is never equal to the other memory, and isn't read either.

### Expecting an Exception

`SetExceptionExpected(true, ASWBCB_HERE, msg)` makes the rest of the test pass only if it throws an
exception of any type; `SetExceptionExpected(false, ...)` cancels that. To also check the
exception's type, and optionally that its message contains a given text, use the template form:

```
SetExceptionExpected<EStringListError>(ASWBCB_HERE, "an empty list", "out of bounds");

m_List->Delete(0);
```

The type may be an RTL exception class, a `std::exception` class or any other C++ type, and a
descendant of it matches too. The message checked is an RTL exception's `Message` or a
`std::exception`'s `what()`. An exception of another type, or without that text, fails the test
with what was caught, and so does no exception at all:

```
// Test failed: "Readme_Snippets.Fail_ExpectedWrong": expected exception type was not thrown (caught a different exception): EConvertError: 'abc' is not a valid integer value
```

An expected exception never hides a failure elsewhere in the test: a failed `Assert*` still fails
it, rather than counting as the expected exception, and so does a failed `Check*` earlier in the
test.

### Checking That a Call Throws

`SetExceptionExpected()` covers a test that ends by throwing. To check a single call instead, so the
test carries on afterwards, pass it to `CheckThrows`/`AssertThrows`, with the expected exception
type and, optionally, a text its message must contain. `CheckNoThrow`/`AssertNoThrow` check that a
call doesn't throw, and a failure shows what it threw.

The call is a callable without parameters: a closure to a method of the group (written
`DoDeleteFirst` or `&DoDeleteFirst`) or of another object (`&m_List->Clear`), a function, or a
functor object:

```
void __fastcall TTest_ReadmeSnippets::DoDeleteFirst()
{
    m_List->Delete(0);
}

// In a test:
CheckThrows<EStringListError>(DoDeleteFirst, ASWBCB_HERE, "empty list", "out of bounds");
CheckThrows<EConvertError>(ParseEmpty, ASWBCB_HERE);              // void ParseEmpty()
CheckThrows<Exception>(TNotANumberFunctor(), ASWBCB_HERE);        // A class with "void operator()() const"
m_List->Add("x");
CheckNoThrow(DoDeleteFirst, ASWBCB_HERE);
CheckEquals(0, m_List->Count, ASWBCB_HERE);
CheckNoThrow(&m_List->Clear, ASWBCB_HERE, "another object's method");
```

C++03 has no lambdas, so the code to call goes in a method of the group, which can use the group's
members. With bcc32, a closure to a method that returns a value must be written with `&`, and a
functor's class must be at namespace scope, not inside a function. A return value is ignored.

The type is matched as `SetExceptionExpected<TException>()` matches it. A failed `Assert*`, `Fail()`
or `Skip()` inside the callable isn't taken as the thrown exception; it ends the test as usual,
unless the requested type is that framework exception (e.g. `CheckThrows<TExceptTrue>`).

With bcc32c and bcc64x (Clang, which handles C++ exceptions synchronously), a `try` block in a test
catches an access violation only when it happens in a function the block calls, and in an optimized
bcc64x build not even then if the compiler can see that function's code (RSS-4778). `CheckThrows`
and `CheckNoThrow` call the callable out of line, so they catch it; so does the framework when a
test doesn't, failing the test as an unexpected exception.

### Failing a Test

`Fail(ASWBCB_HERE, msg)` fails the current test unconditionally, e.g. on a code path it should
never reach. Like an `Assert*` method, it ends the test, and it fails it even while an exception is
expected:

```
Fail(ASWBCB_HERE, "unexpected shape kind");
// Test failed: "Readme_Snippets.Fail_Fail": Failed: Test_Fail_Fail (423): unexpected shape kind
```

### Skipping a Test

`Skip(ASWBCB_HERE, reason)` ends the current test and reports it as skipped. Skipped tests are
counted separately from passed and failed ones and don't affect the exit code. No `return` is
needed afterward, since `Skip()` throws to unwind the rest of the test, as an `Assert*` method does:

```
Skip(ASWBCB_HERE, "needs a network connection");
// Test skipped: "Readme_Snippets.Skip": Test skipped: Test_Skip (444): needs a network connection
```

This keeps a known-broken, environment-specific or unfinished test in the group, and in `--list`,
instead of commenting it out and losing the reminder that it exists.

### Parameterized Tests

`RegisterTestCases()`, called from a group's constructor, registers one test per element of a
`std::vector<TParam>`, each calling the given method with its own copy of that element. Each row is
an independent, separately named test, so `--filter`, `--shuffle`, the reports and the GUI runner
treat it like any other. The method takes the row, `void __fastcall Test_X(TParam const& row)`, and
must not be published, since a published `Test_` method would also run as an ordinary test, without
a row:

```
struct THexCase
{
    String Input;
    int Expected;
};

// In the group's class, private:
void __fastcall Test_HexToInt(THexCase const& row);

// In its constructor:
std::vector<THexCase> hexCases;
THexCase row;
row.Input = "A";
row.Expected = 10;
hexCases.push_back(row);
row.Input = "ff";
row.Expected = 255;
hexCases.push_back(row);

RegisterTestCases(Test_HexToInt, "HexToInt", hexCases);

void __fastcall TTest_ReadmeSnippets::Test_HexToInt(THexCase const& row)
{
    CheckEquals(row.Expected, StrToInt("$" + row.Input), ASWBCB_HERE);
}
```

The tests run after the group's published tests, named `HexToInt[0]`, `HexToInt[1]`, and so on.
The method can also be given as `&TTest_Group::Test_HexToInt`, and an optional fourth argument, a
function `std::string Name(THexCase const& row)`, labels each row, e.g. `HexToInt[A]`, which reads
better in `--filter` patterns and CI output. A name used twice in a group stops the run at startup.
`tests\Test_ASWExample_WordCounter.cpp` registers one, `Count_Words`, with labeled rows.

## Notes for bcc32

The classic compiler has quirks that tests run into more than most code. The ones found while
building this framework:

- Read a VCL `__property` into a local variable before using it inside a larger expression: bcc32
  can mistype a property read nested in an expression, sometimes reporting an error far from the
  cause. Passing a property straight to a check, as in `CheckEquals(2, m_List->Count, ...)`, is
  fine.
- After one real error, bcc32 may report follow-on errors in unrelated templates, such as
  `Undefined symbol` for a `catch` variable in a framework header. Fix the first error first.
- `nullptr` works on every compiler: `ASWUnitTestsBCB_Types.h`, which the framework's headers
  include, defines it as `NULL` for bcc32, which predates C++11. On bcc32 it's then an integer
  constant, so don't overload a function on `std::nullptr_t`.
- A closure to another object's method is written `&obj->Method`.
- bcc32 accepts `enum class`, but only scopes the names: a value still converts to an integer, and
  compares with integers and with other enum types. bcc64x rejects all of those, so code that
  relies on them builds only with bcc32; convert explicitly (`static_cast<int>(color)`). bcc32
  doesn't accept a 64-bit underlying type (`enum class E : unsigned __int64`).
- An exception thrown while a test group's base class is being constructed, such as one from an
  argument of `inherited(...)`, reaches the framework as an access violation, so the group's tests
  fail with `EAccessViolation` rather than the real exception (bcc64x reports the real one). Throw
  from the group constructor's body instead.

## Adding ASWUnitTestsBCB to Your Projects

This repository's own projects (`rad370\ASWUnitTestsBCB.cbproj` and
`gui\rad370\ASWUnitTestsBCB_GUI.cbproj`) build only its own tests and the example in `toTest`. Don't
use or change them from your project: changes inside the framework's folder get lost or conflict
when you update it. Add ASWUnitTestsBCB to your repository as a git submodule instead (e.g. in
`third_party\asw-unit-tests-bcb`), and create a test project of your own that compiles its sources:

```
my-project/
+-- third_party/asw-unit-tests-bcb/   <- git submodule, never modified, freely updated
|   +-- src/                          <- the framework and the console runner
|   +-- gui/src/                      <- the VCL GUI runner, if you use it
|   `-- rad370/, gui/rad370/, tests/, toTest/, docs/   <- this framework's own projects and docs
+-- tests/                            <- your test groups (Test_MyClass.cpp/.h), self-registered
`-- tests/MyProjectTests.cbproj       <- your own test project
```

### A Console Test Project

1. In RAD Studio, create a C++ console application that uses the RTL (with the VCL as its
   framework, as this repository's project does), for each platform you test on: Win32 for bcc32 and
   bcc32c, Win64x for bcc64x.
2. Remove the `.cpp` file with `main()` that the wizard created, and add the submodule's `src\*.cpp`
   files to the project, `main.cpp` included. `main.cpp` refers to no test, so it's used as it is,
   and it builds whether the project's "TCHAR maps to" option is `char` or `wchar_t`.
3. Add the submodule's `src` folder to the project's include path, and add your test groups' files
   to the project, along with the code they test, or the libraries it lives in.
4. Build and run. The test program takes the [command line options](#command-line-options); add
   `--pause` to the project's run parameters to read the console before it closes.

The framework needs only the RTL, which a console project uses through `System.hpp`. With bcc32 and
bcc32c, a console program that uses the RTL starts with code that needs `vcle.lib` or `rtle.lib`,
which the build only links while the project's link-with-package-statics list is empty, and the IDE
fills that in when it saves the project options. `main.cpp` names the library itself
(`#pragma comment(lib, ...)`), so the link works either way.

Every file of the framework starts with `#include <System.hpp>` and `#pragma hdrstop`, so it works
with RAD Studio's automatic precompiled headers, or with none. If the wizard created a precompiled
header file (e.g. `MyProjectTestsPCH1.h`) and set the project to inject it into every file, the
framework's files get it too, which is harmless as long as it only includes RTL and standard
headers.

Each test group's file is compiled into the program, so the linker keeps it, although nothing refers
to it but its own registration. The test methods themselves are referred to by their class's type
information, so the linker keeps them too. Don't put test files in a static library that the test
program links: the linker would leave out every file nothing refers to.

To run the tests on a machine without RAD Studio, such as a CI agent, turn off linking with runtime
packages and with the dynamic RTL in the project options, so the program needs none of RAD Studio's
BPL and DLL files.

### A GUI Test Project

To run your tests in the [VCL GUI runner](#vcl-gui-runner), create a RAD Studio VCL application
instead, remove the form and the `WinMain()` the wizard created, and add:

- the submodule's `src\*.cpp` files except `main.cpp`;
- the files in `gui\src`, `ASWUnitTestsBCB_GUI_MainForm.dfm` included;
- your test groups and the code they test;
- a `WinMain()` modeled on `gui\rad370\ASWUnitTestsBCB_GUI.cpp`, which parses the command line,
  applies the saved theme and starts the main form;
- RAD Studio's Carbon style, for the dark theme, and any of Charcoal Dark Slate, Cobalt XEMedia,
  Onyx Blue and Tablet Dark for **Dark Theme** to offer (Project > Options > Application >
  Appearance, which this repository's project has as `Custom_Styles`); without Carbon, Dark shows
  the light look.

Add both `src` and `gui\src` to the include path. One test group file can be in a console and a GUI
project at the same time, so the same tests run in both.

### Updating

After updating the submodule, compare its `src` and `gui\src` folders with the files your test
projects list, and add or remove files to match: a new framework file missing from a project makes
its link fail. `CHANGELOG.md` says what changed.

# Coding Standards

This repository uses `uncrustify` with the `.uncrustify.cfg` configuration, enforced by a Git hook.
Enable it locally with:

```
git config --local core.hooksPath .githooks/
```

See [CONTRIBUTING.md](CONTRIBUTING.md) and [AGENTS.md](AGENTS.md) for the rest.

# License

Licensed under the [Apache License, Version 2.0](LICENSE).
