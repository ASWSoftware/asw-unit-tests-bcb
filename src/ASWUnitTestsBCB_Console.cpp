/* **************************************************************************
ASWUnitTestsBCB_Console.cpp
Author: Anthony S. West - ASW Software

Colored console output.

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
#include "ASWUnitTestsBCB_Console.h"
//---------------------------------------------------------------------------
#include <cstddef>
//---------------------------------------------------------------------------
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------
using namespace ASWUnitTestsBCB;
//---------------------------------------------------------------------------

#if !defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING)
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

//---------------------------------------------------------------------------

namespace
{

// A color name that --color-pass, --color-fail and --color-skip take.
struct TColorName
{
    char const* Name;
    TConsoleColor Color;
};

TColorName const ColorNames[] =
{
    { "default", TConsoleColor::Default },
    { "black", TConsoleColor::Black },
    { "red", TConsoleColor::Red },
    { "green", TConsoleColor::Green },
    { "yellow", TConsoleColor::Yellow },
    { "blue", TConsoleColor::Blue },
    { "magenta", TConsoleColor::Magenta },
    { "cyan", TConsoleColor::Cyan },
    { "white", TConsoleColor::White },
    { "bright-black", TConsoleColor::BrightBlack },
    { "bright-red", TConsoleColor::BrightRed },
    { "bright-green", TConsoleColor::BrightGreen },
    { "bright-yellow", TConsoleColor::BrightYellow },
    { "bright-blue", TConsoleColor::BrightBlue },
    { "bright-magenta", TConsoleColor::BrightMagenta },
    { "bright-cyan", TConsoleColor::BrightCyan },
    { "bright-white", TConsoleColor::BrightWhite }
};

TColorMode g_ColorMode = TColorMode::Auto;
HANDLE g_ColorCheckedOutput = INVALID_HANDLE_VALUE; // The standard output DetectColorSupport() last checked
bool g_ColorSupported = false;                      // What it found
TConsoleColor g_FailColor = TConsoleColor::Red;
TConsoleColor g_PassColor = TConsoleColor::Green;
TConsoleColor g_SkipColor = TConsoleColor::Yellow;

//---------------------------------------------------------------------------

char const* AnsiCode(TConsoleColor color);
bool DetectColorSupport(HANDLE output);
bool NoColorRequested();

//---------------------------------------------------------------------------
/*
    AnsiCode

    The escape sequence that sets 'color' as the text color.
*/
char const* AnsiCode(TConsoleColor color)
{
    static char const* const codes[] =
    {
        "\x1b[39m", // Default
        "\x1b[30m", // Black
        "\x1b[31m", // Red
        "\x1b[32m", // Green
        "\x1b[33m", // Yellow
        "\x1b[34m", // Blue
        "\x1b[35m", // Magenta
        "\x1b[36m", // Cyan
        "\x1b[37m", // White
        "\x1b[90m", // BrightBlack
        "\x1b[91m", // BrightRed
        "\x1b[92m", // BrightGreen
        "\x1b[93m", // BrightYellow
        "\x1b[94m", // BrightBlue
        "\x1b[95m", // BrightMagenta
        "\x1b[96m", // BrightCyan
        "\x1b[97m"  // BrightWhite
    };

    return codes[static_cast<int>(color)];
}
//---------------------------------------------------------------------------
/*
    DetectColorSupport

    Whether 'output' is a console that shows escape sequences, turning its virtual terminal processing on. Output
    redirected to a file or a pipe isn't.
*/
bool DetectColorSupport(HANDLE output)
{
    if ((output == INVALID_HANDLE_VALUE) || (output == nullptr))
        return false;

    DWORD mode = 0;

    if (!::GetConsoleMode(output, &mode))
        return false;

    return ::SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}
//---------------------------------------------------------------------------
/*
    NoColorRequested

    Whether the NO_COLOR environment variable is set to something (https://no-color.org). Read from the process's
    environment, not the C runtime's copy, so a change made with SetEnvironmentVariable() counts.
*/
bool NoColorRequested()
{
    wchar_t value[2];
    return ::GetEnvironmentVariableW(L"NO_COLOR", value, sizeof(value) / sizeof(value[0])) > 0;
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TConsole
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
std::string TConsole::Colorize(std::string const& text, TLogKind kind)
{
    if ((kind == TLogKind::Info) || !IsColorEnabled())
        return text;

    return AnsiCode(GetColor(kind)) + text + "\x1b[0m";
}
//---------------------------------------------------------------------------
TConsoleColor TConsole::GetColor(TLogKind kind)
{
    switch (kind)
    {
        case TLogKind::Pass:
            return g_PassColor;

        case TLogKind::Fail:
            return g_FailColor;

        case TLogKind::Skip:
            return g_SkipColor;

        case TLogKind::Info:
        default:
            return TConsoleColor::Default;
    }
}
//---------------------------------------------------------------------------
TColorMode TConsole::GetColorMode()
{
    return g_ColorMode;
}
//---------------------------------------------------------------------------
bool TConsole::IsColorEnabled()
{
    if (g_ColorMode == TColorMode::Always)
        return true;

    if (g_ColorMode == TColorMode::Never)
        return false;

    if (NoColorRequested())
        return false;

    // Checked again only when standard output changes (e.g. SetStdHandle() redirected it), since checking turns the
    // console's virtual terminal processing on
    HANDLE const output = ::GetStdHandle(STD_OUTPUT_HANDLE);

    if (output != g_ColorCheckedOutput)
    {
        g_ColorSupported = DetectColorSupport(output);
        g_ColorCheckedOutput = output;
    }

    return g_ColorSupported;
}
//---------------------------------------------------------------------------
TOptional<TConsoleColor> TConsole::ParseColorName(std::string const& name)
{
    for (std::size_t i = 0; i < (sizeof(ColorNames) / sizeof(ColorNames[0])); ++i)
    {
        if (name == ColorNames[i].Name)
            return ColorNames[i].Color;
    }

    return TOptional<TConsoleColor>();
}
//---------------------------------------------------------------------------
void TConsole::SetColor(TLogKind kind, TConsoleColor color)
{
    switch (kind)
    {
        case TLogKind::Pass:
            g_PassColor = color;
            break;

        case TLogKind::Fail:
            g_FailColor = color;
            break;

        case TLogKind::Skip:
            g_SkipColor = color;
            break;

        case TLogKind::Info:
        default:
            break; // Info is never colored
    }
}
//---------------------------------------------------------------------------
void TConsole::SetColorMode(TColorMode mode)
{
    g_ColorMode = mode;
}
//---------------------------------------------------------------------------
std::string TConsole::StripColor(std::string const& text)
{
    if (text.find('\x1b') == std::string::npos)
        return text;

    std::string stripped;
    std::string::size_type i = 0;

    while (i < text.size())
    {
        // A sequence is ESC, '[', any digits and ';', then 'm'; anything else is kept as it is.
        std::string::size_type end = i + 2;

        if ((text[i] == '\x1b') && (end <= text.size()) && (text[i + 1] == '['))
        {
            while ((end < text.size()) && (((text[end] >= '0') && (text[end] <= '9')) || (text[end] == ';')))
                ++end;

            if ((end < text.size()) && (text[end] == 'm'))
            {
                i = end + 1;
                continue;
            }
        }

        stripped += text[i++];
    }

    return stripped;
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
