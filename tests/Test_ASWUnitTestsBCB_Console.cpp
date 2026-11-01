/* **************************************************************************
Test_ASWUnitTestsBCB_Console.cpp
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_Console.h.

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
#include "Test_ASWUnitTestsBCB_Console.h"
//---------------------------------------------------------------------------
#include <string>
//---------------------------------------------------------------------------
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Console.h"
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_StdOutRedirect.h"
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TColorSettingsGuard
//
// Restores TConsole's color mode and colors when it goes out of scope, so
// a test can change them without affecting the rest of the run.
/////////////////////////////////////////////////////////////////////////////
class TColorSettingsGuard
{
private:
    ASWUnitTestsBCB::TColorMode m_Mode;
    ASWUnitTestsBCB::TConsoleColor m_Pass;
    ASWUnitTestsBCB::TConsoleColor m_Fail;
    ASWUnitTestsBCB::TConsoleColor m_Skip;

    TColorSettingsGuard(TColorSettingsGuard const&);            // Not copyable
    TColorSettingsGuard& operator=(TColorSettingsGuard const&); // Not copyable

public:
    TColorSettingsGuard()
        : m_Mode(ASWUnitTestsBCB::TConsole::GetColorMode()),
          m_Pass(ASWUnitTestsBCB::TConsole::GetColor(ASWUnitTestsBCB::TLogKind::Pass)),
          m_Fail(ASWUnitTestsBCB::TConsole::GetColor(ASWUnitTestsBCB::TLogKind::Fail)),
          m_Skip(ASWUnitTestsBCB::TConsole::GetColor(ASWUnitTestsBCB::TLogKind::Skip))
    {
    }

    ~TColorSettingsGuard()
    {
        ASWUnitTestsBCB::TConsole::SetColorMode(m_Mode);
        ASWUnitTestsBCB::TConsole::SetColor(ASWUnitTestsBCB::TLogKind::Pass, m_Pass);
        ASWUnitTestsBCB::TConsole::SetColor(ASWUnitTestsBCB::TLogKind::Fail, m_Fail);
        ASWUnitTestsBCB::TConsole::SetColor(ASWUnitTestsBCB::TLogKind::Skip, m_Skip);
    }
};


/////////////////////////////////////////////////////////////////////////////
// TRecordingObserver
//
// Records the log text an observer receives.
/////////////////////////////////////////////////////////////////////////////
class TRecordingObserver : public ASWUnitTestsBCB::ITestRunObserver
{
public:
    std::string LogText;

    virtual void OnLog(std::string const& text)
    {
        LogText += text;
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

//---------------------------------------------------------------------------

void UseDefaultColors();

//---------------------------------------------------------------------------
/*
    UseDefaultColors

    Sets the default colors (green, red, yellow), which --color-pass, --color-fail or --color-skip may have changed
    for this run.
*/
void UseDefaultColors()
{
    ASWUnitTestsBCB::TConsole::SetColor(ASWUnitTestsBCB::TLogKind::Pass, ASWUnitTestsBCB::TConsoleColor::Green);
    ASWUnitTestsBCB::TConsole::SetColor(ASWUnitTestsBCB::TLogKind::Fail, ASWUnitTestsBCB::TConsoleColor::Red);
    ASWUnitTestsBCB::TConsole::SetColor(ASWUnitTestsBCB::TLogKind::Skip, ASWUnitTestsBCB::TConsoleColor::Yellow);
}
//---------------------------------------------------------------------------

} // namespace


