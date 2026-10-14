/* **************************************************************************
Test_ASWUnitTestsBCB_CLI.cpp
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_CLI.h.

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
#include "Test_ASWUnitTestsBCB_CLI.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_CLI.h"
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TParseResult
//
// What TCLIParser::ParseArguments() did with a command line.
/////////////////////////////////////////////////////////////////////////////
struct TParseResult
{
    bool Proceed;   // ParseArguments()' result
    int ExitCode;   // -1 unless ParseArguments() set it
    std::string Output;
    ASWUnitTestsBCB::TCLIOptions Options;

    TParseResult()
        : Proceed(false),
          ExitCode(-1)
    {
    }
};

/////////////////////////////////////////////////////////////////////////////
// TSilentObserver
//
// Keeps an inner handler's output off the console.
/////////////////////////////////////////////////////////////////////////////
class TSilentObserver : public ASWUnitTestsBCB::ITestRunObserver
{
public:
    virtual void OnLog(std::string const& /*text*/)
    {
    }

    virtual void OnTestFinished(ASWUnitTestsBCB::TTestCaseRecord const& /*record*/)
    {
    }

    virtual void OnTestStarted(std::string const& /*groupName*/, std::string const& /*testName*/)
    {
    }

    virtual bool StopRequested()
    {
        return false;
    }
};

std::vector<std::string> Args(char const* arg1 = nullptr, char const* arg2 = nullptr, char const* arg3 = nullptr,
    char const* arg4 = nullptr);
std::string Matching(ASWUnitTestsBCB::ITestFilter const& filter, std::vector<std::string> const& names);
TParseResult Parse(std::vector<std::string> const& args);

//---------------------------------------------------------------------------
// Returns a command line's arguments: the program's name, then each argument given, up to the first nullptr.
std::vector<std::string> Args(char const* arg1, char const* arg2, char const* arg3, char const* arg4)
{
    char const* const given[] = { arg1, arg2, arg3, arg4 };
    std::vector<std::string> args(1, "ASWUnitTestsBCB.exe");

    for (std::size_t i = 0; (i < (sizeof(given) / sizeof(given[0]))) && (given[i] != nullptr); ++i)
        args.push_back(given[i]);

    return args;
}
//---------------------------------------------------------------------------
// Returns the names 'filter' matches, as "Group.Test;Group.Test;".
std::string Matching(ASWUnitTestsBCB::ITestFilter const& filter, std::vector<std::string> const& names)
{
    std::string matching;

    for (std::size_t i = 0; i < names.size(); ++i)
    {
        if (filter.Matches(names[i]))
            matching += names[i] + ";";
    }

    return matching;
}
//---------------------------------------------------------------------------
TParseResult Parse(std::vector<std::string> const& args)
{
    TParseResult result;
    std::ostringstream out;

    result.Proceed = ASWUnitTestsBCB::TCLIParser::ParseArguments(args, result.Options, result.ExitCode, out);
    result.Output = out.str();
    return result;
}
//---------------------------------------------------------------------------

} // namespace


