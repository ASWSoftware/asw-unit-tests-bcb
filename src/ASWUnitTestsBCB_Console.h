/* **************************************************************************
ASWUnitTestsBCB_Console.h
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

#ifndef ASWUnitTestsBCB_ConsoleH
#define ASWUnitTestsBCB_ConsoleH
//---------------------------------------------------------------------------
#include <string>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TConsoleColor
/////////////////////////////////////////////////////////////////////////////
struct TConsoleColor
{
    enum Type
    {
        Default,
        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
        BrightBlack,
        BrightRed,
        BrightGreen,
        BrightYellow,
        BrightBlue,
        BrightMagenta,
        BrightCyan,
        BrightWhite
    };
};


/////////////////////////////////////////////////////////////////////////////
// TColorMode
/////////////////////////////////////////////////////////////////////////////
struct TColorMode
{
    enum Type
    {
        Auto, // Color only if standard output is a console that supports it, and NO_COLOR isn't set
        Always,
        Never
    };
};


/////////////////////////////////////////////////////////////////////////////
// TLogKind
//
// What a piece of output represents, for TConsole::Colorize(). Info is
// never colored; it exists so callers can pass a kind unconditionally.
/////////////////////////////////////////////////////////////////////////////
struct TLogKind
{
    enum Type
    {
        Info,
        Pass,
        Fail,
        Skip
    };
};


/////////////////////////////////////////////////////////////////////////////
// TConsole
//
// Colors the pass, fail and skip status text with ANSI escape sequences
// (as asw-unit-tests does), only when TColorMode::Always asks for it or,
// in the default TColorMode::Auto, when standard output is a console that
// supports them (turning on the console's virtual terminal processing, so
// Windows 10 and later consoles show them) and the NO_COLOR environment
// variable (https://no-color.org) isn't set. The color for each TLogKind
// can be changed with SetColor(). The framework's Log() methods remove the
// sequences again from what an ITestRunObserver receives.
/////////////////////////////////////////////////////////////////////////////
class TConsole
{
public:
    // Wraps 'text' in the escape sequences for 'kind''s color, or returns it unchanged if color is off or 'kind' is
    // TLogKind::Info.
    static std::string Colorize(std::string const& text, TLogKind::Type kind);
    // The color for 'kind'; TConsoleColor::Default for TLogKind::Info.
    static TConsoleColor::Type GetColor(TLogKind::Type kind);
    static TColorMode::Type GetColorMode();
    static bool IsColorEnabled();
    // Parses a color name that --color-pass, --color-fail and --color-skip take, e.g. "red" or "bright-green";
    // returns no value if 'name' isn't one.
    static TOptional<TConsoleColor::Type> ParseColorName(std::string const& name);
    static void SetColor(TLogKind::Type kind, TConsoleColor::Type color);
    static void SetColorMode(TColorMode::Type mode);
    // Returns 'text' without the escape sequences Colorize() adds (any "ESC [ ... m").
    static std::string StripColor(std::string const& text);
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_ConsoleH
