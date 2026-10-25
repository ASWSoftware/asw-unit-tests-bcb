/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_Layout.cpp
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
#include "Test_ASWUnitTestsBCB_GUI_Layout.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <memory>
#include <vector>
//---------------------------------------------------------------------------
#include <System.IniFiles.hpp>
#include <System.SysUtils.hpp>
#include <System.Types.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_Layout.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Inifiles;
using namespace System::Sysutils;
using namespace System::Types;
//---------------------------------------------------------------------------

namespace
{

TMemIniFile* NewMemIniFile();
std::vector<TRect> WorkAreas(TRect const& first);
std::vector<TRect> WorkAreas(TRect const& first, TRect const& second);

//---------------------------------------------------------------------------
/*
    NewMemIniFile

    An empty, in-memory INI file; nothing is read from or written to disk. The caller owns it.
*/
TMemIniFile* NewMemIniFile()
{
    return new TMemIniFile(UnicodeString());
}
//---------------------------------------------------------------------------
std::vector<TRect> WorkAreas(TRect const& first)
{
    return std::vector<TRect>(1, first);
}
//---------------------------------------------------------------------------
std::vector<TRect> WorkAreas(TRect const& first, TRect const& second)
{
    std::vector<TRect> workAreas(1, first);
    workAreas.push_back(second);
    return workAreas;
}
//---------------------------------------------------------------------------

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_Layout
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_GUI_Layout::TTest_ASWUnitTestsBCB_GUI_Layout()
    : inherited("ASWUnitTestsBCB_GUI_Layout_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_GUILayoutFilePath_IsPerExeUnderAppData()
{
    // Arrange
    UnicodeString const exeName = ExtractFileName(ParamStr(0));
    UnicodeString const appData = GetEnvironmentVariable("APPDATA");

    // Act
    UnicodeString const path = GUILayoutFilePath();

    // Assert
    UnicodeString const folder = ExtractFileDir(path);
    CheckEquals(ChangeFileExt(exeName, ".ini"), ExtractFileName(path), ASWBCB_HERE, "named after this executable");
    CheckEquals("ASWUnitTestsBCB", ExtractFileName(folder), ASWBCB_HERE, "in an ASWUnitTestsBCB folder");
    AssertFalse(appData.IsEmpty(), ASWBCB_HERE, "%APPDATA% is set");
    CheckTrue(SameText(ExtractFileDir(folder), ExcludeTrailingPathDelimiter(appData)), ASWBCB_HERE,
        "under %APPDATA%");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_IsWindowReachable_FalseOffScreen()
{
    // Arrange
    std::vector<TRect> const workAreas = WorkAreas(TRect(0, 0, 1920, 1040));

    // Act and Assert
    CheckFalse(IsWindowReachable(TRect(2000, 100, 2800, 700), workAreas), ASWBCB_HERE,
        "a window past a disconnected monitor's edge");
    CheckFalse(IsWindowReachable(TRect(1870, 100, 2670, 700), workAreas), ASWBCB_HERE,
        "only 50 pixels of its title bar on screen");
    CheckFalse(IsWindowReachable(TRect(100, 100, 900, 700), std::vector<TRect>()), ASWBCB_HERE,
        "no work areas at all");
    CheckFalse(IsWindowReachable(TRect(100, 100, 100, 700), workAreas), ASWBCB_HERE, "an empty rectangle");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_IsWindowReachable_FalseWhenTitleBarIsHidden()
{
    // Arrange
    std::vector<TRect> const workAreas = WorkAreas(TRect(0, 0, 1920, 1040));

    // Act and Assert
    CheckFalse(IsWindowReachable(TRect(100, -20, 900, 580), workAreas), ASWBCB_HERE, "its top above the work area");
    CheckFalse(IsWindowReachable(TRect(100, 1020, 900, 1620), workAreas), ASWBCB_HERE,
        "its title bar below the work area's bottom, e.g. behind the taskbar");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_IsWindowReachable_TrueOnAWorkArea()
{
    // Arrange: a second monitor to the left of the primary one, at negative screen coordinates.
    std::vector<TRect> const workAreas = WorkAreas(TRect(0, 0, 1920, 1040), TRect(-1600, 0, 0, 860));

    // Act and Assert
    CheckTrue(IsWindowReachable(TRect(100, 100, 900, 700), workAreas), ASWBCB_HERE, "on the primary monitor");
    CheckTrue(IsWindowReachable(TRect(-1200, 100, -400, 700), workAreas), ASWBCB_HERE, "on the second monitor");
    CheckTrue(IsWindowReachable(TRect(1770, 100, 2570, 700), workAreas), ASWBCB_HERE,
        "150 pixels of its title bar on screen");
    CheckTrue(IsWindowReachable(TRect(100, 800, 900, 1400), workAreas), ASWBCB_HERE,
        "only its title bar and a little more on screen");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_LoadGUILayout_EmptyIniHasNoValues()
{
    // Arrange
    std::unique_ptr<TMemIniFile> const ini(NewMemIniFile());

    // Act
    TGUILayout const layout = LoadGUILayout(*ini);

    // Assert
    CheckFalse(layout.DetailHeight.HasValue(), ASWBCB_HERE, "no detail pane height");
    CheckFalse(layout.Maximized, ASWBCB_HERE, "not maximized");
    CheckFalse(layout.TestTreeWidth.HasValue(), ASWBCB_HERE, "no test tree width");
    CheckFalse(layout.Window.HasValue(), ASWBCB_HERE, "no window bounds");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_LoadGUILayout_IgnoresInvalidValues()
{
    // Arrange
    std::unique_ptr<TMemIniFile> const ini(NewMemIniFile());
    ini->WriteString("Panels", "DetailHeight", "0");
    ini->WriteString("Panels", "TestTreeWidth", "wide");
    ini->WriteString("Window", "Height", "600");
    ini->WriteString("Window", "Left", "10");
    ini->WriteString("Window", "Maximized", "yes");
    ini->WriteString("Window", "Top", "");
    ini->WriteString("Window", "Width", "800");

    // Act
    TGUILayout const layout = LoadGUILayout(*ini);

    // Assert
    CheckFalse(layout.DetailHeight.HasValue(), ASWBCB_HERE, "a zero size is ignored");
    CheckFalse(layout.Maximized, ASWBCB_HERE, "a flag that isn't a number reads as not maximized");
    CheckFalse(layout.TestTreeWidth.HasValue(), ASWBCB_HERE, "a size that isn't a number is ignored");
    CheckFalse(layout.Window.HasValue(), ASWBCB_HERE, "the window isn't placed without all four values");

    // Arrange: every window value valid except a height below the minimum.
    ini->WriteString("Window", "Height", "40");
    ini->WriteString("Window", "Top", "20");

    // Act and Assert
    CheckFalse(LoadGUILayout(*ini).Window.HasValue(), ASWBCB_HERE, "nor with a height too small to use");

    // Arrange: and every window value valid.
    ini->WriteString("Window", "Height", "150");

    // Act and Assert
    CheckTrue(LoadGUILayout(*ini).Window.HasValue(), ASWBCB_HERE, "but with the smallest valid height");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_SaveGUILayout_RemovesMissingValues()
{
    // Arrange
    std::unique_ptr<TMemIniFile> const ini(NewMemIniFile());
    TGUILayout full;
    full.DetailHeight = 170;
    full.TestTreeWidth = 360;
    TGUIWindowBounds window;
    window.Height = 700;
    window.Left = 50;
    window.Top = 60;
    window.Width = 1000;
    full.Window = window;
    SaveGUILayout(*ini, full);
    bool const savedFirst = ini->ValueExists("Panels", "DetailHeight") && ini->ValueExists("Window", "Width");

    // Act
    SaveGUILayout(*ini, TGUILayout());

    // Assert
    CheckTrue(savedFirst, ASWBCB_HERE, "the full layout was written");
    CheckFalse(ini->ValueExists("Panels", "DetailHeight"), ASWBCB_HERE, "the detail pane height is removed");
    CheckFalse(ini->ValueExists("Panels", "TestTreeWidth"), ASWBCB_HERE, "the test tree width is removed");
    CheckFalse(ini->ValueExists("Window", "Height"), ASWBCB_HERE, "and so are the window's height,");
    CheckFalse(ini->ValueExists("Window", "Left"), ASWBCB_HERE, "left,");
    CheckFalse(ini->ValueExists("Window", "Top"), ASWBCB_HERE, "top,");
    CheckFalse(ini->ValueExists("Window", "Width"), ASWBCB_HERE, "and width");
    CheckTrue(ini->ValueExists("Window", "Maximized"), ASWBCB_HERE, "the maximized flag is always written");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_SaveGUILayout_RoundTrips()
{
    // Arrange: a window on a monitor left of the primary one, so its position is negative.
    std::unique_ptr<TMemIniFile> const ini(NewMemIniFile());
    TGUILayout saved;
    saved.DetailHeight = 170;
    saved.Maximized = true;
    saved.TestTreeWidth = 360;
    TGUIWindowBounds window;
    window.Height = 700;
    window.Left = -1500;
    window.Top = 60;
    window.Width = 1024;
    saved.Window = window;

    // Act
    SaveGUILayout(*ini, saved);
    TGUILayout const loaded = LoadGUILayout(*ini);

    // Assert
    AssertTrue(loaded.DetailHeight.HasValue(), ASWBCB_HERE, "the detail pane height is kept");
    CheckEquals(170, loaded.DetailHeight.Value(), ASWBCB_HERE, "unchanged");
    CheckTrue(loaded.Maximized, ASWBCB_HERE, "the maximized flag is kept");
    AssertTrue(loaded.TestTreeWidth.HasValue(), ASWBCB_HERE, "the test tree width is kept");
    CheckEquals(360, loaded.TestTreeWidth.Value(), ASWBCB_HERE, "unchanged");
    AssertTrue(loaded.Window.HasValue(), ASWBCB_HERE, "the window bounds are kept");
    CheckEquals(700, loaded.Window.Value().Height, ASWBCB_HERE, "height");
    CheckEquals(-1500, loaded.Window.Value().Left, ASWBCB_HERE, "left");
    CheckEquals(60, loaded.Window.Value().Top, ASWBCB_HERE, "top");
    CheckEquals(1024, loaded.Window.Value().Width, ASWBCB_HERE, "width");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_ScaleFrom96_ScalesAndRounds()
{
    // Act and Assert
    CheckEquals(100, ScaleFrom96(100, 96), ASWBCB_HERE, "100% scaling");
    CheckEquals(125, ScaleFrom96(100, 120), ASWBCB_HERE, "125% scaling");
    CheckEquals(150, ScaleFrom96(100, 144), ASWBCB_HERE, "150% scaling");
    CheckEquals(200, ScaleFrom96(100, 192), ASWBCB_HERE, "200% scaling");
    CheckEquals(152, ScaleFrom96(101, 144), ASWBCB_HERE, "151.5 rounds to the nearest pixel, half away from zero");
    CheckEquals(-152, ScaleFrom96(-101, 144), ASWBCB_HERE, "-151.5 too");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_ScaleTo96_ReversesScaleFrom96()
{
    // Act and Assert
    CheckEquals(100, ScaleTo96(125, 120), ASWBCB_HERE, "from 125% scaling");
    CheckEquals(100, ScaleTo96(150, 144), ASWBCB_HERE, "from 150% scaling");

    int const values[] = { 60, 150, 333, 1008 };

    for (std::size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
    {
        CheckEquals(values[i], ScaleTo96(ScaleFrom96(values[i], 144), 144), ASWBCB_HERE,
            "a size survives being scaled to 150% and back: " + IntToString(values[i]));
    }

    CheckEquals(360, ScaleTo96(360, 0), ASWBCB_HERE, "an unknown DPI leaves the size as it is");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_ScreenToWorkspace_ReversesWorkspaceToScreen()
{
    // Arrange: a taskbar docked at the left of the primary monitor pushes its work area right 60 pixels.
    TRect const primaryMonitor(0, 0, 1920, 1080);
    TRect const leftTaskbarWorkArea(60, 0, 1920, 1080);
    TRect const screenRect(150, 90, 1350, 910);

    // Act
    TRect const workspaceRect = ScreenToWorkspace(screenRect, primaryMonitor, leftTaskbarWorkArea);
    TRect const roundTrip = WorkspaceToScreen(workspaceRect, primaryMonitor, leftTaskbarWorkArea);

    // Assert
    CheckEquals(90, workspaceRect.Left, ASWBCB_HERE, "left moves by the work area's offset");
    CheckEquals(90, workspaceRect.Top, ASWBCB_HERE, "top is unchanged");
    CheckEquals(1290, workspaceRect.Right, ASWBCB_HERE, "right moves by the work area's offset");
    CheckEquals(910, workspaceRect.Bottom, ASWBCB_HERE, "bottom is unchanged");
    CheckEquals(150, roundTrip.Left, ASWBCB_HERE, "converting back restores left");
    CheckEquals(90, roundTrip.Top, ASWBCB_HERE, "top");
    CheckEquals(1350, roundTrip.Right, ASWBCB_HERE, "right");
    CheckEquals(910, roundTrip.Bottom, ASWBCB_HERE, "and bottom");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Layout::Test_WorkspaceToScreen_OffsetsByPrimaryWorkArea()
{
    // Arrange: a taskbar docked at the top of the primary monitor pushes its work area down 40 pixels.
    TRect const primaryMonitor(0, 0, 1920, 1080);
    TRect const topTaskbarWorkArea(0, 40, 1920, 1080);

    // Act
    TRect const shifted = WorkspaceToScreen(TRect(10, 10, 810, 610), primaryMonitor, topTaskbarWorkArea);
    TRect const unshifted = WorkspaceToScreen(TRect(10, 10, 810, 610), primaryMonitor, TRect(0, 0, 1920, 1040));

    // Assert
    CheckEquals(10, shifted.Left, ASWBCB_HERE, "left is unchanged");
    CheckEquals(50, shifted.Top, ASWBCB_HERE, "top moves down with the work area");
    CheckEquals(810, shifted.Right, ASWBCB_HERE, "right is unchanged");
    CheckEquals(650, shifted.Bottom, ASWBCB_HERE, "bottom moves down with the work area");
    CheckEquals(10, unshifted.Top, ASWBCB_HERE, "with the taskbar at the bottom, nothing moves");
    CheckEquals(610, unshifted.Bottom, ASWBCB_HERE, "bottom neither");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_GUI_Layout, "SelfTests.GUI")
