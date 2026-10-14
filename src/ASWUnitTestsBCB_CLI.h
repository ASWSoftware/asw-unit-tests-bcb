/* **************************************************************************
ASWUnitTestsBCB_CLI.h
Author: Anthony S. West - ASW Software

The console test program's command-line options and exit codes.

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

#ifndef ASWUnitTestsBCB_CLIH
#define ASWUnitTestsBCB_CLIH
//---------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Console.h"
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_JUnitReport.h"
#include "ASWUnitTestsBCB_TestBase.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

// The console test program's exit codes, the same as asw-unit-tests'.
int const ExitCode_Success = 0;                   // Every test that ran passed or was skipped
int const ExitCode_TestsFailed = 1;               // A test failed, or the run was stopped early
int const ExitCode_UnhandledException = 2;        // A std::exception or RTL exception ended the run
int const ExitCode_UnhandledExceptionUnknown = 3; // An exception of any other type ended the run
int const ExitCode_InvalidArguments = 4;          // Invalid command line arguments
int const ExitCode_TestTimedOut = 5;              // A test ran too long, and the run was aborted
int const ExitCode_TestCrashed = 6;               // A test crashed, and the run was aborted


/////////////////////////////////////////////////////////////////////////////
// TCLIOptions
//
// Holds every option --help documents, parsed by
// TCLIParser::ParseArguments() and used by main.cpp.
/////////////////////////////////////////////////////////////////////////////
struct TCLIOptions
{
    bool ListOnly;                            // --list
    bool HasFilter;                           // --filter was given
    bool FilterIgnoreCase;                    // --filter-ignore-case
    std::string FilterPattern;                // --filter
    bool Shuffle;                             // --shuffle, or implied by --shuffle-seed
    TOptional<unsigned int> ShuffleSeed;      // --shuffle-seed
    bool HasPartitionIndex;                   // --partition-index was given
    bool HasPartitionCount;                   // --partition-count was given
    unsigned int PartitionIndex;              // --partition-index, from 1
    unsigned int PartitionCount;              // --partition-count
    TOptional<unsigned int> TestTimeoutSeconds; // --test-timeout-seconds
    bool CatchCrashes;                        // --catch-crashes
    TColorMode::Type ColorMode;               // --color, --no-color; TColorMode::Auto by default
    TOptional<TConsoleColor::Type> ColorPass; // --color-pass
    TOptional<TConsoleColor::Type> ColorFail; // --color-fail
    TOptional<TConsoleColor::Type> ColorSkip; // --color-skip
    std::string JUnitReportPath;              // --report-junit; empty for no report
    std::string ProjectName;                  // --project-name; "ASWUnitTestsBCB" by default
    bool PauseOnExit;                         // --pause

    TCLIOptions();
};


/////////////////////////////////////////////////////////////////////////////
// TCLIParser
//
// Turns the program's command line into a TCLIOptions.
/////////////////////////////////////////////////////////////////////////////
class TCLIParser
{
public:
    // Returns the filter that 'options'' --filter (with --filter-ignore-case) and --partition-index and
    // --partition-count select together, for 'tester''s Run() and ListTests(), and its description for their log in
    // 'filterDescription' (e.g. "\"*Parse*\" (case-insensitive), partition 2 of 4"). Without either, returns nullptr and
    // leaves 'filterDescription' empty. The caller owns the filter. A partition holds every test whose position in
    // tester.GetAllTestFullNames() (from 0), modulo the count, is its index - 1, so every test is in exactly one
    // partition, whichever group it's in, and shuffling doesn't move a test to another one. Call after
    // tester.Initialize().
    static ITestFilter* BuildTestFilter(TTestHandler& tester, TCLIOptions const& options,
        std::string& filterDescription);

    // Parses 'args' (args[0] is the program's name, as in argv) into 'options'. An option that takes a value accepts
    // it as the next argument ("--project-name X") or after an equals sign ("--project-name=X"), and a later option
    // overrides an earlier one. Returns true if the program should go on to run the tests. Returns false, with
    // 'exitCode' set, if it should end now: ExitCode_Success after writing --help's or --version's text to 'out', or
    // ExitCode_InvalidArguments after writing an error message there.
    static bool ParseArguments(std::vector<std::string> const& args, TCLIOptions& options, int& exitCode,
        std::ostream& out);

    // Parses "auto", "always" or "never", as --color takes them; returns no value for anything else.
    static TOptional<TColorMode::Type> ParseColorMode(std::string const& text);

    // Returns 'text' as a non-negative integer if all of it is one, in decimal digits only (no sign or spaces), that
    // fits in an unsigned int; otherwise no value.
    static TOptional<unsigned int> ParseUnsignedInt(std::string const& text);

    // Splits 'commandLine', as GetCommandLineW() returns it, into arguments by the rules the C runtime uses for argv,
    // and returns them in UTF-8. Unlike argv, which holds each argument in the ANSI code page, this keeps every
    // character, e.g. of a non-ASCII project name.
    static std::vector<std::string> SplitCommandLine(wchar_t const* commandLine);
};

//---------------------------------------------------------------------------

// The exit code for a completed TTestHandler::Run(): ExitCode_TestTimedOut or ExitCode_TestCrashed if the run was
// aborted for that reason, otherwise ExitCode_TestsFailed if any test failed or the run was stopped early (see
// ITestRunObserver::StopRequested()), otherwise ExitCode_Success.
int ExitCodeForResults(TTestResults const& results);

// Converts a run's records into the test cases a --report-junit report is written from.
std::vector<TJUnitTestCase> ToJUnitTestCases(std::vector<TTestCaseRecord> const& records);

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_CLIH
