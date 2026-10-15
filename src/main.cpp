/* **************************************************************************
main.cpp
Author: Anthony S. West - ASW Software

Console app that runs the ASWUnitTestsBCB framework.

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
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
//---------------------------------------------------------------------------
#include <System.SysUtils.hpp>
#include <tchar.h>
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_CLI.h"
#include "ASWUnitTestsBCB_Console.h"
#include "ASWUnitTestsBCB_Exception.h"
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_JUnitReport.h"
//---------------------------------------------------------------------------
using namespace ASWUnitTestsBCB;
using namespace System;
using namespace System::Sysutils;
//---------------------------------------------------------------------------

// A Win32 console program that uses the RTL starts with c0x32, which needs __InitVCL and __ExitVCL from vcle.lib
// (bcc32) or rtle.lib (bcc32c). The build links that library only while the project's LinkPackageStatics is empty,
// and the IDE fills it in (e.g. with "rtl.lib") when it saves the project options, after which the link fails. Naming
// the library here keeps the link working whatever the project says; linking it twice is harmless.
#if defined(__BORLANDC__) && !defined(__clang__)
#pragma comment(lib, "vcle")
#elif defined(__BORLANDC__) && defined(__clang__) && !defined(_WIN64)
#pragma comment(lib, "rtle")
#endif

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TUTF8ConsoleOutput
//
// Sets the console's output code page to UTF-8 for as long as it exists,
// and restores the original afterwards, so the UTF-8 text the framework
// writes, such as non-ASCII characters in a message, reads correctly in a
// console window. Output redirected to a file or a pipe is the same UTF-8
// bytes either way.
/////////////////////////////////////////////////////////////////////////////
class TUTF8ConsoleOutput
{
private:
    UINT m_OriginalCodePage; // 0 without a console

    TUTF8ConsoleOutput(TUTF8ConsoleOutput const&);            // Not copyable
    TUTF8ConsoleOutput& operator=(TUTF8ConsoleOutput const&); // Not copyable

public:
    TUTF8ConsoleOutput()
        : m_OriginalCodePage(::GetConsoleOutputCP())
    {
        if (m_OriginalCodePage != 0)
            ::SetConsoleOutputCP(CP_UTF8);
    }

    ~TUTF8ConsoleOutput()
    {
        if (m_OriginalCodePage != 0)
            ::SetConsoleOutputCP(m_OriginalCodePage);
    }
};

void ApplyColorOptions(TCLIOptions const& options);
void PauseIfRequested(bool pauseOnExit);
int RunTests(TCLIOptions const& options);

//---------------------------------------------------------------------------
void ApplyColorOptions(TCLIOptions const& options)
{
    TConsole::SetColorMode(options.ColorMode);

    if (options.ColorPass.HasValue())
        TConsole::SetColor(TLogKind::Pass, options.ColorPass.Value());

    if (options.ColorFail.HasValue())
        TConsole::SetColor(TLogKind::Fail, options.ColorFail.Value());

    if (options.ColorSkip.HasValue())
        TConsole::SetColor(TLogKind::Skip, options.ColorSkip.Value());
}
//---------------------------------------------------------------------------
void PauseIfRequested(bool pauseOnExit)
{
    if (pauseOnExit)
        std::system("pause");
}
//---------------------------------------------------------------------------
// Runs every registered test and returns the program's exit code (see ASWUnitTestsBCB_CLI.h). After a timeout or a
// crash that ended the run, the handler and its groups are left as they are, never destroyed: an abandoned test's
// thread may still be using its group (see main()).
int RunTests(TCLIOptions const& options)
{
    try
    {
        std::unique_ptr<TTestHandler> tester(new TTestHandler());

        tester->Initialize(options.ProjectName);

        std::string filterDescription;
        std::unique_ptr<ITestFilter> const filter(TCLIParser::BuildTestFilter(*tester, options, filterDescription));

        if (options.ListOnly)
        {
            // Applying the filter here lets the caller check a pattern's matches before running it.
            tester->ListTests(filter.get(), filterDescription);
            return ExitCode_Success;
        }

        TTestResults const testResults = tester->Run(filter.get(), filterDescription, options.Shuffle,
            options.ShuffleSeed, options.TestTimeoutSeconds, options.CatchCrashes);

        if (!options.JUnitReportPath.empty())
        {
            if (TJUnitReportWriter::Write(options.JUnitReportPath, tester->GetProjectName(),
                ToJUnitTestCases(testResults.CaseRecords)))
                tester->Log("JUnit report written to: " + options.JUnitReportPath);
            else
                tester->Log("Error: could not write JUnit report to: " + options.JUnitReportPath);
        }

        int const exitCode = ExitCodeForResults(testResults);

        if ((exitCode == ExitCode_TestTimedOut) || (exitCode == ExitCode_TestCrashed))
            static_cast<void>(tester.release());

        return exitCode;
    }
    catch (TExceptFatalError const& fatalEx)
    {
        std::cout << "\nTerminating app. Unhandled exception: " << fatalEx.what() << std::endl;
        return fatalEx.IsUnknownException() ? ExitCode_UnhandledExceptionUnknown : ExitCode_UnhandledException;
    }
    catch (std::exception const& stdEx)
    {
        std::cout << "\nTerminating app. Unhandled exception: " << DescribeException(stdEx) << std::endl;
        return ExitCode_UnhandledException;
    }
    catch (Exception& rtlEx)
    {
        std::cout << "\nTerminating app. Unhandled exception: " << DescribeException(rtlEx) << std::endl;
        return ExitCode_UnhandledException;
    }
    catch (...)
    {
        std::cout << "\nTerminating app. Unhandled exception: Unknown" << std::endl;
        return ExitCode_UnhandledExceptionUnknown;
    }
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------
// _tmain is main, or wmain when the project's "TCHAR maps to" option is wchar_t (_UNICODE), which links a startup
// module that calls wmain. The arguments aren't used either way (see below), so a test project builds with either.
int _tmain(int /*argc*/, _TCHAR* /*argv*/[])
{
    int exitCode = ExitCode_Success;

    {
        TUTF8ConsoleOutput const utf8ConsoleOutput;
        TCLIOptions options;

        // The arguments come from the wide command line, not argv, so that non-ASCII text in them arrives intact.
        if (!TCLIParser::ParseArguments(TCLIParser::SplitCommandLine(::GetCommandLineW()), options, exitCode,
            std::cout))
            return exitCode;

        ApplyColorOptions(options);

        exitCode = RunTests(options);

        PauseIfRequested(options.PauseOnExit);
    }

    // After a timeout, an abandoned test's thread may still be running, and after a stack overflow the process's
    // state may be damaged, so end the process now, as asw-unit-tests does (with std::_Exit()), without the static
    // destructors and exit procedures the runtime libraries would run.
    if ((exitCode == ExitCode_TestTimedOut) || (exitCode == ExitCode_TestCrashed))
    {
        std::cout.flush();
        ::ExitProcess(static_cast<UINT>(exitCode));
    }

    return exitCode;
}
