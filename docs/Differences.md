# Differences from ASWUnitTests

ASWUnitTestsBCB is a sibling of [ASWUnitTests](https://github.com/ASWSoftware/asw-unit-tests), not
a port of its code. The two share concepts, names, command-line options, output and report formats,
but ASWUnitTestsBCB is written in C++03 for RAD Studio's classic compiler (bcc32), and finds tests
in `__published` sections instead of having them registered. This page lists every ASWUnitTests
feature and what ASWUnitTestsBCB does with it, compared with ASWUnitTests 1.5.0:

- **Same** - works the same way, with the same names and output.
- **Changed** - available, but works differently; the notes say how.
- **Not in this edition** - left out; the notes say why, and what to use instead.
- **Only here** - something ASWUnitTestsBCB does that ASWUnitTests doesn't.

To move tests from ASWUnitTests, see [Porting from ASWUnitTests](PortingFromASWUnitTests.md).

## Writing Tests

| Feature | Status | Notes |
|---|---|---|
| Test group base class `TTestGroupBase` | Changed | Derives from `System::TObject`, so its constructor and destructor are `__fastcall` and groups are created with `new`. |
| Registering test methods with `RegisterTest()` in the constructor | Not in this edition | Every `void __fastcall Test_<Name>()` in a `__published` section is a test named `<Name>`, found through the RTL's type information. Nothing to register, so nothing to forget. |
| Self-registering test methods (`ASW_TEST`, `TAutoTestGroup`) | Not in this edition | Published methods already register themselves, without macros or a template base. |
| Test order | Changed | Declaration order of the published methods, ancestors' tests first, then parameterized tests in registration order. ASWUnitTests runs `ASW_TEST` tests alphabetically, then registered ones in registration order. |
| Inherited tests | Only here | A group runs the tests its ancestors publish, and a descendant can replace one by publishing the same method again. |
| `ASW_REGISTER_TEST_GROUP`, `ASW_REGISTER_TEST_GROUP_ORDERED` | Changed | Named `ASWBCB_REGISTER_TEST_GROUP` and `ASWBCB_REGISTER_TEST_GROUP_ORDERED`; same placement, ordering and alphabetical default. |
| Test suites (`TSuitePath`, `GetSuitePath()`, `GetQualifiedTestGroupName()`, `SuitePath` in `TTestCaseRecord` and `TTestId`) | Changed | The same suite paths, qualified names, run order and shuffled order. An invalid suite path stops the run at startup with `TExceptInvalidSuitePath` and exit code `2`, thrown by `TTestHandler::Initialize()` rather than by `TSuitePath`'s constructor: bcc32 can't deliver an exception thrown while a group's base class is constructed. The two methods belong to `TTestGroupBase`, since there's no `ITestGroup`. |
| `ASW_REGISTER_TEST_GROUP_IN_SUITE`, `ASW_REGISTER_TEST_GROUP_IN_SUITE_ORDERED` | Changed | Named `ASWBCB_REGISTER_TEST_GROUP_IN_SUITE` and `ASWBCB_REGISTER_TEST_GROUP_IN_SUITE_ORDERED`. The registration's suite also holds the stand-in for a group whose constructor throws. |
| `SetUp_Group()`, `TearDown_Group()`, `SetUp_Test(ITestCase&)`, `TearDown_Test(ITestCase&)` | Same | |
| A group's constructor or `SetUp_Group()` throws | Changed | Each of the group's tests fails with that exception, and the run carries on (exit code `1`), as in ASWUnitTests. For a constructor, ASWUnitTests can't list the group's tests without an instance, so it fails one stand-in test, `(construction)`, instead. |
| Duplicate test names | Changed | Only possible as published overloads, or a parameterized name used twice; either stops the run at startup, as in ASWUnitTests. |
| Location arguments `__func__, __LINE__` | Changed | `ASWBCB_HERE`, or `__FUNC__, __LINE__` written out. Failure messages show the bare method name with every compiler. |
| Overloads without location arguments (`std::source_location`, C++20) | Not in this edition | C++03 has no `std::source_location`; `ASWBCB_HERE` is the short form. |
| Message arguments | Changed | Optional, and of any C++ or RTL string type (`char const*`, `std::string`, `wchar_t const*`, `std::wstring`, `String`, `AnsiString`, `UTF8String`). ASWUnitTests takes a `std::string`, and requires it for `Fail()` and `Skip()`. |
| `Log()`, `LogAppend()` | Same | |
| `Fail()`, `Skip()` | Same | The message or reason is optional here. |
| `SetExceptionExpected(bool, ...)` | Same | |
| `SetExceptionExpected<T>()` | Changed | `T` may be any C++ type, as in ASWUnitTests. A class that isn't a `std::exception` doesn't match a `std::exception` subclass derived from it; ASWUnitTests matches it and checks its `what()`. A thrown object that is neither a `std::exception` nor an RTL exception is described as "an object that is neither a std::exception nor an RTL Exception", where ASWUnitTests says "a non-std::exception object", or names its type when a message was expected (`an object of type "int"`). |
| RTL exception support (`ASWUNITTESTS_RTL_EXCEPTIONS`) | Changed | Always on, with every compiler, bcc32 included; there's nothing to define. |
| `TExceptRTLException` (an RTL exception crossing from a timeout's worker thread) | Not in this edition | An exception that ends the run on the worker arrives on the waiting thread as a `TExceptFatalError` with the same text and exit code. |
| Parameterized tests (`RegisterTestCases()`) | Changed | The method is a closure to an unpublished `void __fastcall Test_X(TParam const& row)` (`Test_X`) or a member pointer (`&TTest_Group::Test_X`), and the optional row namer is a function pointer, `std::string (*)(TParam const&)`, since C++03 has no `std::function`. |
| Lambdas for `CheckThrows`, `CheckNoThrow` and `RegisterTest()` | Changed | C++03 has no lambdas. `CheckThrows` and `CheckNoThrow` take a closure to a method of the group or another object, a function, or a functor object. |

## Checks and Asserts

Every `Check*`/`Assert*` family of ASWUnitTests is here, with the same names, argument order and
failure texts.

| Feature | Status | Notes |
|---|---|---|
| `Equals`/`NotEquals` for `bool`, integers of any types, pointers | Same | |
| `Equals`/`NotEquals` for enums | Changed | Any enum, `enum class` too, compares by its integer value, with another enum or an integer. ASWUnitTests compares an `enum class` only with one of the same type; bcc32's `enum class` converts to an integer like a plain enum, so the two can't be told apart there. |
| `Equals`/`NotEquals` for strings | Same | Any two of the C++ and RTL string types, e.g. a `std::string` with an `L"..."` literal, or an `AnsiString` with a `UTF8String`. Two narrow or two wide C++ strings are compared as they are, any other pair as Unicode here and as UTF-8 there, with the same result. The RTL types need `ASWUNITTESTS_RTL_EXCEPTIONS` in ASWUnitTests. |
| `EqualsIC`/`NotEqualsIC`, `Contains`, `StartsWith`, `EndsWith` (and their `Not` and `IC` forms) | Same | Any two string types, as `Equals`. A null C string, as either argument, never matches a substring, prefix or suffix check, and equals only another null one in `EqualsIC`, whatever the other argument's type. |
| `Matches`/`NotMatches` | Changed | PCRE syntax, from the RTL's `TRegEx`, where ASWUnitTests uses ECMAScript (`std::regex`, `std::wregex`). Every text is matched as UTF-16, so `.` is one character; ASWUnitTests matches two narrow texts byte by byte. |
| `Near`/`NotNear` | Changed | All three arguments `float` or all `double`; a mix, or an integer argument, is ambiguous and doesn't compile. ASWUnitTests takes any three integer or floating-point types but `bool`, in any mix, `long double` too, compared in their common type, and three integers exactly. A negative or NaN tolerance fails all four in both. |
| `GreaterThan`, `GreaterThanOrEqual`, `LessThan`, `LessThanOrEqual` | Same | |
| `Empty`/`NotEmpty` | Changed | Containers need `size()` as well as `empty()`; ASWUnitTests needs only `empty()`, and shows the element count when there's a `size()`. C strings, character arrays, `AnsiString` and `UTF8String` work in both, and a null C string fails both checks. |
| `Null`/`NotNull` | Changed | Raw pointers, `std::unique_ptr` and interface references (`System::DelphiInterface`); not `std::shared_ptr` or `std::function`, which bcc32 doesn't have. |
| `Same`/`NotSame` | Changed | Raw pointers, arrays, `std::unique_ptr` and interface references; not `std::shared_ptr`. |
| `IsType`/`IsNotType` | Changed | Not for `std::shared_ptr`. With bcc32 the two classes must be related. With bcc32c, a class that isn't a `TObject` descendant is named in a failure as the compiler mangles it; ASWUnitTests shows it as written. |
| `EqualsMem`/`NotEqualsMem` | Same | |
| `Throws`/`NoThrow` | Changed | The callables above instead of lambdas. As in ASWUnitTests, the expected type may be any C++ type, and a class that isn't a `std::exception` matches a `std::exception` subclass derived from it, but here its `what()` isn't checked: it counts as an exception without a message. An object of any other type is described as in `SetExceptionExpected<T>()`. |
| `True`/`False` | Same | |

## Running Tests

| Feature | Status | Notes |
|---|---|---|
| Console runner and its output | Changed | The banner's first line names the edition and the build's bits, without the operating system: `ASWUnitTestsBCB - Version 1.1.0 - 64-bit`, where ASWUnitTests shows `ASWUnitTests - Version 1.5.0 - 64-bit - Windows`. The rest is the same. |
| `TTestHandler::GetAppBitsStr()`, `GetBuildOS()` | Not in this edition | The banner shows the bits, which code can tell from `_WIN64`; the operating system is always Windows. |
| Exit codes `0` to `6` | Same | |
| `--filter`, `--filter-ignore-case`, `--list` | Same | |
| `--shuffle`, `--shuffle-seed` | Same | The same seed gives the same order as in ASWUnitTests. |
| `--partition-index`, `--partition-count` | Same | |
| `--test-timeout-seconds` | Changed | Runs each test on a `TThread`; the test's `TThread::Synchronize()` and `Queue()` calls still reach the main thread. |
| `--catch-crashes` | Changed | Takes only a stack overflow and an illegal instruction, the faults that would end the process. Access violations and other faults the RTL turns into exceptions fail just the test with or without it, and a test can expect them. ASWUnitTests also takes access violations, divisions by zero and the POSIX signals. |
| `--color`, `--no-color`, `--color-pass`, `--color-fail`, `--color-skip`, `NO_COLOR` | Same | The `auto` mode checks standard output again when it changes. |
| `--report-junit`, `--project-name` | Changed | The default name is `ASWUnitTestsBCB`. The XML is the same. |
| `--pause` | Changed | Prompts "Press any key to continue" (the `pause` command). |
| `--help`, `-h`, `--version` | Same | |
| Command-line arguments | Same | Read from the UTF-16 command line, so non-ASCII text arrives intact, and `src/main.cpp` defines `_tmain`, as there. |
| Console code page | Same | Set to UTF-8 while the runner runs, and restored afterwards. |
| `TConsoleUTF8Output`, `UTF8ToWide()`, `CommandLineArguments()`, `SystemErrorText()` | Not in this edition | `src/main.cpp` sets the code page with a class of its own. For the others, use this edition's `FromUTF8()` and `TCLIParser::SplitCommandLine(::GetCommandLineW())`, and the RTL's `SysErrorMessage()`. |
| Final message after a fatal error | Changed | Names the method and the test (e.g. `SetUp_Test() exception for "Group.Test": ...`). |
| Per-test timing | Same | |
| `ITestRunObserver` | Same | |
| `TStdOutRedirect` | Same | |
| `TJUnitReportWriter` | Same | `Write()` takes the path as UTF-8, as there. |

## VCL GUI Runner

| Feature | Status | Notes |
|---|---|---|
| Test tree with suite nodes and Expand All/Collapse All, detail pane, failures, log, filter box, Run Selected/Failed, Stop, shuffle options, JUnit export, saved layout and selection | Same | The files live in `%APPDATA%\ASWUnitTestsBCB\`. |
| GUI-only options `--run`, `--exit`, `--layout-ignore`, `--layout-reset` | Same | |
| View > Theme (Use Windows Setting, Light, Dark), applied at once, following Windows' app mode and high contrast | Same | Saved in the layout file, and covered by `--layout-ignore` and `--layout-reset`, as there. |
| View > Theme > Dark Theme: Carbon, Charcoal Dark Slate, Cobalt XEMedia, Onyx Blue and Tablet Dark | Same | Each listed when the executable links it in as a custom style, and Carbon always. |
| VCL style files (`.vsf`) as dark themes, Open Themes Folder | Same | The folder is `%APPDATA%\ASWUnitTestsBCB\Themes`. |
| After a timeout or a crash that ends a run | Same | The GUI runs no more tests until restarted, and exits without destroying anything, since the abandoned test may still be running. |
| Defining `ASWUNITTESTS_RTL_EXCEPTIONS` for the GUI | Not in this edition | Nothing to define. |
| Win32 GUI runner (`ASWUnitTests_Win32_GUI`), with its INI theme files, the Carbon tints and Save Theme Template... | Not in this edition | The VCL GUI runner only, since everything here needs RAD Studio anyway. |

## Compilers and Building

| Feature | Status | Notes |
|---|---|---|
| C++ standard | Changed | C++03 (ASWUnitTests needs C++17). `nullptr` works with bcc32, where `ASWUnitTestsBCB_Types.h` defines it. |
| bcc32 (classic Win32 compiler) | Only here | ASWUnitTests doesn't build with it. |
| bcc32c, bcc64x | Same | |
| MSVC, MinGW, GCC, Clang, Linux | Not in this edition | ASWUnitTestsBCB is for C++Builder only, and needs the RTL. Use ASWUnitTests. |
| CMake project, `ASWUnitTests_Sources.cmake` (`ASWUNITTESTS_SOURCES`, `ASWUNITTESTS_CORE_SOURCES`) and the GUI runners' source lists | Not in this edition | RAD Studio projects only. |
| Version macros | Changed | `ASWUNITTESTSBCB_VERSION_*` macros, without the namespace constants. |
| `Deploy.bat` | Same | |
| Icon (`resource\ASWUnitTests.ico`) | Not in this edition | The projects use RAD Studio's default icon. |
