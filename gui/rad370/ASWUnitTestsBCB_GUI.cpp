/* **************************************************************************
ASWUnitTestsBCB_GUI.cpp
Author: Anthony S. West - ASW Software

VCL GUI app that runs the ASWUnitTestsBCB framework's tests.

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
#include <exception>
#include <string>
//---------------------------------------------------------------------------
#include <System.SysUtils.hpp>
#include <Vcl.Forms.hpp>
#include <tchar.h>
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_CLI.h"
#include "ASWUnitTestsBCB_Exception.h"
#include "ASWUnitTestsBCB_GUI_CommandLine.h"
#include "ASWUnitTestsBCB_GUI_TextDialog.h"
//---------------------------------------------------------------------------
// Before the form's own header, which declares the form variable USEFORM() declares too, as the IDE writes it.
USEFORM("..\src\ASWUnitTestsBCB_GUI_MainForm.cpp", FormASWUnitTestsBCBGUIMain);
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_MainForm.h"
//---------------------------------------------------------------------------
using namespace ASWUnitTestsBCB;
using namespace System;
using namespace System::Sysutils;
//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
    int exitCode = ExitCode_UnhandledExceptionUnknown; // Unless something sets it
    bool runAborted = false;

    try
    {
        Application->Initialize();
        Application->MainFormOnTaskBar = true;
        Application->Title = "ASWUnitTestsBCB";

        // Parsed before the main form exists, so --help, --version, and argument errors are shown on their own and the
        // process exits with the same code the console runner would use, without opening the GUI.
        TGUIOptions options;
        TGUICommandLineResult const commandLine = TGUICommandLine::Parse(TGUICommandLine::ProcessArguments(),
            options);

        if (commandLine.EarlyExitCode.HasValue())
        {
            ShowTextDialog(Application->Title, commandLine.Message);
            return commandLine.EarlyExitCode.Value();
        }

        Application->CreateForm(__classid(TFormASWUnitTestsBCBGUIMain), &FormASWUnitTestsBCBGUIMain);
        FormASWUnitTestsBCBGUIMain->Start(options);
        Application->Run();

        // The form outlives Run(); Application frees it later, during shutdown.
        exitCode = FormASWUnitTestsBCBGUIMain->ExitCode();
        runAborted = FormASWUnitTestsBCBGUIMain->WasRunAborted();
    }
    catch (Exception& exception)
    {
        Application->ShowException(&exception);
        exitCode = ExitCode_UnhandledException;
    }
    catch (std::exception const& exception)
    {
        ShowTextDialog(Application->Title, "Unhandled exception: " + DescribeException(exception));
        exitCode = ExitCode_UnhandledException;
    }
    catch (...)
    {
        ShowTextDialog(Application->Title, "Unhandled exception: " + DescribeUnknownException());
    }

    // After a timeout, an abandoned test's thread may still be running (and may still log to the form), and after a
    // stack overflow the process's state may be damaged, so end the process now, as the console runner does, without
    // the cleanup that would destroy the form and the test groups.
    if (runAborted)
        ::ExitProcess(static_cast<UINT>(exitCode));

    return exitCode;
}
//---------------------------------------------------------------------------
