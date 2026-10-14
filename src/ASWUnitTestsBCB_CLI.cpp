/* **************************************************************************
ASWUnitTestsBCB_CLI.cpp
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

#include <System.hpp>
#pragma hdrstop

//---------------------------------------------------------------------------
// Module header
#include "ASWUnitTestsBCB_CLI.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <memory>
#include <set>
//---------------------------------------------------------------------------
#include <windows.h>
#include <shellapi.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TPartitionTestFilter
//
// Matches the tests of one --partition-index, among those another filter
// (if any) matches.
/////////////////////////////////////////////////////////////////////////////
class TPartitionTestFilter : public ASWUnitTestsBCB::ITestFilter
{
private:
    std::set<std::string> m_Members;
    std::unique_ptr<ASWUnitTestsBCB::ITestFilter> const m_Previous; // nullptr for none

    TPartitionTestFilter(TPartitionTestFilter const&);            // Not copyable
    TPartitionTestFilter& operator=(TPartitionTestFilter const&); // Not copyable

public:
    // Takes ownership of 'previous'.
    TPartitionTestFilter(std::set<std::string> const& members, ASWUnitTestsBCB::ITestFilter* previous)
        : m_Members(members),
          m_Previous(previous)
    {
    }

    virtual bool Matches(std::string const& fullTestName) const
    {
        if ((m_Previous.get() != nullptr) && !m_Previous->Matches(fullTestName))
            return false;

        return m_Members.find(fullTestName) != m_Members.end();
    }
};

bool IsValueOption(std::string const& arg, std::string const& name);
void PrintUsage(std::ostream& out);
bool TakeOptionValue(std::vector<std::string> const& args, std::size_t& index, std::string& value);

//---------------------------------------------------------------------------
// Whether 'arg' is the option 'name' that takes a value, given either alone (with the value in the next argument) or
// as "name=value".
bool IsValueOption(std::string const& arg, std::string const& name)
{
    return (arg == name) || (arg.compare(0, name.size() + 1, name + "=") == 0);
}
//---------------------------------------------------------------------------
void PrintUsage(std::ostream& out)
{
    out
    << "Usage: ASWUnitTestsBCB [options]\n"
    "\n"
    "Options:\n"
    "  --filter <pattern>  Run only tests whose full name matches <pattern>.\n"
    "                      A full name is \"GroupName.TestName\", prefixed by\n"
    "                      \"Suite.Path.\" for a group in a suite. '*' matches\n"
    "                      any sequence of characters (including none); '?'\n"
    "                      matches exactly one character. The whole name must\n"
    "                      match, e.g. \"*String*\" for a substring search.\n"
    "  --filter-ignore-case\n"
    "                      Match --filter's <pattern> case-insensitively.\n"
    "                      Has no effect without --filter.\n"
    "  --shuffle           Run groups, and each group's tests, in a randomized\n"
    "                      order instead of the default deterministic order.\n"
    "                      The seed used is logged so a failure caused by\n"
    "                      order can be reproduced via --shuffle-seed.\n"
    "  --shuffle-seed <N>  Shuffle (implies --shuffle) using an explicit\n"
    "                      unsigned integer seed, to reproduce a previous\n"
    "                      --shuffle run's order.\n"
    "  --partition-index <N>\n"
    "                      1-based index of this run's partition, from 1 to\n"
    "                      --partition-count. Requires --partition-count.\n"
    "  --partition-count <N>\n"
    "                      Splits the full test suite into <N> roughly-equal\n"
    "                      partitions by each test's position in the canonical\n"
    "                      registration order (the same order --list shows),\n"
    "                      so every test runs in exactly one partition\n"
    "                      regardless of which group it's in. Run <N> separate\n"
    "                      invocations (e.g. one per CI job), each with its\n"
    "                      own --partition-index, to run the suite in parallel\n"
    "                      with no coordination between processes. Requires\n"
    "                      --partition-index.\n"
    "  --test-timeout-seconds <N>\n"
    "                      Abort the run if any single test does not finish\n"
    "                      within <N> seconds. The offending test is recorded\n"
    "                      as failed (with a message explaining why) and no\n"
    "                      further tests or groups run afterward. There is no\n"
    "                      default; a hung test runs indefinitely unless this\n"
    "                      is given.\n"
    "  --catch-crashes     Catch a native crash in a test that would otherwise\n"
    "                      end the process (a stack overflow or an illegal\n"
    "                      instruction) and record the test as failed. A\n"
    "                      stack overflow aborts the run afterward, the same\n"
    "                      way --test-timeout-seconds does. Off by default.\n"
    "                      An access violation always just fails its test.\n"
    "  --color <mode>      One of \"auto\" (default; color only on an\n"
    "                      interactive terminal that supports it, and only\n"
    "                      if the NO_COLOR environment variable isn't set),\n"
    "                      \"always\", or \"never\".\n"
    "  --no-color          Shorthand for --color never.\n"
    "  --color-pass/--color-fail/--color-skip <color>\n"
    "                      Set the color used for passed/failed/skipped\n"
    "                      status text. <color> is one of: default, black,\n"
    "                      red, green, yellow, blue, magenta, cyan, white,\n"
    "                      or bright-<name> for the bright variant (e.g.\n"
    "                      bright-red). Defaults: pass=green, fail=red,\n"
    "                      skip=yellow.\n"
    "  --report-junit <path>\n"
    "                      Write a JUnit-style XML test report to <path>, in\n"
    "                      addition to the normal console output. Recognized\n"
    "                      by most CI systems (GitHub Actions, GitLab CI,\n"
    "                      Jenkins, Azure DevOps, CircleCI) for native test\n"
    "                      result reporting.\n"
    "  --project-name <name>\n"
    "                      Set the name this run is identified by: shown in\n"
    "                      the console's \"Initializing...\" line and, if\n"
    "                      --report-junit is also given, used as the report's\n"
    "                      <testsuites name=\"...\"> attribute (default:\n"
    "                      \"ASWUnitTestsBCB\").\n"
    "  --list              List all registered tests by full name (see\n"
    "                      --filter) and exit, without running anything.\n"
    "  --pause             Prompt \"Press any key to continue\" before exiting\n"
    "                      after a --list command or test run.\n"
    "  --version           Print the framework version and exit.\n"
    "  --help              Show this message and exit.\n";
}
//---------------------------------------------------------------------------
// Takes the value of the option args[index] (see IsValueOption()): what follows its '=', or else the next argument,
// which 'index' then moves to. Returns false if the option is alone and the last argument.
bool TakeOptionValue(std::vector<std::string> const& args, std::size_t& index, std::string& value)
{
    std::string const& arg = args[index];
    std::string::size_type const equalsPos = arg.find('=');

    if (equalsPos != std::string::npos)
    {
        value = arg.substr(equalsPos + 1);
        return true;
    }

    if ((index + 1) >= args.size())
        return false;

    value = args[++index];
    return true;
}
//---------------------------------------------------------------------------

} // namespace

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TCLIOptions
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TCLIOptions::TCLIOptions()
    : ListOnly(false),
      HasFilter(false),
      FilterIgnoreCase(false),
      Shuffle(false),
      HasPartitionIndex(false),
      HasPartitionCount(false),
      PartitionIndex(0),
      PartitionCount(0),
      CatchCrashes(false),
      ColorMode(TColorMode::Auto),
      ProjectName("ASWUnitTestsBCB"),
      PauseOnExit(false)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TCLIParser
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
ITestFilter* TCLIParser::BuildTestFilter(TTestHandler& tester, TCLIOptions const& options,
    std::string& filterDescription)
{
    std::unique_ptr<ITestFilter> filter;

    filterDescription.clear();

    if (options.HasFilter)
    {
        filter.reset(new TPatternTestFilter(options.FilterPattern, options.FilterIgnoreCase));
        filterDescription = "\"" + options.FilterPattern + "\"" +
            (options.FilterIgnoreCase ? " (case-insensitive)" : "");
    }

    if (options.HasPartitionCount)
    {
        // By each test's position in the unshuffled order, so --shuffle never moves a test to another partition, and
        // the partitions need no coordination between the processes that run them (each writes its own report).
        std::vector<std::string> const allTestNames = tester.GetAllTestFullNames();
        std::set<std::string> members;

        for (std::size_t testPos = 0; testPos < allTestNames.size(); ++testPos)
        {
            if ((testPos % options.PartitionCount) == (options.PartitionIndex - 1))
                members.insert(allTestNames[testPos]);
        }

        filter.reset(new TPartitionTestFilter(members, filter.release()));

        if (!filterDescription.empty())
            filterDescription += ", ";

        filterDescription += "partition " + UIntToString(options.PartitionIndex) + " of " +
            UIntToString(options.PartitionCount);
    }

    return filter.release();
}
//---------------------------------------------------------------------------
bool TCLIParser::ParseArguments(std::vector<std::string> const& args, TCLIOptions& options, int& exitCode,
    std::ostream& out)
{
    for (std::size_t i = 1; i < args.size(); ++i)
    {
        std::string const& arg = args[i];

        if (arg == "--version")
        {
            out << TTestHandler::GetVersionFullStr() << std::endl;
            exitCode = ExitCode_Success;
            return false;
        }
        else if ((arg == "--help") || (arg == "-h"))
        {
            PrintUsage(out);
            exitCode = ExitCode_Success;
            return false;
        }
        else if (arg == "--list")
        {
            options.ListOnly = true;
        }
        else if (IsValueOption(arg, "--filter"))
        {
            if (!TakeOptionValue(args, i, options.FilterPattern))
            {
                out << "Error: --filter requires a pattern argument.\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }

            options.HasFilter = true;
        }
        else if (arg == "--filter-ignore-case")
        {
            options.FilterIgnoreCase = true;
        }
        else if (arg == "--shuffle")
        {
            options.Shuffle = true;
        }
        else if (IsValueOption(arg, "--shuffle-seed"))
        {
            std::string value;

            if (!TakeOptionValue(args, i, value))
            {
                out << "Error: --shuffle-seed requires a numeric argument.\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }

            options.ShuffleSeed = ParseUnsignedInt(value);

            if (!options.ShuffleSeed.HasValue())
            {
                out << "Error: --shuffle-seed requires a non-negative integer argument.\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }

            options.Shuffle = true;
        }
        else if (IsValueOption(arg, "--partition-index") || IsValueOption(arg, "--partition-count"))
        {
            bool const isIndex = IsValueOption(arg, "--partition-index");
            char const* const name = isIndex ? "--partition-index" : "--partition-count";
            std::string value;

            if (!TakeOptionValue(args, i, value))
            {
                out << "Error: " << name << " requires a numeric argument.\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }

            TOptional<unsigned int> const parsed = ParseUnsignedInt(value);

            if (!parsed.HasValue())
            {
                out << "Error: " << name << " requires a positive integer argument.\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }

            if (isIndex)
            {
                options.PartitionIndex = parsed.Value();
                options.HasPartitionIndex = true;
            }
            else
            {
                options.PartitionCount = parsed.Value();
                options.HasPartitionCount = true;
            }
        }
        else if (IsValueOption(arg, "--test-timeout-seconds"))
        {
            std::string value;

            if (!TakeOptionValue(args, i, value))
            {
                out << "Error: --test-timeout-seconds requires a numeric argument.\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }

            options.TestTimeoutSeconds = ParseUnsignedInt(value);

            // Zero would never let a test finish.
            if (!options.TestTimeoutSeconds.HasValue() || (options.TestTimeoutSeconds.Value() == 0))
            {
                out << "Error: --test-timeout-seconds requires a positive integer argument.\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }
        }
        else if (arg == "--catch-crashes")
        {
            options.CatchCrashes = true;
        }
        else if (arg == "--no-color")
        {
            options.ColorMode = TColorMode::Never;
        }
        else if (IsValueOption(arg, "--color"))
        {
            std::string value;
            TOptional<TColorMode::Type> parsed;

            if (TakeOptionValue(args, i, value))
                parsed = ParseColorMode(value);

            if (!parsed.HasValue())
            {
                out << "Error: --color requires \"auto\", \"always\", or \"never\".\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }

            options.ColorMode = parsed.Value();
        }
        else if (IsValueOption(arg, "--color-pass") || IsValueOption(arg, "--color-fail") ||
                 IsValueOption(arg, "--color-skip"))
        {
            std::string const name = arg.substr(0, arg.find('='));
            std::string value;

            if (!TakeOptionValue(args, i, value))
            {
                out << "Error: " << name << " requires a color name argument.\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }

            TOptional<TConsoleColor::Type> const parsed = TConsole::ParseColorName(value);

            if (!parsed.HasValue())
            {
                out << "Error: " << name << " was given an unrecognized color name.\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }

            if (name == "--color-pass")
                options.ColorPass = parsed;
            else if (name == "--color-fail")
                options.ColorFail = parsed;
            else
                options.ColorSkip = parsed;
        }
        else if (IsValueOption(arg, "--report-junit"))
        {
            if (!TakeOptionValue(args, i, options.JUnitReportPath))
            {
                out << "Error: --report-junit requires a file path argument.\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }
        }
        else if (IsValueOption(arg, "--project-name"))
        {
            if (!TakeOptionValue(args, i, options.ProjectName))
            {
                out << "Error: --project-name requires a name argument.\n";
                exitCode = ExitCode_InvalidArguments;
                return false;
            }
        }
        else if (arg == "--pause")
        {
            options.PauseOnExit = true;
        }
        else
        {
            out << "Error: unrecognized option \"" << arg << "\".\n\n";
            PrintUsage(out);
            exitCode = ExitCode_InvalidArguments;
            return false;
        }
    }

    if (options.HasPartitionIndex != options.HasPartitionCount)
    {
        out << "Error: --partition-index and --partition-count must be given together.\n";
        exitCode = ExitCode_InvalidArguments;
        return false;
    }

    if (options.HasPartitionCount)
    {
        if (options.PartitionCount == 0)
        {
            out << "Error: --partition-count must be at least 1.\n";
            exitCode = ExitCode_InvalidArguments;
            return false;
        }

        if ((options.PartitionIndex == 0) || (options.PartitionIndex > options.PartitionCount))
        {
            out << "Error: --partition-index must be between 1 and --partition-count ("
            << options.PartitionCount << ").\n";
            exitCode = ExitCode_InvalidArguments;
            return false;
        }
    }

    return true;
}
//---------------------------------------------------------------------------
TOptional<TColorMode::Type> TCLIParser::ParseColorMode(std::string const& text)
{
    if (text == "auto")
        return TColorMode::Auto;

    if (text == "always")
        return TColorMode::Always;

    if (text == "never")
        return TColorMode::Never;

    return TOptional<TColorMode::Type>();
}
//---------------------------------------------------------------------------
TOptional<unsigned int> TCLIParser::ParseUnsignedInt(std::string const& text)
{
    if (text.empty())
        return TOptional<unsigned int>();

    unsigned int const maxValue = 0xFFFFFFFFu;
    unsigned int value = 0;

    for (std::string::size_type i = 0; i < text.size(); ++i)
    {
        if ((text[i] < '0') || (text[i] > '9'))
            return TOptional<unsigned int>();

        unsigned int const digit = static_cast<unsigned int>(text[i] - '0');

        if (value > ((maxValue - digit) / 10))
            return TOptional<unsigned int>(); // Too large

        value = (value * 10) + digit;
    }

    return value;
}
//---------------------------------------------------------------------------
std::vector<std::string> TCLIParser::SplitCommandLine(wchar_t const* commandLine)
{
    std::vector<std::string> args;
    int argCount = 0;
    LPWSTR* const argList = ::CommandLineToArgvW(commandLine, &argCount);

    if (argList == nullptr)
        return args;

    for (int i = 0; i < argCount; ++i)
        args.push_back(ToUTF8(UnicodeString(argList[i])));

    ::LocalFree(argList);
    return args;
}
//---------------------------------------------------------------------------


// /////// Shared runner helpers ///////////////////////

//---------------------------------------------------------------------------
int ExitCodeForResults(TTestResults const& results)
{
    if (results.TimedOut)
        return ExitCode_TestTimedOut;

    if (results.Crashed)
        return ExitCode_TestCrashed;

    if ((results.FailedCount > 0) || results.Stopped)
        return ExitCode_TestsFailed;

    return ExitCode_Success;
}
//---------------------------------------------------------------------------
std::vector<TJUnitTestCase> ToJUnitTestCases(std::vector<TTestCaseRecord> const& records)
{
    std::vector<TJUnitTestCase> testCases;
    testCases.reserve(records.size());

    for (std::size_t i = 0; i < records.size(); ++i)
    {
        TTestCaseRecord const& record = records[i];
        TJUnitOutcome::Type outcome = TJUnitOutcome::Pass;

        if (record.Outcome == TTestOutcome::Fail)
            outcome = TJUnitOutcome::Fail;
        else if (record.Outcome == TTestOutcome::Skip)
            outcome = TJUnitOutcome::Skip;

        testCases.push_back(TJUnitTestCase(record.GroupName, record.TestName, record.DurationSeconds, outcome,
            record.Message));
    }

    return testCases;
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