namespace ASWUnitTestsBCB
{

// The inner group the tests below run. It isn't registered, so the program's own run never includes it.

/////////////////////////////////////////////////////////////////////////////
// TInnerCon_Mixed
/////////////////////////////////////////////////////////////////////////////
class TInnerCon_Mixed : public TTestGroupBase
{
public:
    __fastcall TInnerCon_Mixed()
        : TTestGroupBase("Mixed")
    {
    }

__published:
    void __fastcall Test_Fails();
    void __fastcall Test_Passes();
    void __fastcall Test_Skips();
};

void __fastcall TInnerCon_Mixed::Test_Fails()
{
    CheckTrue(false, ASWBCB_HERE);
}

void __fastcall TInnerCon_Mixed::Test_Passes()
{
}

void __fastcall TInnerCon_Mixed::Test_Skips()
{
    Skip(ASWBCB_HERE, "why");
}


/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_Console
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_Console::TTest_ASWUnitTestsBCB_Console()
    : inherited("ASWUnitTestsBCB_Console_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Console::Test_Colorize_ByModeAndKind()
{
    TColorSettingsGuard const restoreColors;
    UseDefaultColors();

    TConsole::SetColorMode(TColorMode::Always);
    CheckTrue(TConsole::IsColorEnabled(), ASWBCB_HERE, "always");
    CheckEquals("\x1b[32mok\x1b[0m", TConsole::Colorize("ok", TLogKind::Pass), ASWBCB_HERE, "pass: green");
    CheckEquals("\x1b[31mok\x1b[0m", TConsole::Colorize("ok", TLogKind::Fail), ASWBCB_HERE, "fail: red");
    CheckEquals("\x1b[33mok\x1b[0m", TConsole::Colorize("ok", TLogKind::Skip), ASWBCB_HERE, "skip: yellow");
    CheckEquals("ok", TConsole::Colorize("ok", TLogKind::Info), ASWBCB_HERE, "info: never colored");

    TConsole::SetColor(TLogKind::Pass, TConsoleColor::BrightBlue);
    TConsole::SetColor(TLogKind::Fail, TConsoleColor::Default);
    TConsole::SetColor(TLogKind::Skip, TConsoleColor::Black);
    TConsole::SetColor(TLogKind::Info, TConsoleColor::Red);
    CheckEquals("\x1b[94mok\x1b[0m", TConsole::Colorize("ok", TLogKind::Pass), ASWBCB_HERE, "bright blue");
    CheckEquals("\x1b[39mok\x1b[0m", TConsole::Colorize("ok", TLogKind::Fail), ASWBCB_HERE, "default");
    CheckEquals("\x1b[30mok\x1b[0m", TConsole::Colorize("ok", TLogKind::Skip), ASWBCB_HERE, "black");
    CheckEquals("ok", TConsole::Colorize("ok", TLogKind::Info), ASWBCB_HERE, "info has no color to set");
    CheckEquals(TConsoleColor::BrightBlue, TConsole::GetColor(TLogKind::Pass), ASWBCB_HERE);
    CheckEquals(TConsoleColor::Default, TConsole::GetColor(TLogKind::Info), ASWBCB_HERE);

    TConsole::SetColorMode(TColorMode::Never);
    CheckFalse(TConsole::IsColorEnabled(), ASWBCB_HERE, "never");
    CheckEquals("ok", TConsole::Colorize("ok", TLogKind::Fail), ASWBCB_HERE, "never");
    CheckEquals(TColorMode::Never, TConsole::GetColorMode(), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Console::Test_IsColorEnabled_AutoFollowsOutputAndNoColor()
{
    // Arrange: this process's console, even when the run's output is redirected, and a file
    HANDLE const console = ::CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);

    if (console == INVALID_HANDLE_VALUE)
        Skip(ASWBCB_HERE, "this process has no console");

    HANDLE const file = ::CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0,
        nullptr);
    TColorSettingsGuard const restoreColors;
    HANDLE const originalOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
    wchar_t originalNoColor[256];
    DWORD const originalNoColorLength = ::GetEnvironmentVariableW(L"NO_COLOR", originalNoColor,
        sizeof(originalNoColor) / sizeof(originalNoColor[0]));

    TConsole::SetColorMode(TColorMode::Auto);
    ::SetEnvironmentVariableW(L"NO_COLOR", nullptr);

    // Act: standard output a file, the console, then the file again
    ::SetStdHandle(STD_OUTPUT_HANDLE, file);
    bool const toFile = TConsole::IsColorEnabled();
    ::SetStdHandle(STD_OUTPUT_HANDLE, console);
    bool const toConsole = TConsole::IsColorEnabled();
    ::SetEnvironmentVariableW(L"NO_COLOR", L"");
    bool const withEmptyNoColor = TConsole::IsColorEnabled();
    ::SetEnvironmentVariableW(L"NO_COLOR", L"1");
    bool const withNoColor = TConsole::IsColorEnabled();
    std::string const colorized = TConsole::Colorize("ok", TLogKind::Pass);
    ::SetEnvironmentVariableW(L"NO_COLOR", nullptr);
    ::SetStdHandle(STD_OUTPUT_HANDLE, file);
    bool const toFileAgain = TConsole::IsColorEnabled();

    ::SetEnvironmentVariableW(L"NO_COLOR", (originalNoColorLength > 0) ? originalNoColor : nullptr);
    ::SetStdHandle(STD_OUTPUT_HANDLE, originalOutput);
    ::CloseHandle(file);
    ::CloseHandle(console);

    // Assert
    CheckFalse(toFile, ASWBCB_HERE, "auto mode doesn't color a file");
    CheckTrue(toConsole, ASWBCB_HERE, "auto mode colors a console (Windows 10 and later)");
    CheckTrue(withEmptyNoColor, ASWBCB_HERE, "an empty NO_COLOR doesn't count");
    CheckFalse(withNoColor, ASWBCB_HERE, "NO_COLOR turns color off in auto mode");
    CheckEquals("ok", colorized, ASWBCB_HERE, "NO_COLOR");
    CheckFalse(toFileAgain, ASWBCB_HERE, "auto mode follows standard output");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Console::Test_Log_ColorsTheConsoleOnly()
{
    // Arrange
    TColorSettingsGuard const restoreColors;
    UseDefaultColors();
    TConsole::SetColorMode(TColorMode::Always);
    TTestGroupRegistrar const mixed(CreateTestGroup<TInnerCon_Mixed>, __classid(TInnerCon_Mixed), "TInnerCon_Mixed", "",
        0, nullptr);
    std::string console;
    TRecordingObserver observer;

    // Act: a run that writes to the console, and one that reports to an observer
    {
        TStdOutRedirect const capture;
        TTestHandler handler;
        handler.Initialize("Inner", &mixed);
        handler.Run();
        console = capture.Str();
    }

    {
        TTestHandler handler;
        handler.SetRunObserver(&observer);
        handler.Initialize("Inner", &mixed);
        handler.Run();
    }

    // Assert: the status text colored on the console, as asw-unit-tests colors it
    CheckContains(console, "\n\x1b[31m***Test failed\x1b[0m: \"Mixed.Fails\"", ASWBCB_HERE);
    CheckContains(console, "Finished test: \"Mixed.Fails\" - \x1b[31mfailed\x1b[0m (", ASWBCB_HERE);
    CheckContains(console, "Finished test: \"Mixed.Passes\" - \x1b[32mpassed\x1b[0m (", ASWBCB_HERE);
    CheckContains(console, "\n\x1b[33m***Test skipped\x1b[0m: \"Mixed.Skips\": ", ASWBCB_HERE);
    CheckContains(console, "Finished test: \"Mixed.Skips\" - \x1b[33mskipped\x1b[0m (", ASWBCB_HERE);
    CheckContains(console, "\nDone. \x1b[32mSucceeded: 1\x1b[0m, \x1b[31mfailed: 1\x1b[0m, \x1b[33mskipped: 1\x1b[0m\n",
        ASWBCB_HERE, "the group's totals");
    CheckContains(console, "] Tests done: Totals: \x1b[32msucceeded: 1\x1b[0m, \x1b[31mfailed: 1\x1b[0m, "
        "\x1b[33mskipped: 1\x1b[0m\n", ASWBCB_HERE, "the run's totals");

    // and the same lines plain for an observer
    std::string const& log = observer.LogText;
    CheckEquals(std::string::npos, log.find('\x1b'), ASWBCB_HERE, "no escape sequences");
    CheckContains(log, "\n***Test failed: \"Mixed.Fails\"", ASWBCB_HERE);
    CheckContains(log, "Finished test: \"Mixed.Passes\" - passed (", ASWBCB_HERE);
    CheckContains(log, "\nDone. Succeeded: 1, failed: 1, skipped: 1\n", ASWBCB_HERE);
    CheckContains(log, "] Tests done: Totals: succeeded: 1, failed: 1, skipped: 1\n", ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Console::Test_ParseColorName()
{
    char const* const names[] =
    {
        "default", "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white", "bright-black",
        "bright-red", "bright-green", "bright-yellow", "bright-blue", "bright-magenta", "bright-cyan", "bright-white"
    };

    for (int i = 0; i < static_cast<int>(sizeof(names) / sizeof(names[0])); ++i)
    {
        TOptional<TConsoleColor> const color = TConsole::ParseColorName(names[i]);
        CheckTrue(color.HasValue(), ASWBCB_HERE, names[i]);
        CheckEquals(i, static_cast<int>(color.Value()), ASWBCB_HERE, names[i]);
    }

    CheckFalse(TConsole::ParseColorName("").HasValue(), ASWBCB_HERE, "empty");
    CheckFalse(TConsole::ParseColorName("bogus").HasValue(), ASWBCB_HERE, "unknown");
    CheckFalse(TConsole::ParseColorName("Red").HasValue(), ASWBCB_HERE, "case-sensitive");
    CheckFalse(TConsole::ParseColorName("bright").HasValue(), ASWBCB_HERE, "'bright' alone");
    CheckFalse(TConsole::ParseColorName("bright red").HasValue(), ASWBCB_HERE, "a space for the hyphen");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Console::Test_StripColor()
{
    CheckEquals("plain", TConsole::StripColor("plain"), ASWBCB_HERE);
    CheckEquals("", TConsole::StripColor(""), ASWBCB_HERE);
    CheckEquals("a ok b", TConsole::StripColor("a \x1b[32mok\x1b[0m b"), ASWBCB_HERE, "Colorize()' sequences");
    CheckEquals("X", TConsole::StripColor("\x1b[1;31mX\x1b[m"), ASWBCB_HERE, "parameters, or none");
    CheckEquals("\x1b[2Jx", TConsole::StripColor("\x1b[2Jx"), ASWBCB_HERE, "another sequence stays");
    CheckEquals("\x1b" "x", TConsole::StripColor("\x1b" "x"), ASWBCB_HERE, "an escape without '[' stays");
    CheckEquals("x\x1b[", TConsole::StripColor("x\x1b["), ASWBCB_HERE, "an unfinished sequence at the end stays");
    CheckEquals("x\x1b", TConsole::StripColor("x\x1b"), ASWBCB_HERE, "an escape at the end stays");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_Console, "SelfTests")
