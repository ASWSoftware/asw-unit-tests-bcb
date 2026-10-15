# AGENTS.md

## Scope

These instructions apply to the entire repository unless a more specific `AGENTS.md` is added in a subdirectory. Keep changes focused on the requested behavior and preserve existing public APIs unless a change is necessary.

## Repository Overview

ASWUnitTestsBCB is a C++Builder-only unit-testing framework for RAD Studio, written in the C++03 subset that RAD Studio's classic Win32 compiler (bcc32) and its Clang-based compilers (bcc64x, bcc32c) all accept. Test groups derive from an RTL class, and their test methods are discovered from `__published` sections (as DUnit does) instead of being registered by hand.

- `src/` contains the framework implementation.
- `tests/` contains the framework's own tests, written with the framework (`Test_ASWUnitTestsBCB_*`), and an example test group (`Test_ASWExample_WordCounter`). Tests of the framework's behavior run small unregistered groups (directly, or through a `TTestHandler` given an unregistered `TTestGroupRegistrar` list) and check their recorded results, so the program's own run never includes them.
- `toTest/` contains example code under test for the example test group.
- `rad370/` contains the RAD Studio project that builds the framework's unit-test executable, and its build scripts.
- `gui/` contains the VCL GUI runner: `gui/src/` its units (`ASWUnitTestsBCB_GUI_*`; the ones without VCL UI code are
  tested by `tests/Test_ASWUnitTestsBCB_GUI_*` in the console executable), and `gui/rad370/` its RAD Studio project
  and build scripts.
- `README.md` contains usage and integration examples.
- `docs/` contains longer documentation the README links to: the differences from `asw-unit-tests`
  and the porting guides from DUnit and `asw-unit-tests`.
- `.uncrustify.cfg` and `.githooks/` define the repository formatting workflow.

Describe new top-level folders here when they're added.

