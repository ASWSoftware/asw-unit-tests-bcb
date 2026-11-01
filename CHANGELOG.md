# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- A dark theme in the VCL GUI runner, with **View > Theme** to choose it: **Use Windows Setting**
  (the default) follows Windows' app mode setting, switching as soon as it changes, but stays light
  while high contrast is on, and **Light** and **Dark** override it. The choice applies without a
  restart and is saved in the layout file; `--layout-ignore` and `--layout-reset` cover it as they
  do the layout. Dark is RAD Studio's Carbon VCL style, so a project of your own that runs its
  tests in the VCL GUI runner needs Carbon among its custom styles (Project > Options >
  Application > Appearance), without which Dark shows the light look, and a `WinMain()` that
  applies the saved theme (see Changed).
- **View > Theme > Dark Theme** in the VCL GUI runner chooses the dark look's colors from five of
  RAD Studio's dark VCL styles: Carbon (the default), Charcoal Dark Slate, Cobalt XEMedia, Onyx Blue
  and Tablet Dark. It lists the ones the executable links in as custom styles, and Carbon always.
  The choice is saved in the layout file with the theme.
- VCL style files (`.vsf`) as dark themes in the VCL GUI runner: **View > Theme > Dark Theme** also
  lists the files in `%APPDATA%\ASWUnitTestsBCB\Themes`, shared by every test app, and **Open
  Themes Folder** opens it. A style file's problems, such as a missing file or a style name already
  taken, are reported in the log, with Carbon used in its place.

### Changed

- A VCL GUI project of your own needs the two new files in `gui\src`,
  `ASWUnitTestsBCB_GUI_Style.cpp` and `ASWUnitTestsBCB_GUI_Theme.cpp`, and its `WinMain()` must
  load the saved theme and dark theme and apply them before creating the main form, then pass them
  to the form's `Start()`, which now takes them, the dark theme's VCL style and the problems found
  loading it as well as the options, as `gui\rad370\ASWUnitTestsBCB_GUI.cpp` does.
- A negative or NaN tolerance now fails `CheckNotNear`/`AssertNotNear` as well as
  `CheckNear`/`AssertNear`, whatever the values, with "Invalid tolerance ...: it must be zero or
  more". Before, the NotNear methods passed with one unless the values were equal, and the Near
  methods passed with one when they were. A test that passes a negative tolerance must pass its
  absolute value instead.

### Fixed

- The VCL GUI runner's tooltips for Run Selected and Copy Details showed their keyboard shortcut
  twice, e.g. "(F9) (F9)".
- The VCL GUI runner's progress bar, in the light theme, lagged one test behind the run once a test
  had failed and it turned red, so a run with a failure ended with it short of full, e.g. two thirds
  full after three failing tests. It now shows every finished test.
- With `--test-timeout-seconds`, in a Win64 (bcc64x) build, a test ran with the default
  floating-point environment instead of the one the run started with: rounding to nearest, whatever
  rounding mode was set, e.g. with `std::fesetround()`. Each test now starts with the floating-point
  environment of the thread that runs the tests, as it does without a timeout. Win32 builds already
  kept it.

## [1.0.0] - 2026-10-15

### Added

- Test groups whose tests are found, not registered. A group derives from `TTestGroupBase` (a
  `System::TObject` descendant, in `ASWUnitTestsBCB_TestBase.h`), and every method in its
  `__published` sections declared `void __fastcall Test_<Name>()` runs as the test `<Name>`, in
  declaration order, after the tests it inherits from ancestor groups. A test method a descendant
  publishes again runs in the ancestor's place. Other published methods aren't tests, and one class
  publishing a test name twice (overloads) stops the run at startup.
- `ASWBCB_REGISTER_TEST_GROUP(Class)` and `ASWBCB_REGISTER_TEST_GROUP_ORDERED(Class, order)`, from
  `ASWUnitTestsBCB_Registry.h`, register a group from its own `.cpp` file. Groups run by order, then
  alphabetically by name, and each group's tests run on one instance between `SetUp_Group()` and
  `TearDown_Group()`, with `SetUp_Test()` and `TearDown_Test()` around each test.
- Test suites, for organizing groups into a hierarchy of any depth.
  `TSuitePath("Framework.Console")` passed to the `TTestGroupBase` constructor, or a registration
  with `ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(Class, "Framework.Console")` (or its `_ORDERED` form),
  puts a group in suite `Console` inside suite `Framework`. The suite path starts its tests' full
  names, so `--filter "Framework.*"` runs a whole suite, and `--list`, the log, the records and the
  JUnit report show `Framework.Console.<group>`. A suite's groups run together, shuffled or not,
  and an order places a group among its own suite's groups and suites, where a suite sorts as order
  `0` by its own name. The VCL GUI runner shows each suite as a tree node with a check box and a
  status. A suite path with an empty suite name, or a registration and a constructor that name
  different suites, stops the run at startup with `TExceptInvalidSuitePath`.
