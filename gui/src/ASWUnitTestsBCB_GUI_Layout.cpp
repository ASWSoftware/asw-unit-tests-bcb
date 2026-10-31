/* **************************************************************************
ASWUnitTestsBCB_GUI_Layout.cpp
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
#include "ASWUnitTestsBCB_GUI_Layout.h"
//---------------------------------------------------------------------------
#include <algorithm>
#include <cmath>
//---------------------------------------------------------------------------
#include <System.IniFiles.hpp>
#include <System.IOUtils.hpp>
#include <System.SysUtils.hpp>
#include <System.Types.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Inifiles;
using namespace System::Ioutils;
using namespace System::Sysutils;
using namespace System::Types;
//---------------------------------------------------------------------------

namespace
{

char const PanelsSection[] = "Panels";
char const WindowSection[] = "Window";

// Smallest sizes accepted from a layout file, at 96 DPI; anything smaller is treated as invalid.
int const MinPanelSize = 1;
int const MinWindowHeight = 150;
int const MinWindowWidth = 200;

// How much of a window's top edge must be on a work area for IsWindowReachable(), in pixels: the top TitleBarHeight
// pixels, at least MinGrabWidth of them across (or the window's whole width, if narrower).
int const MinGrabWidth = 100;
int const TitleBarHeight = 30;

ASWUnitTestsBCB::TOptional<int> ReadInt(TCustomIniFile& ini, char const* section, char const* ident);
ASWUnitTestsBCB::TOptional<int> ReadSize(TCustomIniFile& ini, char const* section, char const* ident, int minimum);
int RoundToInt(double value);
void WriteOrDelete(TCustomIniFile& ini, char const* section, char const* ident,
    ASWUnitTestsBCB::TOptional<int> const& value);

//---------------------------------------------------------------------------
/*
    ReadInt

    The value, or no value if it's missing or isn't a whole number.
*/
ASWUnitTestsBCB::TOptional<int> ReadInt(TCustomIniFile& ini, char const* section, char const* ident)
{
    int value = 0;

    if (!TryStrToInt(ini.ReadString(section, ident, UnicodeString()), value))
        return ASWUnitTestsBCB::TOptional<int>();

    return value;
}
//---------------------------------------------------------------------------
/*
    ReadSize

    The value, or no value if it's missing, isn't a whole number, or is less than 'minimum'.
*/
ASWUnitTestsBCB::TOptional<int> ReadSize(TCustomIniFile& ini, char const* section, char const* ident, int minimum)
{
    ASWUnitTestsBCB::TOptional<int> const value = ReadInt(ini, section, ident);

    if (!value.HasValue() || (value.Value() < minimum))
        return ASWUnitTestsBCB::TOptional<int>();

    return value;
}
//---------------------------------------------------------------------------
/*
    RoundToInt

    Rounds to the nearest whole number, halves away from zero, as std::lround (C++11) does.
*/
int RoundToInt(double value)
{
    return static_cast<int>((value < 0.0) ? -std::floor(-value + 0.5) : std::floor(value + 0.5));
}
//---------------------------------------------------------------------------
void WriteOrDelete(TCustomIniFile& ini, char const* section, char const* ident,
    ASWUnitTestsBCB::TOptional<int> const& value)
{
    if (value.HasValue())
        ini.WriteInteger(section, ident, value.Value());
    else
        ini.DeleteKey(section, ident);
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TGUIWindowBounds
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TGUIWindowBounds::TGUIWindowBounds()
    : Height(0),
      Left(0),
      Top(0),
      Width(0)
{
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TGUILayout
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TGUILayout::TGUILayout()
    : Maximized(false)
{
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
UnicodeString GUILayoutFilePath()
{
    // GetHomePath() is %APPDATA% on Windows.
    UnicodeString const folder = TPath::Combine(TPath::GetHomePath(), "ASWUnitTestsBCB");
    UnicodeString const fileName = ChangeFileExt(ExtractFileName(ParamStr(0)), ".ini");

    return TPath::Combine(folder, fileName);
}
//---------------------------------------------------------------------------
bool IsWindowReachable(TRect const& bounds, std::vector<TRect> const& workAreas)
{
    if ((bounds.Width() <= 0) || (bounds.Height() <= 0))
        return false;

    int const titleBarBottom = bounds.Top + std::min(bounds.Height(), TitleBarHeight);
    int const minWidth = std::min(bounds.Width(), MinGrabWidth);

    for (std::vector<TRect>::const_iterator it = workAreas.begin(); it != workAreas.end(); ++it)
    {
        int const overlapWidth = std::min(bounds.Right, it->Right) - std::max(bounds.Left, it->Left);
        bool const titleBarInside = (bounds.Top >= it->Top) && (titleBarBottom <= it->Bottom);

        if (titleBarInside && (overlapWidth >= minWidth))
            return true;
    }

    return false;
}
//---------------------------------------------------------------------------
TGUILayout LoadGUILayout(TCustomIniFile& ini)
{
    TGUILayout layout;

    layout.DetailHeight = ReadSize(ini, PanelsSection, "DetailHeight", MinPanelSize);
    layout.Maximized = ini.ReadBool(WindowSection, "Maximized", false);
    layout.TestTreeWidth = ReadSize(ini, PanelsSection, "TestTreeWidth", MinPanelSize);

    TOptional<int> const height = ReadSize(ini, WindowSection, "Height", MinWindowHeight);
    TOptional<int> const left = ReadInt(ini, WindowSection, "Left");
    TOptional<int> const top = ReadInt(ini, WindowSection, "Top");
    TOptional<int> const width = ReadSize(ini, WindowSection, "Width", MinWindowWidth);

    // Only all four together place the window.
    if (height.HasValue() && left.HasValue() && top.HasValue() && width.HasValue())
    {
        TGUIWindowBounds window;
        window.Height = height.Value();
        window.Left = left.Value();
        window.Top = top.Value();
        window.Width = width.Value();
        layout.Window = window;
    }

    return layout;
}
//---------------------------------------------------------------------------
/*
    NewGUILayoutFile

    Without an encoding, a TMemIniFile reads a file without a byte order mark, and writes a new one, in the ANSI code
    page, which loses each character it doesn't have. TEncoding::UTF8 is shared, never freed by the TMemIniFile.
*/
TMemIniFile* NewGUILayoutFile(UnicodeString const& path)
{
    return new TMemIniFile(path, TEncoding::UTF8);
}
//---------------------------------------------------------------------------
void SaveGUILayout(TCustomIniFile& ini, TGUILayout const& layout)
{
    WriteOrDelete(ini, PanelsSection, "DetailHeight", layout.DetailHeight);
    WriteOrDelete(ini, PanelsSection, "TestTreeWidth", layout.TestTreeWidth);

    TOptional<int> height;
    TOptional<int> left;
    TOptional<int> top;
    TOptional<int> width;

    if (layout.Window.HasValue())
    {
        height = layout.Window.Value().Height;
        left = layout.Window.Value().Left;
        top = layout.Window.Value().Top;
        width = layout.Window.Value().Width;
    }

    WriteOrDelete(ini, WindowSection, "Height", height);
    WriteOrDelete(ini, WindowSection, "Left", left);
    WriteOrDelete(ini, WindowSection, "Top", top);
    WriteOrDelete(ini, WindowSection, "Width", width);

    ini.WriteBool(WindowSection, "Maximized", layout.Maximized);
}
//---------------------------------------------------------------------------
int ScaleFrom96(int value, int pixelsPerInch)
{
    return RoundToInt(static_cast<double>(value) * pixelsPerInch / 96.0);
}
//---------------------------------------------------------------------------
int ScaleTo96(int value, int pixelsPerInch)
{
    if (pixelsPerInch <= 0)
        return value;

    return RoundToInt(static_cast<double>(value) * 96.0 / pixelsPerInch);
}
//---------------------------------------------------------------------------
TRect ScreenToWorkspace(TRect const& rect, TRect const& primaryMonitor, TRect const& primaryWorkArea)
{
    int const dx = primaryWorkArea.Left - primaryMonitor.Left;
    int const dy = primaryWorkArea.Top - primaryMonitor.Top;

    return TRect(rect.Left - dx, rect.Top - dy, rect.Right - dx, rect.Bottom - dy);
}
//---------------------------------------------------------------------------
TRect WorkspaceToScreen(TRect const& rect, TRect const& primaryMonitor, TRect const& primaryWorkArea)
{
    int const dx = primaryWorkArea.Left - primaryMonitor.Left;
    int const dy = primaryWorkArea.Top - primaryMonitor.Top;

    return TRect(rect.Left + dx, rect.Top + dy, rect.Right + dx, rect.Bottom + dy);
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