It is a sibling of [ASWUnitTests](https://github.com/ASWSoftware/asw-unit-tests) (`asw-unit-tests`, C++17), not a fork. The two share concepts, names, command-line options, output formats and documentation style, but no code. Use `asw-unit-tests` as the reference for behavior, names and wording when porting a feature, and decide what carries over, what changes for C++03 and the RTL, and what is dropped. Never change `asw-unit-tests` from work in this repository, and never give it C++03 code paths.

## Accuracy-First Engineering

Contributors and AI coding agents are expected to be expert, skeptical, and careful. Verify assumptions by inspecting nearby code, call sites, tests, project files, and relevant history or documentation when needed. Neither the human contributor nor the AI is infallible; treat requirements and proposed fixes as hypotheses to check, and prefer evidence from a focused build or test over intuition. This matters doubly here: the three compilers disagree about C++ in ways that only a build on each of them shows.

Before changing behavior:

1. Identify the code path that owns the behavior.
2. State or test a concrete hypothesis about the cause or intended result.
3. Make the smallest change that addresses the cause.
4. Run the narrowest relevant validation, then broader checks when practical.

Do not hide unrelated failures by changing tests or weakening diagnostics. Do not revert changes that were already present unless explicitly asked.

## Language and Toolchain Compatibility

New and modified code must compile as C++03 in the subset that the classic compiler (bcc32) accepts, and build without warnings on every required compiler:

| Compiler | Platform | Status |
|---|---|---|
| bcc32 (classic) | Win32 | Required |
| bcc64x (Clang, modern) | Win64x | Required |
| bcc32c (Clang, C++17) | Win32 | Supported |
| bcc64 (Clang, legacy Win64) | Win64 | Not supported |

RAD Studio 13.1 is the minimum supported version.

- Don't use C++11 or later features (`auto` type deduction, lambdas, rvalue references, `static_assert`, `override`, `constexpr`, range-based `for`, inline variables) even where bcc64x and bcc32c accept them. Use a feature only after it's been shown to work on bcc32 and recorded as such. The exceptions are `nullptr`, which `src/ASWUnitTestsBCB_Types.h` defines as `NULL` for bcc32, and `enum class`.
- Declare enums as `enum class`. bcc32 accepts it but only scopes the names: its values still convert implicitly to integers, and compare with integers and with other enum types, all of which bcc64x and bcc32c reject. Convert explicitly (`static_cast<int>(color)`), and rely on the bcc64x build to catch a missed conversion. bcc32 doesn't accept a 64-bit underlying type (`enum class E : unsigned __int64` is error E2552).
- Don't use C++03 features that later standards removed, since bcc64x compiles as C++23 and bcc32c as C++17: `std::auto_ptr`, `std::random_shuffle`, `std::bind1st`/`std::bind2nd`, `std::ptr_fun`/`std::mem_fun`, the `register` keyword. Use `std::unique_ptr` instead of `std::auto_ptr`: it works on all three compilers (including `std::move`, arrays, custom deleters and `TObject` descendants), but bcc32's standard containers can't hold it, since they copy their elements. Prefer `static const` class members to anonymous-enum constants, since arithmetic between those warns in C++20 and later.
- Neither `std::tr1` nor `std` offers `function` or `shared_ptr` on bcc32, and bcc64x has no `std::tr1` at all.
- The RTL is allowed and preferred where it replaces a missing C++03 library feature (e.g. `System::Diagnostics::TStopwatch`, `System::Classes::TThread`, `System::Ioutils::TFile`). The framework itself needs only the RTL; the VCL is used only by the GUI runner.
- C++Builder language extensions (`__published`, `__fastcall`, `__closure`, `__property`) are part of the design, not portability problems.
- Put compiler-specific code behind a narrow check for that one compiler, leaving the other compilers' paths untouched. bcc32 is `defined(__BORLANDC__) && !defined(__clang__)`; bcc64x also defines `__MINGW32__`, so code guarded by `__MINGW32__` builds there too.
- Keep headers self-contained and avoid relying on transitive includes.
- Write a null pointer as `nullptr`, never `NULL` or `0`, and include `ASWUnitTestsBCB_Types.h` in each file that uses it. On bcc32 it's `NULL`, an integer constant, so don't overload on `std::nullptr_t` or count on `nullptr` not matching an integer parameter.
- Compare a pointer explicitly against `nullptr` (`if (p != nullptr)`) rather than relying on its conversion to `bool`.
- An RTL exception (`System::Sysutils::Exception` and descendants) is freed when the first handler that catches it ends, so never rethrow and catch one again inside a `catch` handler (the "classify the current exception" idiom crashes). Catch each exception exactly once, with ordinary typed handlers.

Keep all source files (`.cpp`/`.h`) plain ASCII, including comments, so that no compiler's assumption about the source character set can change a string's value. If a string needs a specific non-ASCII character, use a numeric escape instead of typing the character literally (e.g. `"\xC3\xA9"` for UTF-8 bytes, or `L"\x00E9"` for a wide-string code point), splitting adjacent escapes into separate concatenated string literals if needed so one escape doesn't consume a following hex digit.

## Code Organization

- Insert new class methods in alphabetical order within their existing section. If the class has named or visibly separated sections, preserve those sections and alphabetize only within the relevant section.
- Insert new free functions in alphabetical order within their existing section. Apply the same section rule when functions are grouped.
- Follow the surrounding naming, indentation, brace, include, and comment style. Avoid unrelated formatting changes.
- A `.cpp` file that uses the RTL (every one in this repository) starts with `#include <System.hpp>` and `#pragma hdrstop`, before its other includes, so `<System.hpp>` (which prepares and includes `<windows.h>`) comes first in every translation unit and marks where a precompiled header would end. Headers never use `#pragma hdrstop`, and still include the RTL headers they need.
- In a `.cpp` file, refer to RTL names unqualified (`String`, `TObject`, `Exception`): after the includes, a block of `using namespace` directives names `System` and each `System::` namespace the file uses (e.g. `using namespace System::Sysutils;`), alphabetically, with `using namespace ASWUnitTestsBCB;` first in a file outside that namespace. Headers never use `using namespace` and always qualify RTL names (`System::String`).
- Order a source file's includes, after that, in blocks separated by a `//---...` line: its own header (in a `.cpp`), then the C++ standard library headers, then the RTL and Windows headers, then this repository's headers, alphabetically within each block. In the RTL and Windows block, `<windows.h>` (and other Windows SDK headers) come after the `<System.*.hpp>` headers: the RTL headers prepare the SDK headers before including `<windows.h>` themselves, and with `<windows.h>` first, a bcc32 Release build (inlining on) reports dozens of W8026 warnings from inside the SDK headers. A file whose own header already includes the RTL still lists `<windows.h>` in that block, after the standard library headers.
- Write acronyms in class, file and function names in uppercase (`CLI`, `GUI`, `RTL`, `UTF8`, `XML`), never title case. `JUnit` is a name, not an acronym.
- Keep declarations and definitions consistent, and update all relevant project or build files when adding, removing or renaming source files, including `Deploy.bat`, which copies an explicit list of files.
- Add or update focused tests for behavior changes, including error and compiler-specific cases where applicable.

## Building and Testing

`rad370\ASWUnitTestsBCB.cbproj` builds the console unit-test executable `ASWUnitTestsBCB.exe` for three compilers from one project. Its Win32 platform uses the classic compiler (bcc32, RAD Studio's default), as the IDE builds it; the `Build_Win32Clang_*.bat` scripts switch it to bcc32c and give that build its own folders with msbuild properties. Win64x uses bcc64x. Each compiler and configuration has its own script and output folder:

| Compiler | Scripts | Executable |
|---|---|---|
| bcc32 | `rad370\Build_Win32_Debug.bat`, `rad370\Build_Win32_Release.bat` | `rad370\Win32\<Config>\ASWUnitTestsBCB.exe` |
| bcc32c | `rad370\Build_Win32Clang_Debug.bat`, `rad370\Build_Win32Clang_Release.bat` | `rad370\Win32Clang\<Config>\ASWUnitTestsBCB.exe` |
| bcc64x | `rad370\Build_Win64x_Debug.bat`, `rad370\Build_Win64x_Release.bat` | `rad370\Win64x\<Config>\ASWUnitTestsBCB.exe` |

`gui\rad370\ASWUnitTestsBCB_GUI.cbproj` builds the VCL GUI runner, `ASWUnitTestsBCB_GUI.exe`, which runs the same self-tests, the same way: `gui\rad370\Build_<Platform>_<Config>.bat` writes it to `gui\rad370\<Platform>\<Config>\`. `rad370\ASWUnitTestsBCB_Group.groupproj` opens both projects in the IDE. A source file the framework or its tests gain belongs in both projects, and in `Deploy.bat`. `ASWUnitTestsBCB_GUI.exe --run --exit --report-junit <file>` runs the suite unattended (start it with `Start-Process -PassThru` and wait, since a GUI process doesn't block the shell); its forms are otherwise tested by hand.

Change the project's settings in the IDE, not by hand: the IDE rewrites the whole file when it saves, silently drops what it doesn't recognize (such as a hand-written conditional `PropertyGroup`), and crashes on some hand-written files. Adding or removing a `CppCompile` item by hand, in the IDE's format, is fine. Settings only one script needs belong in that script.

The `Rad370` environment variable must point to the RAD Studio installation (e.g. `C:\Program Files (x86)\Embarcadero\Studio\37.0`). Run build scripts from PowerShell or `cmd.exe` by their full path (e.g. `& cmd.exe /c "<repo>\rad370\Build_Win32_Debug.bat"`); they work from any current directory and return msbuild's exit code. Only the compiler's warnings count: MSBuild's MSB4056 warning (launched from an MTA thread) is noise. Before considering a change complete:

- Build it with bcc32 and bcc64x, and with bcc32c when the change touches compiler-specific code.
- Check that each build reported no errors before running its executable, so you don't test a stale one, and that it added no warnings.
- Run the unit-test executable from each build. Tests should be run outside the debugger unless debugging an expected exception is intentional.
- Build Release with bcc32 too when a change adds or changes code in headers (templates, inline functions): its inline-expansion warnings (e.g. W8026, W8027) only appear with inlining on.

## Changelog

`CHANGELOG.md` follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and Semantic Versioning.

- Record each notable change under `## [Unreleased]`, in the matching `### Added`, `### Changed`, `### Deprecated`, `### Removed`, `### Fixed`, or `### Security` section.
- Describe the effect on someone using the framework, not the commit. For a breaking change, say what existing code must change.
- Record each change as its own bullet.
- The version lives only in `src/ASWUnitTestsBCB_Version.h` (the `ASWUNITTESTSBCB_VERSION_*` macros). `main` carries only release versions, with an empty `ASWUNITTESTSBCB_VERSION_PRERELEASE`. Between releases, `develop` carries the next planned version with a pre-release such as `dev.1` (e.g. `0.2.0-dev.1` after `0.1.0`), which sorts before that release; bump its number only to tell dev builds apart, and never bump the patch number on `develop`.
- When preparing a release, set the release version in `src/ASWUnitTestsBCB_Version.h` (empty pre-release; the number develop forecast may change, e.g. to a patch release or the next major), rename `[Unreleased]` to `[x.y.z] - YYYY-MM-DD`, start a new empty `[Unreleased]`, and update the compare links at the bottom of the file. After merging the release into `develop`, move `develop` to the next minor version with pre-release `dev.1`.

## Formatting and Review

Use the repository's `.uncrustify.cfg` configuration. The README documents how to enable the local Git hook with:

```text
git config --local core.hooksPath .githooks/
```

The hook needs the patched `uncrustify.exe` that the maintainer uses; a stock release may format some code differently. It leaves `__published:`, `__fastcall`, `__closure` and `__property` alone.

Source files, project files, scripts and docs use CRLF line endings. Some editing tools write LF; check new and edited files (e.g. compare `wc -l < file` with `tr -cd '\r' < file | wc -c`) and convert them before committing.

Wrap Markdown prose at 100 columns in `README.md`, `CONTRIBUTING.md`, `CHANGELOG.md` and `docs/`, re-wrapping the edited paragraph when a change makes a line too long. Don't split inline code or a link's text across lines. Code blocks and tables are exempt, and so is a line that only overflows because a single link or URL can't be split.

Don't use em dashes in prose, comments or documentation; use a hyphen surrounded by spaces, or restructure the sentence.

Review the final diff for accidental file changes, ordering regressions, missing includes, compiler assumptions, and tests that do not actually exercise the changed behavior. Do not commit changes unless the user explicitly requests a commit, or has authorized commits for the work at hand (for example, local commits on a working branch). Never push.