- `CheckEquals`, `CheckNotEquals`, `AssertEquals` and `AssertNotEquals` for `bool`, integers of any
  types (compared by value, so `-1` never equals `0xFFFFFFFF`, and shown as numbers, `char` too),
  enums, `enum class` too (by their integer values, also against integers), pointers (by address)
  and strings of any C++ or RTL string type in any combination, with `IC` forms (`CheckEqualsIC`,
  ...) that compare two strings ignoring the case of the ASCII letters A-Z; `CheckTrue`,
  `CheckFalse`, `AssertTrue`, `AssertFalse`, `Fail()` and `Skip()`. A `Check*` failure lets the test
  go on and fails it at the end; an `Assert*` failure ends it. The location is `ASWBCB_HERE` or the
  explicit `__FUNC__, __LINE__`, and failure messages show the bare method name with every compiler.
  The message argument is optional and takes any string type.
- `CheckNear` and `CheckNotNear`, and their `Assert*` versions, compare two `float` or two `double`
  values with an absolute tolerance. Equal values (infinities too) are near; a NaN is near nothing.
- `CheckNull` and `CheckNotNull` for raw pointers, `std::unique_ptr` and interface references
  (`System::DelphiInterface`, e.g. `_di_IXMLNode`); `CheckSame` and `CheckNotSame`, which compare
  pointers, arrays, `std::unique_ptr` and interface references by address; and
  `CheckIsType<T>` and `CheckIsNotType<T>`, which check an object's class with `dynamic_cast` and
  name a `TObject` descendant by its `ClassName()`. Each has an `Assert*` version.
- `CheckGreaterThan`, `CheckGreaterThanOrEqual`, `CheckLessThan` and `CheckLessThanOrEqual`, and
  their `Assert*` versions, for integers and floating-point values in any mix. Integers compare by
  value (`-1` is less than `0xFFFFFFFF`); a NaN is in no order.
- `CheckContains`, `CheckStartsWith` and `CheckEndsWith`, with `Not` and `IC` forms
  (`CheckNotContainsIC`, ...) and `Assert*` versions, for a text and a part of any C++ or RTL string
  types.
- `CheckEmpty` and `CheckNotEmpty`, and their `Assert*` versions, for strings of any C++ or RTL
  string type and for containers (any class with `empty()` and `size()`, e.g. `std::vector` or
  `TBytes`). A failure shows the string or the container's element count.
- `CheckEqualsMem` and `CheckNotEqualsMem`, and their `Assert*` versions, compare a number of bytes
  at two addresses. A failure shows the offset of the first differing byte and up to 16 bytes from
  there in hexadecimal.
- `CheckThrows<T>` and `CheckNoThrow`, and their `Assert*` versions, call a closure to a test
  group's method (`DoConvert` or `&DoConvert`), a function or a functor object, and check whether it
  throws a `T` (an RTL exception class, a `std::exception` class or any other C++ type), optionally
  with a message substring. The test carries on afterwards, and a failed `Assert*`, `Fail()` or
  `Skip()` inside the callable still ends it.
- `CheckMatches` and `CheckNotMatches`, and their `Assert*` versions, check whether the whole of a
  text matches a regular expression, both strings of any C++ or RTL string type. The syntax is
  PCRE's, as the RTL's `TRegEx` takes it (`asw-unit-tests` uses ECMAScript). An invalid pattern
  fails the check, showing why.
- Parameterized tests: `RegisterTestCases()`, called from a group's constructor, registers one test
  per element of a `std::vector`, calling an unpublished `void __fastcall Test_X(TParam const& row)`
  (given as `Test_X` or `&TTest_Group::Test_X`) with its own copy of the row. The tests run after
  the published ones and are named `Base[0]`, `Base[1]`, ..., or with labels from an optional name
  function.
- `Log()` writes a line to the log, and `LogAppend()` writes text without ending the line. The text
  goes to the console, to the test's own log in the VCL GUI runner, and to an `ITestRunObserver`.
- An exception a test doesn't expect fails only that test, with `Unexpected exception: ` and its
  description (`ClassName: Message` for an RTL exception, access violations included).
  `SetExceptionExpected(true, ...)` expects any exception, and `SetExceptionExpected<T>(...)` one of
  type `T` (an RTL exception class, a `std::exception` class or any other C++ type), optionally with
  a message substring. A group whose constructor or `SetUp_Group()` throws fails each of its tests
  with that exception, and the run carries on.
- A console test runner (`src/main.cpp`) with `asw-unit-tests`' output and exit codes: `0` passed,
  `1` a test failed, `2` or `3` an exception from `SetUp_Test()`, `TearDown_Test()` or
  `TearDown_Group()` ended the run. The log starts with the version and whether the build is
  32-bit or 64-bit, e.g. `ASWUnitTestsBCB - Version 1.0.0 - 64-bit`. The console shows UTF-8 text
  correctly. `ITestRunObserver` receives the output and each test's result as the run goes.
- The console runner's command-line options, as in `asw-unit-tests`: `--help` (or `-h`),
  `--version`, `--project-name <name>` (or `--project-name=<name>`), which names the run in the
  `Initializing...` line, and `--pause`, which waits for a key press before the program ends. An
  unrecognized option or a missing value exits with code `4`. The arguments are read from the wide
  command line, so non-ASCII text in them arrives intact.
