/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_CommandLine.cpp
Author: Anthony S. West - ASW Software

See header for info.

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
#include "Test_ASWUnitTestsBCB_GUI_CommandLine.h"
//---------------------------------------------------------------------------
#include <algorithm>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_CLI.h"
#include "ASWUnitTestsBCB_GUI_CommandLine.h"
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_StdOutRedirect.h"
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------
using namespace System;
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TArgs
//
// Builds a list of arguments one call at a time, e.g.
// TArgs()("--filter")("*String*").List.
/////////////////////////////////////////////////////////////////////////////
struct TArgs
{
    std::vector<std::string> List;

    TArgs& operator()(std::string const& arg)
    {
        List.push_back(arg);
        return *this;
    }
};

bool Contains(std::vector<std::string> const& list, std::string const& item);

//---------------------------------------------------------------------------
bool Contains(std::vector<std::string> const& list, std::string const& item)
{
    return std::find(list.begin(), list.end(), item) != list.end();
}
//---------------------------------------------------------------------------

} // namespace


namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_CommandLine
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::TTest_ASWUnitTestsBCB_GUI_CommandLine()
    : inherited("ASWUnitTestsBCB_GUI_CommandLine_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::Test_HelpText_MatchesHelpOption()
{
    // Arrange
    TGUIOptions options;
    std::string const helpOutput = TGUICommandLine::Parse(TArgs()("--help").List, options).Message;
    std::string consoleOutput;
    std::string text;

    // Act
    {
        TStdOutRedirect redirect;
        text = TGUICommandLine::HelpText();
        consoleOutput = redirect.Str();
    }

    // Assert
    CheckEquals(helpOutput, text, ASWBCB_HERE, "the same text --help shows");
    CheckContains(text, "--run ", ASWBCB_HERE, "including the GUI-only options");
    CheckEmpty(consoleOutput, ASWBCB_HERE, "nothing was written to std::cout");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::Test_Parse_ConsoleOnlyOptionsAreReportedAsIgnored()
{
    // Arrange
    TGUIOptions options;

    // Act
    TGUICommandLineResult const result = TGUICommandLine::Parse(
        TArgs()("--list")("--pause")("--no-color")("--color-fail")("red").List, options);

    // Assert
    CheckFalse(result.EarlyExitCode.HasValue(), ASWBCB_HERE, "console-only options still open the GUI");
    CheckTrue(Contains(options.IgnoredOptions, "--list"), ASWBCB_HERE, "--list is reported as ignored");
    CheckTrue(Contains(options.IgnoredOptions, "--pause"), ASWBCB_HERE, "--pause is reported as ignored");
    CheckTrue(Contains(options.IgnoredOptions, "--color/--no-color"), ASWBCB_HERE,
        "--no-color is reported as ignored");
    CheckTrue(Contains(options.IgnoredOptions, "--color-pass/--color-fail/--color-skip"), ASWBCB_HERE,
        "--color-fail is reported as ignored");
    CheckEquals(4, options.IgnoredOptions.size(), ASWBCB_HERE, "once each");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::Test_Parse_ExitWithoutRunIsInvalid()
{
    // Arrange
    TGUIOptions options;

    // Act
    TGUICommandLineResult const result = TGUICommandLine::Parse(TArgs()("--exit").List, options);

    // Assert
    AssertTrue(result.EarlyExitCode.HasValue(), ASWBCB_HERE, "the GUI doesn't open");
    CheckEquals(ExitCode_InvalidArguments, result.EarlyExitCode.Value(), ASWBCB_HERE,
        "with the invalid-arguments exit code");
    CheckEquals("Error: --exit requires --run.\n", result.Message, ASWBCB_HERE, "and says why");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::Test_Parse_HelpIncludesGUIOptions()
{
    // Arrange
    TGUIOptions options;
    std::string consoleOutput;
    TGUICommandLineResult result;

    // Act
    {
        TStdOutRedirect redirect;
        result = TGUICommandLine::Parse(TArgs()("--help").List, options);
        consoleOutput = redirect.Str();
    }

    // Assert
    AssertTrue(result.EarlyExitCode.HasValue(), ASWBCB_HERE, "--help doesn't open the GUI");
    CheckEquals(ExitCode_Success, result.EarlyExitCode.Value(), ASWBCB_HERE, "and isn't an error");
    CheckStartsWith(result.Message, "Usage: ASWUnitTestsBCB [options]\n", ASWBCB_HERE,
        "the console runner's usage text comes first");
    CheckContains(result.Message, "--filter <pattern>", ASWBCB_HERE, "the console runner's options are listed");
    CheckEndsWith(result.Message, TGUICommandLine::GUIOptionsHelp(), ASWBCB_HERE, "and then the GUI-only ones");
    CheckContains(result.Message, "--layout-ignore ", ASWBCB_HERE, "including --layout-ignore");
    CheckContains(result.Message, "--layout-reset ", ASWBCB_HERE, "and --layout-reset");
    CheckEmpty(consoleOutput, ASWBCB_HERE, "the parser's output went into the result, not to std::cout");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::Test_Parse_LayoutOptionsAreGUIOnlyOptions()
{
    // Arrange
    TGUIOptions options;

    // Act: the console runner's parser would reject both as unrecognized, so this also proves they never reach it.
    TGUICommandLineResult const result = TGUICommandLine::Parse(
        TArgs()("--layout-ignore")("--filter")("*String*")("--layout-reset").List, options);

    // Assert
    CheckFalse(result.EarlyExitCode.HasValue(), ASWBCB_HERE, "the GUI opens");
    CheckTrue(options.LayoutIgnore, ASWBCB_HERE, "--layout-ignore is recognized");
    CheckTrue(options.LayoutReset, ASWBCB_HERE, "--layout-reset is recognized");
    CheckEquals("*String*", options.CLI.FilterPattern, ASWBCB_HERE,
        "the console options between them are still parsed");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::Test_Parse_NoArgumentsUsesDefaults()
{
    // Arrange
    TGUIOptions options;

    // Act
    TGUICommandLineResult const result = TGUICommandLine::Parse(std::vector<std::string>(), options);

    // Assert
    CheckFalse(result.EarlyExitCode.HasValue(), ASWBCB_HERE, "the GUI opens");
    CheckEmpty(result.Message, ASWBCB_HERE, "with nothing to report");
    CheckFalse(options.RunOnStart, ASWBCB_HERE, "without running anything");
    CheckFalse(options.ExitAfterRun, ASWBCB_HERE, "or exiting");
    CheckFalse(options.LayoutIgnore, ASWBCB_HERE, "the saved layout is loaded and saved");
    CheckFalse(options.LayoutReset, ASWBCB_HERE, "and not reset");
    CheckEmpty(options.IgnoredOptions, ASWBCB_HERE, "no options were ignored");
    CheckEquals("ASWUnitTestsBCB", options.CLI.ProjectName, ASWBCB_HERE, "the console runner's defaults apply");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::Test_Parse_PassesConsoleOptionsThrough()
{
    // Arrange
    TGUIOptions options;

    // Act
    TGUICommandLineResult const result = TGUICommandLine::Parse(TArgs()("--filter")("*String*")("--project-name")(
        "MyProject")("--test-timeout-seconds")("30")("--catch-crashes").List, options);

    // Assert
    CheckFalse(result.EarlyExitCode.HasValue(), ASWBCB_HERE, "the GUI opens");
    CheckTrue(options.CLI.HasFilter, ASWBCB_HERE, "--filter is parsed by the console runner's parser");
    CheckEquals("*String*", options.CLI.FilterPattern, ASWBCB_HERE, "with its pattern");
    CheckEquals("MyProject", options.CLI.ProjectName, ASWBCB_HERE, "--project-name too");
    CheckTrue(options.CLI.TestTimeoutSeconds.HasValue(), ASWBCB_HERE, "--test-timeout-seconds too");
    CheckEquals(30u, options.CLI.TestTimeoutSeconds.Value(), ASWBCB_HERE);
    CheckTrue(options.CLI.CatchCrashes, ASWBCB_HERE, "--catch-crashes too");
    CheckEmpty(options.IgnoredOptions, ASWBCB_HERE, "none of those is ignored");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::Test_Parse_RunAndExitAreGUIOnlyOptions()
{
    // Arrange
    TGUIOptions options;

    // Act: the console runner's parser would reject both as unrecognized, so this also proves they never reach it.
    TGUICommandLineResult const result = TGUICommandLine::Parse(TArgs()("--run")("--filter")("*String*")("--exit").List,
        options);

    // Assert
    CheckFalse(result.EarlyExitCode.HasValue(), ASWBCB_HERE, "the GUI opens");
    CheckTrue(options.RunOnStart, ASWBCB_HERE, "--run is recognized");
    CheckTrue(options.ExitAfterRun, ASWBCB_HERE, "--exit is recognized");
    CheckEquals("*String*", options.CLI.FilterPattern, ASWBCB_HERE,
        "the console options between them are still parsed");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::Test_Parse_UnrecognizedOptionIsInvalid()
{
    // Arrange
    TGUIOptions options;

    // Act
    TGUICommandLineResult const result = TGUICommandLine::Parse(TArgs()("--no-such-option").List, options);

    // Assert
    AssertTrue(result.EarlyExitCode.HasValue(), ASWBCB_HERE, "the GUI doesn't open");
    CheckEquals(ExitCode_InvalidArguments, result.EarlyExitCode.Value(), ASWBCB_HERE,
        "with the invalid-arguments exit code");
    CheckStartsWith(result.Message, "Error: unrecognized option \"--no-such-option\".\n", ASWBCB_HERE,
        "the console runner's own error message is passed on");
    CheckEndsWith(result.Message, TGUICommandLine::GUIOptionsHelp(), ASWBCB_HERE,
        "and the usage text that follows it lists the GUI-only options too");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::Test_Parse_VersionShowsTheVersion()
{
    // Arrange
    TGUIOptions options;

    // Act
    TGUICommandLineResult const result = TGUICommandLine::Parse(TArgs()("--version").List, options);

    // Assert
    AssertTrue(result.EarlyExitCode.HasValue(), ASWBCB_HERE, "--version doesn't open the GUI");
    CheckEquals(ExitCode_Success, result.EarlyExitCode.Value(), ASWBCB_HERE, "and isn't an error");
    CheckEquals(TTestHandler::GetVersionFullStr() + "\n", result.Message, ASWBCB_HERE,
        "the console runner's version text, without the GUI-only options");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_CommandLine::Test_ProcessArguments_MatchesParamCount()
{
    // Arrange: the runner's own arguments vary with how it was started, so only their count can be checked here.
    int const expectedCount = ParamCount();

    // Act
    std::vector<std::string> const args = TGUICommandLine::ProcessArguments();

    // Assert
    CheckEquals(expectedCount, args.size(), ASWBCB_HERE, "one entry per argument, without the program name");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_GUI_CommandLine, "SelfTests.GUI")
