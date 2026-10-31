/* **************************************************************************
ASWUnitTestsBCB_GUI_CommandLine.cpp
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
#include "ASWUnitTestsBCB_GUI_CommandLine.h"
//---------------------------------------------------------------------------
#include <sstream>
//---------------------------------------------------------------------------
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_CLI.h"
#include "ASWUnitTestsBCB_Console.h"
//---------------------------------------------------------------------------
using namespace System;
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TGUIOptions
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TGUIOptions::TGUIOptions()
    : ExitAfterRun(false),
      LayoutIgnore(false),
      LayoutReset(false),
      RunOnStart(false)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TGUICommandLine
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
std::string TGUICommandLine::GUIOptionsHelp()
{
    return
        "\n"
        "GUI-only options:\n"
        "  --run               Run the checked tests once the window opens.\n"
        "  --exit              With --run, close the window when that run finishes,\n"
        "                      returning the same exit code the console runner would.\n"
        "                      Requires --run.\n"
        "  --layout-ignore     Neither load nor save the window layout (its size,\n"
        "                      position, and panel sizes) or the theme; open with the\n"
        "                      defaults.\n"
        "  --layout-reset      Delete the saved window layout (and the theme) and open\n"
        "                      with the defaults; the layout is saved again on closing.\n"
        "\n"
        "Ignored by the GUI: --list, --pause, and the color options.\n";
}
//---------------------------------------------------------------------------
std::string TGUICommandLine::HelpText()
{
    TGUIOptions options;
    return Parse(std::vector<std::string>(1, "--help"), options).Message;
}
//---------------------------------------------------------------------------
TGUICommandLineResult TGUICommandLine::Parse(std::vector<std::string> const& args, TGUIOptions& options)
{
    TGUICommandLineResult result;

    // TCLIParser rejects options it doesn't know, so the GUI-only ones are taken out first. Its args[0] is the
    // program name, which it skips.
    std::vector<std::string> cliArgs(1, "ASWUnitTestsBCB_GUI");

    for (std::vector<std::string>::const_iterator it = args.begin(); it != args.end(); ++it)
    {
        if (*it == "--run")
            options.RunOnStart = true;
        else if (*it == "--exit")
            options.ExitAfterRun = true;
        else if (*it == "--layout-ignore")
            options.LayoutIgnore = true;
        else if (*it == "--layout-reset")
            options.LayoutReset = true;
        else
            cliArgs.push_back(*it);
    }

    std::ostringstream out;
    int exitCode = ExitCode_Success;
    bool const goOn = TCLIParser::ParseArguments(cliArgs, options.CLI, exitCode, out);
    result.Message = out.str();

    // The usage text is written for --help, and after an unrecognized option; either way, it should mention the
    // GUI-only options too.
    if (result.Message.find("Usage: ASWUnitTestsBCB [options]") != std::string::npos)
        result.Message += GUIOptionsHelp();

    if (!goOn)
    {
        result.EarlyExitCode = exitCode;
        return result;
    }

    if (options.ExitAfterRun && !options.RunOnStart)
    {
        result.EarlyExitCode = ExitCode_InvalidArguments;
        result.Message = "Error: --exit requires --run.\n";
        return result;
    }

    if (options.CLI.ListOnly)
        options.IgnoredOptions.push_back("--list");

    if (options.CLI.PauseOnExit)
        options.IgnoredOptions.push_back("--pause");

    if (options.CLI.ColorMode != TColorMode::Auto)
        options.IgnoredOptions.push_back("--color/--no-color");

    if (options.CLI.ColorPass.HasValue() || options.CLI.ColorFail.HasValue() || options.CLI.ColorSkip.HasValue())
        options.IgnoredOptions.push_back("--color-pass/--color-fail/--color-skip");

    return result;
}
//---------------------------------------------------------------------------
std::vector<std::string> TGUICommandLine::ProcessArguments()
{
    std::vector<std::string> args = TCLIParser::SplitCommandLine(::GetCommandLineW());

    if (!args.empty())
        args.erase(args.begin());

    return args;
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