- The version, as the `ASWUNITTESTSBCB_VERSION_MAJOR`, `_MINOR`, `_PATCH`, `_PRERELEASE` and
  `_STRING` macros in `ASWUnitTestsBCB_Version.h`, for code that checks it with `#if`. `--version`,
  the VCL GUI runner's caption and `TTestHandler::GetVersionStr()` show it.
- `--filter <pattern>` runs only the tests whose `GroupName.TestName` full name matches a wildcard
  pattern (`*` for any run of characters, `?` for one, matching the whole name), and
  `--filter-ignore-case` ignores the case of the ASCII letters. `--list` lists the tests, filtered
  too, without running them. The log always says whether a filter is active. Other runners can pass
  their own `ITestFilter`, or `--filter`'s `TPatternTestFilter`, to `TTestHandler::Run()` and
  `ListTests()`.
- `--shuffle` runs the groups, and each group's tests, in a random order, logging the seed, and
  `--shuffle-seed <N>` reproduces a run's order. A seed gives the same order with every compiler,
  and the same order as `asw-unit-tests` gives the same tests. A filtered run keeps the order the
  seed gives the whole suite.
- `--partition-index <i>` and `--partition-count <n>` split the suite into `n` partitions by each
  test's position in the unshuffled order, so `n` processes (e.g. CI jobs) run it in parallel, each
  test in exactly one of them, shuffled or not. They combine with `--filter` and `--list`.
  `TTestHandler::GetAllTestFullNames()` returns that order, and `GetTests()` the same tests with
  their group and test names apart (`TTestId`).
- Colored pass, fail and skip status text on the console: `--color auto` (the default; only when
  the output is a console that shows colors and `NO_COLOR` isn't set), `--color always`,
  `--color never` or `--no-color`, and `--color-pass`, `--color-fail` and `--color-skip` to pick
  the colors. An `ITestRunObserver` always receives the text without color codes.
- Each test's duration, from before `SetUp_Test()` to after `TearDown_Test()`, is shown on its
  `Finished test` line in milliseconds with three decimals and recorded in
  `TTestCaseRecord::DurationSeconds`, with every compiler measuring a test far shorter than a
  millisecond correctly.
- `--test-timeout-seconds <N>` runs each test on a worker thread and abandons one that runs longer,
  recording it as failed, as `asw-unit-tests` does: the run ends after its group is torn down, with
  exit code `5`. A test runs the same on the worker, and its `TThread::Synchronize()` and `Queue()`
  calls still reach the main thread. An abandoned test that ends after all changes no results.
  `TTestHandler::Run()` and `TTestGroupBase::Run()` take the timeout too.
- `--catch-crashes` records a test that causes a stack overflow or an illegal instruction as failed
  (`Test crashed: stack overflow.`) instead of letting it end the process, as `asw-unit-tests` does;
  a stack overflow then ends the run, with exit code `6`. The crashed test is abandoned where it
  faulted, without `TearDown_Test()` or destructors. Access violations and the other faults the RTL
  turns into exceptions are left to it: they fail just the test, and a test can expect them.
- `nullptr` works with every compiler: `ASWUnitTestsBCB_Types.h`, which the framework's headers
  include, defines it as `NULL` for bcc32, which predates C++11.
- `TStdOutRedirect` (`ASWUnitTestsBCB_StdOutRedirect.h`) captures what code under test writes to
  `std::cout`, as in `asw-unit-tests`, and can stop and resume capturing.
- `--report-junit <path>` writes a JUnit-style XML report of the run, as `asw-unit-tests` does,
  which most CI systems read natively; `--project-name` names its `<testsuites>`. The file is UTF-8,
  and its path may hold any characters. `TJUnitReportWriter` (`ASWUnitTestsBCB_JUnitReport.h`)
  builds or writes the same report for other runners.
- `rad370/ASWUnitTestsBCB.cbproj` builds the runner with bcc32 (Win32), bcc32c and bcc64x (Win64x),
  with a build script for each compiler and configuration.
- `Deploy.bat <folder>` copies the framework, the GUI runner, the tests, the projects, their build
  scripts and the docs to a folder, as in `asw-unit-tests`, e.g. to stage a copy for another
  repository. It leaves out the files only contributors to this repository use.
- A VCL GUI runner (`gui/rad370/ASWUnitTestsBCB_GUI.cbproj`), as in `asw-unit-tests`: a tree of
  groups and tests with check boxes and status dots, which View > Expand All and View > Collapse All
  open or close, a filter box, the selected test's details and log, a list of failures and skips,
  Run Selected, Run Failed and Stop, shuffled runs with a chosen or random seed, and JUnit export.
  It takes the console runner's options plus `--run`, `--exit` (exit with the console runner's
  code), `--layout-ignore` and `--layout-reset`, and remembers its window layout and the checked
  tests between sessions. After a test times out or a stack overflow ends a run, it runs no more
  tests until restarted.

[Unreleased]: https://github.com/ASWSoftware/asw-unit-tests-bcb/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/ASWSoftware/asw-unit-tests-bcb/releases/tag/v1.0.0