namespace ASWUnitTestsBCB
{

// The inner groups the tests below use. Neither is registered, so the program's own run never includes them.

/////////////////////////////////////////////////////////////////////////////
// TInnerCLI_A, TInnerCLI_B
/////////////////////////////////////////////////////////////////////////////
class TInnerCLI_A : public TTestGroupBase
{
public:
    __fastcall TInnerCLI_A()
        : TTestGroupBase("A_Group")
    {
    }

__published:
    void __fastcall Test_One();
    void __fastcall Test_Two();
    void __fastcall Test_Three();
};

void __fastcall TInnerCLI_A::Test_One()
{
}

void __fastcall TInnerCLI_A::Test_Two()
{
}

void __fastcall TInnerCLI_A::Test_Three()
{
}

class TInnerCLI_B : public TTestGroupBase
{
public:
    __fastcall TInnerCLI_B()
        : TTestGroupBase("B_Group")
    {
    }

__published:
    void __fastcall Test_Four();
    void __fastcall Test_Five();
};

void __fastcall TInnerCLI_B::Test_Four()
{
}

void __fastcall TInnerCLI_B::Test_Five()
{
}


/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_CLI
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_CLI::TTest_ASWUnitTestsBCB_CLI()
    : inherited("ASWUnitTestsBCB_CLI_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_BuildTestFilter_Partition()
{
    // Arrange: five tests, in this order: A_Group.One, A_Group.Two, A_Group.Three, B_Group.Four, B_Group.Five
    TTestGroupRegistrar const b(CreateTestGroup<TInnerCLI_B>, __classid(TInnerCLI_B), "TInnerCLI_B", "", 0, nullptr);
    TTestGroupRegistrar const a(CreateTestGroup<TInnerCLI_A>, __classid(TInnerCLI_A), "TInnerCLI_A", "", 0, &b);
    TTestHandler handler;
    TSilentObserver observer;
    handler.SetRunObserver(&observer);
    handler.Initialize("Inner", &a);
    std::vector<std::string> const allNames = handler.GetAllTestFullNames();
    std::string description;

    // Act and assert: each test in exactly one partition, by its position
    TCLIOptions options = Parse(Args("--partition-index", "1", "--partition-count", "2")).Options;
    std::unique_ptr<ITestFilter> filter(TCLIParser::BuildTestFilter(handler, options, description));
    AssertNotNull(filter.get(), ASWBCB_HERE);
    CheckEquals("A_Group.One;A_Group.Three;B_Group.Five;", Matching(*filter, allNames), ASWBCB_HERE, "1 of 2");
    CheckEquals("partition 1 of 2", description, ASWBCB_HERE);

    options = Parse(Args("--partition-index=2", "--partition-count=2")).Options;
    filter.reset(TCLIParser::BuildTestFilter(handler, options, description));
    AssertNotNull(filter.get(), ASWBCB_HERE);
    CheckEquals("A_Group.Two;B_Group.Four;", Matching(*filter, allNames), ASWBCB_HERE, "2 of 2");
    CheckEquals("partition 2 of 2", description, ASWBCB_HERE);

    options = Parse(Args("--partition-index", "3", "--partition-count", "7")).Options;
    filter.reset(TCLIParser::BuildTestFilter(handler, options, description));
    AssertNotNull(filter.get(), ASWBCB_HERE);
    CheckEquals("A_Group.Three;", Matching(*filter, allNames), ASWBCB_HERE, "3 of 7");

    options = Parse(Args("--partition-index", "7", "--partition-count", "7")).Options;
    filter.reset(TCLIParser::BuildTestFilter(handler, options, description));
    AssertNotNull(filter.get(), ASWBCB_HERE);
    CheckEquals("", Matching(*filter, allNames), ASWBCB_HERE, "more partitions than tests: some are empty");

    options = Parse(Args("--partition-index", "1", "--partition-count", "1")).Options;
    filter.reset(TCLIParser::BuildTestFilter(handler, options, description));
    AssertNotNull(filter.get(), ASWBCB_HERE);
    CheckEquals(5u, static_cast<unsigned int>(allNames.size()), ASWBCB_HERE);
    CheckEquals("A_Group.One;A_Group.Two;A_Group.Three;B_Group.Four;B_Group.Five;", Matching(*filter, allNames),
        ASWBCB_HERE, "1 of 1: everything");

    // With --filter too: the partition's tests that the pattern matches, the partition still by position in the
    // whole suite
    options = Parse(Args("--filter=B_*", "--partition-index=1", "--partition-count=2")).Options;
    filter.reset(TCLIParser::BuildTestFilter(handler, options, description));
    AssertNotNull(filter.get(), ASWBCB_HERE);
    CheckEquals("B_Group.Five;", Matching(*filter, allNames), ASWBCB_HERE, "--filter and a partition");
    CheckEquals("\"B_*\", partition 1 of 2", description, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_BuildTestFilter_Pattern()
{
    TTestHandler handler; // Only a partition uses it
    std::string description = "not cleared";
    TCLIOptions options;
    std::unique_ptr<ITestFilter> none(TCLIParser::BuildTestFilter(handler, options, description));
    CheckNull(none.get(), ASWBCB_HERE, "no --filter, no filter");
    CheckEquals("", description, ASWBCB_HERE, "no --filter, no description");

    options.FilterIgnoreCase = true;
    none.reset(TCLIParser::BuildTestFilter(handler, options, description));
    CheckNull(none.get(), ASWBCB_HERE, "--filter-ignore-case alone does nothing");

    options = Parse(Args("--filter", "*Parse?")).Options;
    std::unique_ptr<ITestFilter> const pattern(TCLIParser::BuildTestFilter(handler, options, description));
    AssertNotNull(pattern.get(), ASWBCB_HERE);
    CheckEquals("\"*Parse?\"", description, ASWBCB_HERE);
    CheckTrue(pattern->Matches("Group.Parse1"), ASWBCB_HERE);
    CheckFalse(pattern->Matches("Group.parse1"), ASWBCB_HERE, "case-sensitive");
    CheckFalse(pattern->Matches("Group.Parse12"), ASWBCB_HERE, "the whole name");

    options = Parse(Args("--filter=*Parse?", "--filter-ignore-case")).Options;
    std::unique_ptr<ITestFilter> const ignoreCase(TCLIParser::BuildTestFilter(handler, options, description));
    AssertNotNull(ignoreCase.get(), ASWBCB_HERE);
    CheckEquals("\"*Parse?\" (case-insensitive)", description, ASWBCB_HERE);
    CheckTrue(ignoreCase->Matches("Group.parse1"), ASWBCB_HERE, "--filter-ignore-case");

    options = Parse(Args("--filter=")).Options;
    std::unique_ptr<ITestFilter> const empty(TCLIParser::BuildTestFilter(handler, options, description));
    AssertNotNull(empty.get(), ASWBCB_HERE, "an empty pattern is still a filter");
    CheckEquals("\"\"", description, ASWBCB_HERE);
    CheckFalse(empty->Matches("Group.Test"), ASWBCB_HERE, "that matches nothing");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ExitCodeForResults_Priority()
{
    TTestResults results;
    CheckEquals(ExitCode_Success, ExitCodeForResults(results), ASWBCB_HERE, "nothing ran");

    results.SuccessCount = 3;
    results.SkippedCount = 2;
    CheckEquals(ExitCode_Success, ExitCodeForResults(results), ASWBCB_HERE, "skips don't count");

    results.Stopped = true;
    CheckEquals(ExitCode_TestsFailed, ExitCodeForResults(results), ASWBCB_HERE, "stopped early");

    results.Stopped = false;
    results.FailedCount = 1;
    CheckEquals(ExitCode_TestsFailed, ExitCodeForResults(results), ASWBCB_HERE, "a failure");

    results.Crashed = true;
    CheckEquals(ExitCode_TestCrashed, ExitCodeForResults(results), ASWBCB_HERE, "a crash outranks failures");

    results.TimedOut = true;
    CheckEquals(ExitCode_TestTimedOut, ExitCodeForResults(results), ASWBCB_HERE, "a timeout outranks a crash");

    CheckEquals(0, ExitCode_Success, ASWBCB_HERE, "the codes are asw-unit-tests'");
    CheckEquals(1, ExitCode_TestsFailed, ASWBCB_HERE);
    CheckEquals(2, ExitCode_UnhandledException, ASWBCB_HERE);
    CheckEquals(3, ExitCode_UnhandledExceptionUnknown, ASWBCB_HERE);
    CheckEquals(4, ExitCode_InvalidArguments, ASWBCB_HERE);
    CheckEquals(5, ExitCode_TestTimedOut, ASWBCB_HERE);
    CheckEquals(6, ExitCode_TestCrashed, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_CatchCrashes()
{
    TParseResult const catchCrashes = Parse(Args("--catch-crashes", "--pause"));
    CheckTrue(catchCrashes.Proceed, ASWBCB_HERE);
    CheckTrue(catchCrashes.Options.CatchCrashes, ASWBCB_HERE);
    CheckTrue(catchCrashes.Options.PauseOnExit, ASWBCB_HERE, "the option after it");
    CheckEquals("", catchCrashes.Output, ASWBCB_HERE);

    TParseResult const withValue = Parse(Args("--catch-crashes=yes"));
    CheckFalse(withValue.Proceed, ASWBCB_HERE, "takes no value");
    CheckEquals(ExitCode_InvalidArguments, withValue.ExitCode, ASWBCB_HERE, "takes no value");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_Color()
{
    CheckEquals(TColorMode::Always, Parse(Args("--color", "always")).Options.ColorMode, ASWBCB_HERE);
    CheckEquals(TColorMode::Never, Parse(Args("--color=never")).Options.ColorMode, ASWBCB_HERE);
    CheckEquals(TColorMode::Auto, Parse(Args("--no-color", "--color=auto")).Options.ColorMode, ASWBCB_HERE,
        "the last one wins");
    CheckEquals(TColorMode::Never, Parse(Args("--no-color")).Options.ColorMode, ASWBCB_HERE);

    TParseResult const badMode = Parse(Args("--color", "bogus"));
    CheckFalse(badMode.Proceed, ASWBCB_HERE, "an unknown mode");
    CheckEquals(ExitCode_InvalidArguments, badMode.ExitCode, ASWBCB_HERE, "an unknown mode");
    CheckEquals("Error: --color requires \"auto\", \"always\", or \"never\".\n", badMode.Output, ASWBCB_HERE);

    TParseResult const missingMode = Parse(Args("--color"));
    CheckEquals(ExitCode_InvalidArguments, missingMode.ExitCode, ASWBCB_HERE, "a missing mode");
    CheckEquals("Error: --color requires \"auto\", \"always\", or \"never\".\n", missingMode.Output, ASWBCB_HERE);

    TParseResult const colors = Parse(Args("--color-pass", "bright-green", "--color-fail=default", "--color-skip=blue"));
    CheckTrue(colors.Proceed, ASWBCB_HERE);
    CheckTrue(colors.Options.ColorPass.HasValue(), ASWBCB_HERE);
    CheckEquals(TConsoleColor::BrightGreen, colors.Options.ColorPass.Value(), ASWBCB_HERE, "--color-pass <color>");
    CheckTrue(colors.Options.ColorFail.HasValue(), ASWBCB_HERE);
    CheckEquals(TConsoleColor::Default, colors.Options.ColorFail.Value(), ASWBCB_HERE, "--color-fail=<color>");
    CheckTrue(colors.Options.ColorSkip.HasValue(), ASWBCB_HERE);
    CheckEquals(TConsoleColor::Blue, colors.Options.ColorSkip.Value(), ASWBCB_HERE, "--color-skip=<color>");

    TParseResult const badColor = Parse(Args("--color-fail=pink"));
    CheckEquals(ExitCode_InvalidArguments, badColor.ExitCode, ASWBCB_HERE, "an unknown color");
    CheckEquals("Error: --color-fail was given an unrecognized color name.\n", badColor.Output, ASWBCB_HERE);

    TParseResult const missingColor = Parse(Args("--color-skip"));
    CheckEquals(ExitCode_InvalidArguments, missingColor.ExitCode, ASWBCB_HERE, "a missing color");
    CheckEquals("Error: --color-skip requires a color name argument.\n", missingColor.Output, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_Defaults()
{
    TParseResult const none = Parse(Args());

    CheckTrue(none.Proceed, ASWBCB_HERE, "no arguments: run the tests");
    CheckEquals(-1, none.ExitCode, ASWBCB_HERE, "no exit code");
    CheckEquals("", none.Output, ASWBCB_HERE, "nothing written");
    CheckEquals("ASWUnitTestsBCB", none.Options.ProjectName, ASWBCB_HERE);
    CheckFalse(none.Options.PauseOnExit, ASWBCB_HERE);
    CheckFalse(none.Options.ListOnly, ASWBCB_HERE);
    CheckFalse(none.Options.HasFilter, ASWBCB_HERE);
    CheckFalse(none.Options.FilterIgnoreCase, ASWBCB_HERE);
    CheckEquals("", none.Options.FilterPattern, ASWBCB_HERE);
    CheckFalse(none.Options.Shuffle, ASWBCB_HERE);
    CheckFalse(none.Options.ShuffleSeed.HasValue(), ASWBCB_HERE);
    CheckFalse(none.Options.HasPartitionIndex, ASWBCB_HERE);
    CheckFalse(none.Options.HasPartitionCount, ASWBCB_HERE);
    CheckEquals(TColorMode::Auto, none.Options.ColorMode, ASWBCB_HERE);
    CheckFalse(none.Options.ColorPass.HasValue(), ASWBCB_HERE);
    CheckFalse(none.Options.ColorFail.HasValue(), ASWBCB_HERE);
    CheckFalse(none.Options.ColorSkip.HasValue(), ASWBCB_HERE);
    CheckEquals("", none.Options.JUnitReportPath, ASWBCB_HERE, "no report");
    CheckFalse(none.Options.TestTimeoutSeconds.HasValue(), ASWBCB_HERE, "no timeout");
    CheckFalse(none.Options.CatchCrashes, ASWBCB_HERE);

    TParseResult const programOnly = Parse(std::vector<std::string>());
    CheckTrue(programOnly.Proceed, ASWBCB_HERE, "not even the program's name");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_Filter()
{
    TParseResult const separate = Parse(Args("--filter", "*.Parse*"));
    CheckTrue(separate.Proceed, ASWBCB_HERE, "--filter <pattern>");
    CheckEquals(-1, separate.ExitCode, ASWBCB_HERE, "--filter <pattern>");
    CheckTrue(separate.Options.HasFilter, ASWBCB_HERE, "--filter <pattern>");
    CheckEquals("*.Parse*", separate.Options.FilterPattern, ASWBCB_HERE, "--filter <pattern>");
    CheckFalse(separate.Options.FilterIgnoreCase, ASWBCB_HERE, "--filter <pattern>");

    TParseResult const equals = Parse(Args("--filter=*.Parse*", "--filter-ignore-case"));
    CheckTrue(equals.Proceed, ASWBCB_HERE, "--filter=<pattern>");
    CheckTrue(equals.Options.HasFilter, ASWBCB_HERE, "--filter=<pattern>");
    CheckEquals("*.Parse*", equals.Options.FilterPattern, ASWBCB_HERE, "--filter=<pattern>");
    CheckTrue(equals.Options.FilterIgnoreCase, ASWBCB_HERE, "--filter-ignore-case");

    TParseResult const empty = Parse(Args("--filter="));
    CheckTrue(empty.Options.HasFilter, ASWBCB_HERE, "an empty pattern");
    CheckEquals("", empty.Options.FilterPattern, ASWBCB_HERE, "an empty pattern");

    TParseResult const ignoreCaseOnly = Parse(Args("--filter-ignore-case"));
    CheckTrue(ignoreCaseOnly.Proceed, ASWBCB_HERE, "--filter-ignore-case alone");
    CheckFalse(ignoreCaseOnly.Options.HasFilter, ASWBCB_HERE, "--filter-ignore-case alone");
    CheckTrue(ignoreCaseOnly.Options.FilterIgnoreCase, ASWBCB_HERE, "--filter-ignore-case alone");

    TParseResult const missingValue = Parse(Args("--filter"));
    CheckFalse(missingValue.Proceed, ASWBCB_HERE, "a missing pattern");
    CheckEquals(ExitCode_InvalidArguments, missingValue.ExitCode, ASWBCB_HERE, "a missing pattern");
    CheckEquals("Error: --filter requires a pattern argument.\n", missingValue.Output, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_HelpAndVersion()
{
    TParseResult const help = Parse(Args("--help"));
    CheckFalse(help.Proceed, ASWBCB_HERE, "--help");
    CheckEquals(ExitCode_Success, help.ExitCode, ASWBCB_HERE, "--help");
    CheckStartsWith(help.Output, "Usage: ASWUnitTestsBCB [options]\n\nOptions:\n", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --filter <pattern> ", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --filter-ignore-case\n", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --shuffle ", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --shuffle-seed <N> ", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --partition-index <N>\n", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --partition-count <N>\n", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --test-timeout-seconds <N>\n", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --catch-crashes ", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --color <mode> ", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --no-color ", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --color-pass/--color-fail/--color-skip <color>\n", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --report-junit <path>\n", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --project-name <name>\n", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --list ", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --pause ", ASWBCB_HERE);
    CheckContains(help.Output, "\n  --version ", ASWBCB_HERE);
    CheckEquals("  --help              Show this message and exit.\n",
        help.Output.substr(help.Output.rfind("\n  --help") + 1), ASWBCB_HERE, "the last line");

    TParseResult const shortHelp = Parse(Args("-h"));
    CheckFalse(shortHelp.Proceed, ASWBCB_HERE, "-h");
    CheckEquals(ExitCode_Success, shortHelp.ExitCode, ASWBCB_HERE, "-h");
    CheckEquals(help.Output, shortHelp.Output, ASWBCB_HERE, "-h is --help");

    TParseResult const version = Parse(Args("--version"));
    CheckFalse(version.Proceed, ASWBCB_HERE, "--version");
    CheckEquals(ExitCode_Success, version.ExitCode, ASWBCB_HERE, "--version");
    CheckEquals(TTestHandler::GetVersionFullStr() + "\n", version.Output, ASWBCB_HERE, "--version");

    TParseResult const versionFirst = Parse(Args("--pause", "--version", "--bogus"));
    CheckEquals(ExitCode_Success, versionFirst.ExitCode, ASWBCB_HERE, "the arguments after --version are ignored");
    CheckEquals(version.Output, versionFirst.Output, ASWBCB_HERE, "the arguments after --version are ignored");

    TParseResult const errorFirst = Parse(Args("--bogus", "--help"));
    CheckEquals(ExitCode_InvalidArguments, errorFirst.ExitCode, ASWBCB_HERE, "an error before --help");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_InvalidOption()
{
    std::string const usage = Parse(Args("--help")).Output;

    TParseResult const unknown = Parse(Args("--pause", "--bogus"));
    CheckFalse(unknown.Proceed, ASWBCB_HERE, "an unrecognized option");
    CheckEquals(ExitCode_InvalidArguments, unknown.ExitCode, ASWBCB_HERE, "an unrecognized option");
    CheckEquals("Error: unrecognized option \"--bogus\".\n\n" + usage, unknown.Output, ASWBCB_HERE,
        "an unrecognized option, then the usage");

    TParseResult const positional = Parse(Args("Group.Test"));
    CheckEquals(ExitCode_InvalidArguments, positional.ExitCode, ASWBCB_HERE, "an argument that isn't an option");
    CheckStartsWith(positional.Output, "Error: unrecognized option \"Group.Test\".\n", ASWBCB_HERE);

    TParseResult const flagWithValue = Parse(Args("--pause=1"));
    CheckEquals(ExitCode_InvalidArguments, flagWithValue.ExitCode, ASWBCB_HERE, "a value for an option without one");

    TParseResult const longerName = Parse(Args("--project-names=X"));
    CheckEquals(ExitCode_InvalidArguments, longerName.ExitCode, ASWBCB_HERE, "an option's name, extended");

    TParseResult const missingValue = Parse(Args("--project-name"));
    CheckFalse(missingValue.Proceed, ASWBCB_HERE, "a missing value");
    CheckEquals(ExitCode_InvalidArguments, missingValue.ExitCode, ASWBCB_HERE, "a missing value");
    CheckEquals("Error: --project-name requires a name argument.\n", missingValue.Output, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_List()
{
    TParseResult const list = Parse(Args("--list", "--filter", "*X*"));

    CheckTrue(list.Proceed, ASWBCB_HERE, "--list goes on, to list the tests");
    CheckEquals(-1, list.ExitCode, ASWBCB_HERE);
    CheckTrue(list.Options.ListOnly, ASWBCB_HERE);
    CheckTrue(list.Options.HasFilter, ASWBCB_HERE, "with a filter");
    CheckEquals("", list.Output, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_Partition()
{
    TParseResult const separate = Parse(Args("--partition-index", "2", "--partition-count", "3"));
    CheckTrue(separate.Proceed, ASWBCB_HERE, "a valid pair");
    CheckEquals("", separate.Output, ASWBCB_HERE, "a valid pair");
    CheckTrue(separate.Options.HasPartitionIndex, ASWBCB_HERE);
    CheckTrue(separate.Options.HasPartitionCount, ASWBCB_HERE);
    CheckEquals(2u, separate.Options.PartitionIndex, ASWBCB_HERE);
    CheckEquals(3u, separate.Options.PartitionCount, ASWBCB_HERE);

    TParseResult const equals = Parse(Args("--partition-count=3", "--partition-index=3"));
    CheckTrue(equals.Proceed, ASWBCB_HERE, "the = form, in either order, up to the count");
    CheckEquals(3u, equals.Options.PartitionIndex, ASWBCB_HERE);
    CheckEquals(3u, equals.Options.PartitionCount, ASWBCB_HERE);

    TParseResult const indexOnly = Parse(Args("--partition-index", "1"));
    CheckFalse(indexOnly.Proceed, ASWBCB_HERE, "--partition-index alone");
    CheckEquals(ExitCode_InvalidArguments, indexOnly.ExitCode, ASWBCB_HERE, "--partition-index alone");
    CheckEquals("Error: --partition-index and --partition-count must be given together.\n", indexOnly.Output,
        ASWBCB_HERE);

    TParseResult const countOnly = Parse(Args("--partition-count", "2"));
    CheckEquals(ExitCode_InvalidArguments, countOnly.ExitCode, ASWBCB_HERE, "--partition-count alone");

    TParseResult const zeroCount = Parse(Args("--partition-index=1", "--partition-count=0"));
    CheckEquals(ExitCode_InvalidArguments, zeroCount.ExitCode, ASWBCB_HERE, "a count of 0");
    CheckEquals("Error: --partition-count must be at least 1.\n", zeroCount.Output, ASWBCB_HERE);

    TParseResult const zeroIndex = Parse(Args("--partition-index=0", "--partition-count=3"));
    CheckEquals(ExitCode_InvalidArguments, zeroIndex.ExitCode, ASWBCB_HERE, "an index of 0");
    CheckEquals("Error: --partition-index must be between 1 and --partition-count (3).\n", zeroIndex.Output,
        ASWBCB_HERE);

    TParseResult const indexTooHigh = Parse(Args("--partition-index=4", "--partition-count=3"));
    CheckEquals(ExitCode_InvalidArguments, indexTooHigh.ExitCode, ASWBCB_HERE, "an index above the count");
    CheckEquals("Error: --partition-index must be between 1 and --partition-count (3).\n", indexTooHigh.Output,
        ASWBCB_HERE);

    TParseResult const notNumeric = Parse(Args("--partition-index", "x", "--partition-count", "3"));
    CheckEquals(ExitCode_InvalidArguments, notNumeric.ExitCode, ASWBCB_HERE, "an index that isn't a number");
    CheckEquals("Error: --partition-index requires a positive integer argument.\n", notNumeric.Output, ASWBCB_HERE);

    TParseResult const missing = Parse(Args("--partition-index", "1", "--partition-count"));
    CheckEquals(ExitCode_InvalidArguments, missing.ExitCode, ASWBCB_HERE, "a missing count");
    CheckEquals("Error: --partition-count requires a numeric argument.\n", missing.Output, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_Pause()
{
    TParseResult const pause = Parse(Args("--pause"));

    CheckTrue(pause.Proceed, ASWBCB_HERE);
    CheckEquals(-1, pause.ExitCode, ASWBCB_HERE);
    CheckTrue(pause.Options.PauseOnExit, ASWBCB_HERE);
    CheckEquals("", pause.Output, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_ProjectName()
{
    TParseResult const separate = Parse(Args("--project-name", "My Project", "--pause"));
    CheckTrue(separate.Proceed, ASWBCB_HERE, "--project-name <name>");
    CheckEquals(-1, separate.ExitCode, ASWBCB_HERE, "--project-name <name>");
    CheckEquals("My Project", separate.Options.ProjectName, ASWBCB_HERE, "--project-name <name>");
    CheckTrue(separate.Options.PauseOnExit, ASWBCB_HERE, "the option after it");
    CheckEquals("", separate.Output, ASWBCB_HERE);

    TParseResult const equals = Parse(Args("--project-name=My Project"));
    CheckTrue(equals.Proceed, ASWBCB_HERE, "--project-name=<name>");
    CheckEquals("My Project", equals.Options.ProjectName, ASWBCB_HERE, "--project-name=<name>");

    CheckEquals("", Parse(Args("--project-name=")).Options.ProjectName, ASWBCB_HERE, "an empty name");
    CheckEquals("a=b", Parse(Args("--project-name=a=b")).Options.ProjectName, ASWBCB_HERE, "after the first '='");
    CheckEquals("caf\xC3\xA9", Parse(Args("--project-name", "caf\xC3\xA9")).Options.ProjectName, ASWBCB_HERE,
        "UTF-8, as it is");
    CheckEquals("B", Parse(Args("--project-name", "A", "--project-name=B")).Options.ProjectName, ASWBCB_HERE,
        "the last one wins");

    TParseResult const optionAsName = Parse(Args("--project-name", "--pause"));
    CheckEquals("--pause", optionAsName.Options.ProjectName, ASWBCB_HERE, "the next argument, whatever it is");
    CheckFalse(optionAsName.Options.PauseOnExit, ASWBCB_HERE, "the next argument, whatever it is");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_ReportJUnit()
{
    TParseResult const separate = Parse(Args("--report-junit", "results.xml", "--project-name", "MyProject"));
    CheckTrue(separate.Proceed, ASWBCB_HERE, "--report-junit <path>");
    CheckEquals("results.xml", separate.Options.JUnitReportPath, ASWBCB_HERE, "--report-junit <path>");
    CheckEquals("MyProject", separate.Options.ProjectName, ASWBCB_HERE, "the option after it");
    CheckEquals("", separate.Output, ASWBCB_HERE);

    TParseResult const equals = Parse(Args("--report-junit=C:\\CI\\r\xC3\xA9sults.xml"));
    CheckEquals("C:\\CI\\r\xC3\xA9sults.xml", equals.Options.JUnitReportPath, ASWBCB_HERE,
        "--report-junit=<path>, UTF-8 as it is");
    CheckEquals("", Parse(Args("--report-junit=")).Options.JUnitReportPath, ASWBCB_HERE, "an empty path: no report");

    TParseResult const missingValue = Parse(Args("--report-junit"));
    CheckFalse(missingValue.Proceed, ASWBCB_HERE, "a missing path");
    CheckEquals(ExitCode_InvalidArguments, missingValue.ExitCode, ASWBCB_HERE, "a missing path");
    CheckEquals("Error: --report-junit requires a file path argument.\n", missingValue.Output, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_Shuffle()
{
    TParseResult const bare = Parse(Args("--shuffle"));
    CheckTrue(bare.Proceed, ASWBCB_HERE, "--shuffle");
    CheckTrue(bare.Options.Shuffle, ASWBCB_HERE, "--shuffle");
    CheckFalse(bare.Options.ShuffleSeed.HasValue(), ASWBCB_HERE, "--shuffle alone leaves no seed");

    TParseResult const separate = Parse(Args("--shuffle-seed", "42"));
    CheckTrue(separate.Proceed, ASWBCB_HERE, "--shuffle-seed <N>");
    CheckTrue(separate.Options.Shuffle, ASWBCB_HERE, "--shuffle-seed implies --shuffle");
    CheckTrue(separate.Options.ShuffleSeed.HasValue(), ASWBCB_HERE, "--shuffle-seed <N>");
    CheckEquals(42u, separate.Options.ShuffleSeed.Value(), ASWBCB_HERE, "--shuffle-seed <N>");

    TParseResult const equals = Parse(Args("--shuffle-seed=0"));
    CheckTrue(equals.Options.ShuffleSeed.HasValue(), ASWBCB_HERE, "--shuffle-seed=<N>, zero included");
    CheckEquals(0u, equals.Options.ShuffleSeed.Value(), ASWBCB_HERE, "--shuffle-seed=<N>");

    TParseResult const notNumeric = Parse(Args("--shuffle-seed", "abc"));
    CheckFalse(notNumeric.Proceed, ASWBCB_HERE, "a seed that isn't a number");
    CheckEquals(ExitCode_InvalidArguments, notNumeric.ExitCode, ASWBCB_HERE, "a seed that isn't a number");
    CheckEquals("Error: --shuffle-seed requires a non-negative integer argument.\n", notNumeric.Output, ASWBCB_HERE);

    TParseResult const negative = Parse(Args("--shuffle-seed=-1"));
    CheckEquals(ExitCode_InvalidArguments, negative.ExitCode, ASWBCB_HERE, "a negative seed");

    TParseResult const missingValue = Parse(Args("--shuffle-seed"));
    CheckEquals(ExitCode_InvalidArguments, missingValue.ExitCode, ASWBCB_HERE, "a missing seed");
    CheckEquals("Error: --shuffle-seed requires a numeric argument.\n", missingValue.Output, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseArguments_TestTimeout()
{
    TParseResult const separate = Parse(Args("--test-timeout-seconds", "5"));
    CheckTrue(separate.Proceed, ASWBCB_HERE, "--test-timeout-seconds <N>");
    CheckTrue(separate.Options.TestTimeoutSeconds.HasValue(), ASWBCB_HERE, "--test-timeout-seconds <N>");
    CheckEquals(5u, separate.Options.TestTimeoutSeconds.Value(), ASWBCB_HERE, "--test-timeout-seconds <N>");
    CheckEquals("", separate.Output, ASWBCB_HERE);

    TParseResult const equals = Parse(Args("--test-timeout-seconds=4294967295"));
    CheckEquals(4294967295u, equals.Options.TestTimeoutSeconds.Value(), ASWBCB_HERE, "--test-timeout-seconds=<N>");

    TParseResult const zero = Parse(Args("--test-timeout-seconds", "0"));
    CheckFalse(zero.Proceed, ASWBCB_HERE, "zero would never let a test finish");
    CheckEquals(ExitCode_InvalidArguments, zero.ExitCode, ASWBCB_HERE, "zero");
    CheckEquals("Error: --test-timeout-seconds requires a positive integer argument.\n", zero.Output, ASWBCB_HERE);

    TParseResult const notNumeric = Parse(Args("--test-timeout-seconds=abc"));
    CheckEquals(ExitCode_InvalidArguments, notNumeric.ExitCode, ASWBCB_HERE, "not a number");
    CheckEquals("Error: --test-timeout-seconds requires a positive integer argument.\n", notNumeric.Output,
        ASWBCB_HERE);

    TParseResult const missingValue = Parse(Args("--test-timeout-seconds"));
    CheckFalse(missingValue.Proceed, ASWBCB_HERE, "a missing value");
    CheckEquals(ExitCode_InvalidArguments, missingValue.ExitCode, ASWBCB_HERE, "a missing value");
    CheckEquals("Error: --test-timeout-seconds requires a numeric argument.\n", missingValue.Output, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseColorMode()
{
    CheckEquals(TColorMode::Auto, TCLIParser::ParseColorMode("auto").Value(), ASWBCB_HERE);
    CheckEquals(TColorMode::Always, TCLIParser::ParseColorMode("always").Value(), ASWBCB_HERE);
    CheckEquals(TColorMode::Never, TCLIParser::ParseColorMode("never").Value(), ASWBCB_HERE);
    CheckTrue(TCLIParser::ParseColorMode("auto").HasValue(), ASWBCB_HERE);
    CheckFalse(TCLIParser::ParseColorMode("").HasValue(), ASWBCB_HERE, "empty");
    CheckFalse(TCLIParser::ParseColorMode("Always").HasValue(), ASWBCB_HERE, "case-sensitive");
    CheckFalse(TCLIParser::ParseColorMode("bogus").HasValue(), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ParseUnsignedInt()
{
    CheckEquals(0u, TCLIParser::ParseUnsignedInt("0").Value(), ASWBCB_HERE, "zero");
    CheckTrue(TCLIParser::ParseUnsignedInt("0").HasValue(), ASWBCB_HERE, "zero");
    CheckEquals(123u, TCLIParser::ParseUnsignedInt("123").Value(), ASWBCB_HERE);
    CheckEquals(7u, TCLIParser::ParseUnsignedInt("007").Value(), ASWBCB_HERE, "leading zeros");
    CheckEquals(4294967295u, TCLIParser::ParseUnsignedInt("4294967295").Value(), ASWBCB_HERE, "the largest");
    CheckTrue(TCLIParser::ParseUnsignedInt("4294967295").HasValue(), ASWBCB_HERE, "the largest");

    CheckFalse(TCLIParser::ParseUnsignedInt("4294967296").HasValue(), ASWBCB_HERE, "one too large");
    CheckFalse(TCLIParser::ParseUnsignedInt("4294967300").HasValue(), ASWBCB_HERE, "too large in the last digit");
    CheckFalse(TCLIParser::ParseUnsignedInt("99999999999").HasValue(), ASWBCB_HERE, "far too large");
    CheckFalse(TCLIParser::ParseUnsignedInt("").HasValue(), ASWBCB_HERE, "empty");
    CheckFalse(TCLIParser::ParseUnsignedInt("abc").HasValue(), ASWBCB_HERE, "not a number");
    CheckFalse(TCLIParser::ParseUnsignedInt("123abc").HasValue(), ASWBCB_HERE, "trailing text");
    CheckFalse(TCLIParser::ParseUnsignedInt("-5").HasValue(), ASWBCB_HERE, "a sign");
    CheckFalse(TCLIParser::ParseUnsignedInt("+5").HasValue(), ASWBCB_HERE, "a sign");
    CheckFalse(TCLIParser::ParseUnsignedInt(" 5").HasValue(), ASWBCB_HERE, "a space");
    CheckFalse(TCLIParser::ParseUnsignedInt("0x10").HasValue(), ASWBCB_HERE, "hexadecimal");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_SplitCommandLine_CRuntimeRules()
{
    std::vector<std::string> const args = TCLIParser::SplitCommandLine(
        L"\"C:\\My Tests\\Tests.exe\" --project-name \"My Project\" --filter=*Test?* \"\" a\\\"b C:\\dir\\ "
        L"caf\x00E9");

    AssertEquals(8u, args.size(), ASWBCB_HERE);
    CheckEquals("C:\\My Tests\\Tests.exe", args[0], ASWBCB_HERE, "a quoted program path");
    CheckEquals("--project-name", args[1], ASWBCB_HERE);
    CheckEquals("My Project", args[2], ASWBCB_HERE, "quotes group words");
    CheckEquals("--filter=*Test?*", args[3], ASWBCB_HERE, "wildcards aren't expanded");
    CheckEquals("", args[4], ASWBCB_HERE, "an empty argument");
    CheckEquals("a\"b", args[5], ASWBCB_HERE, "an escaped quote");
    CheckEquals("C:\\dir\\", args[6], ASWBCB_HERE, "backslashes before anything but a quote");
    CheckEquals("caf\xC3\xA9", args[7], ASWBCB_HERE, "non-ASCII, in UTF-8");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_CLI::Test_ToJUnitTestCases_CopiesEachRecord()
{
    std::vector<TTestCaseRecord> records(3);
    records[0].GroupName = "Group";
    records[0].TestName = "Passes";
    records[0].DurationSeconds = 0.5;
    records[0].Outcome = TTestOutcome::Pass;
    records[1].GroupName = "Group";
    records[1].TestName = "Fails";
    records[1].DurationSeconds = 0.25;
    records[1].Outcome = TTestOutcome::Fail;
    records[1].Message = "Check failed for: x";
    records[2].GroupName = "Other";
    records[2].TestName = "Skips";
    records[2].Outcome = TTestOutcome::Skip;
    records[2].Message = "not on this platform";

    std::vector<TJUnitTestCase> const testCases = ToJUnitTestCases(records);

    AssertEquals(3u, testCases.size(), ASWBCB_HERE, "one test case per record");
    CheckEquals(TJUnitOutcome::Pass, testCases[0].Outcome, ASWBCB_HERE, "Pass becomes Pass");
    CheckEquals(TJUnitOutcome::Fail, testCases[1].Outcome, ASWBCB_HERE, "Fail becomes Fail");
    CheckEquals(TJUnitOutcome::Skip, testCases[2].Outcome, ASWBCB_HERE, "Skip becomes Skip");
    CheckEquals("Other", testCases[2].GroupName, ASWBCB_HERE, "the group name is kept");
    CheckEquals("Fails", testCases[1].TestName, ASWBCB_HERE, "and the test name");
    CheckEquals("Check failed for: x", testCases[1].Message, ASWBCB_HERE, "and the detail");
    CheckEquals("not on this platform", testCases[2].Message, ASWBCB_HERE, "a skip's too");
    CheckNear(0.5, testCases[0].DurationSeconds, 1e-12, ASWBCB_HERE, "and the duration");
    CheckNear(0.25, testCases[1].DurationSeconds, 1e-12, ASWBCB_HERE, "each one's");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_CLI)
