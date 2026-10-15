/* **************************************************************************
ASWUnitTestsBCB_GUI_MainForm.cpp
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
#include "ASWUnitTestsBCB_GUI_MainForm.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
#include <algorithm>
#include <exception>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <System.Diagnostics.hpp>
#include <System.IniFiles.hpp>
#include <System.IOUtils.hpp>
#include <System.SysUtils.hpp>
#include <System.Types.hpp>
#include <System.UITypes.hpp>
#include <Vcl.Clipbrd.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Graphics.hpp>
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_CLI.h"
#include "ASWUnitTestsBCB_Exception.h"
#include "ASWUnitTestsBCB_GUI_Selection.h"
#include "ASWUnitTestsBCB_GUI_ShuffleSeedDialog.h"
#include "ASWUnitTestsBCB_GUI_TextDialog.h"
#include "ASWUnitTestsBCB_JUnitReport.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace ASWUnitTestsBCB;
using namespace System;
using namespace System::Classes;
using namespace System::Diagnostics;
using namespace System::Inifiles;
using namespace System::Ioutils;
using namespace System::Sysutils;
using namespace System::Types;
using namespace System::Uitypes;
//---------------------------------------------------------------------------

namespace
{

TColor const FailureColor = static_cast<TColor>(RGB(200, 0, 0));
TColor const SkipColor = static_cast<TColor>(RGB(190, 120, 0));

/////////////////////////////////////////////////////////////////////////////
// TNodeKind
//
// What a test tree node stands for. A node's Data holds its kind and an
// index (see NodeData()): a suite node's into TGUITestList::SuitePaths(), a
// group node's into its GroupNames(), and a test node's into the list
// itself.
/////////////////////////////////////////////////////////////////////////////
enum class TNodeKind
{
    Suite,
    Group,
    Test,
};

TTreeNode* AddTreeNode(TTreeNodes* nodes, TTreeNode* parent, std::string const& text, void* data);
std::size_t DataToIndex(void* data);
std::string FirstLine(std::string const& text);
std::string FormatSeconds(double seconds);
void* IndexToData(std::size_t index);
TColor LogLineColor(std::string const& line);
std::vector<TRect> MonitorWorkAreas();
void* NodeData(TNodeKind kind, std::size_t index);
std::size_t NodeIndex(void* data);
TNodeKind NodeKind(void* data);
TTreeNode* SuiteTreeNode(TTreeNodes* nodes, TGUITestList const& testList, std::string const& suitePath,
    std::vector<TTreeNode*>& suiteNodes);
TNodeCheckState ToNodeCheckState(TGUICheckState state);

//---------------------------------------------------------------------------
/*
    AddTreeNode

    Adds a node as the last child of 'parent', or as the last top-level node when 'parent' is nullptr.
*/
TTreeNode* AddTreeNode(TTreeNodes* nodes, TTreeNode* parent, std::string const& text, void* data)
{
    if (parent == nullptr)
        return nodes->AddObject(nullptr, FromUTF8(text), data);

    return nodes->AddChildObject(parent, FromUTF8(text), data);
}
//---------------------------------------------------------------------------
/*
    DataToIndex

    A Failures & Skips list item's Data holds its test's index into TGUITestList.
*/
std::size_t DataToIndex(void* data)
{
    return static_cast<std::size_t>(reinterpret_cast<NativeUInt>(data));
}
//---------------------------------------------------------------------------
std::string FirstLine(std::string const& text)
{
    return text.substr(0, text.find('\n'));
}
//---------------------------------------------------------------------------
std::string FormatSeconds(double seconds)
{
    std::ostringstream text;
    text << std::fixed << std::setprecision(2) << seconds << " s";
    return text.str();
}
//---------------------------------------------------------------------------
void* IndexToData(std::size_t index)
{
    return reinterpret_cast<void*>(static_cast<NativeUInt>(index));
}
//---------------------------------------------------------------------------
/*
    LogLineColor

    The Log tab's color for one line of the framework's log output: its failure lines (a failed test or check, an
    unhandled exception, a fatal SetUp_Test()/TearDown_Test() error, a group whose constructor or SetUp_Group() threw,
    or a test that timed out or crashed) and its skip lines stand out.
*/
TColor LogLineColor(std::string const& line)
{
    if ((line.find("***Test failed") != std::string::npos) || (line.find("**Check failed") != std::string::npos) ||
        (line.find("\" - failed (") != std::string::npos) || (line.find("Unhandled exception") != std::string::npos) ||
        (line.find("!!FATAL ERROR!!") != std::string::npos) || (line.find("!!ERROR!!") != std::string::npos) ||
        (line.find("!!TIMEOUT!!") != std::string::npos) || (line.find("!!CRASH!!") != std::string::npos))
    {
        return FailureColor;
    }

    if ((line.find("***Test skipped") != std::string::npos) || (line.find("\" - skipped (") != std::string::npos))
        return SkipColor;

    return clWindowText;
}
//---------------------------------------------------------------------------
/*
    MonitorWorkAreas

    Every monitor's work area (the part the taskbar and other docked bars leave free), in screen pixels.
*/
std::vector<TRect> MonitorWorkAreas()
{
    std::vector<TRect> workAreas;

    int const monitorCount = Screen->MonitorCount;

    for (int i = 0; i < monitorCount; ++i)
    {
        TRect const workArea = Screen->Monitors[i]->WorkareaRect;
        workAreas.push_back(workArea);
    }

    return workAreas;
}
//---------------------------------------------------------------------------
/*
    NodeData

    A test tree node's Data, from its kind and index (see TNodeKind). The kind takes the low two bits.
*/
void* NodeData(TNodeKind kind, std::size_t index)
{
    return reinterpret_cast<void*>(static_cast<NativeUInt>(index) * 4 + static_cast<NativeUInt>(kind));
}
//---------------------------------------------------------------------------
std::size_t NodeIndex(void* data)
{
    return static_cast<std::size_t>(reinterpret_cast<NativeUInt>(data) / 4);
}
//---------------------------------------------------------------------------
TNodeKind NodeKind(void* data)
{
    return static_cast<TNodeKind>(reinterpret_cast<NativeUInt>(data) % 4);
}
//---------------------------------------------------------------------------
/*
    SuiteTreeNode

    The tree node of suite 'suitePath', by index into testList.SuitePaths() in 'suiteNodes': added the first time it's
    asked for, after the nodes of the suites holding it; nullptr for an empty path (the top level).
*/
TTreeNode* SuiteTreeNode(TTreeNodes* nodes, TGUITestList const& testList, std::string const& suitePath,
    std::vector<TTreeNode*>& suiteNodes)
{
    if (suitePath.empty())
        return nullptr;

    std::size_t const suiteIndex = testList.SuiteIndexOf(suitePath).Value();

    if (suiteNodes[suiteIndex] == nullptr)
    {
        std::string const parentPath = ParentSuitePath(suitePath);
        TTreeNode* const parentNode = SuiteTreeNode(nodes, testList, parentPath, suiteNodes);
        suiteNodes[suiteIndex] = AddTreeNode(nodes, parentNode, NameInSuite(suitePath, parentPath),
            NodeData(TNodeKind::Suite, suiteIndex));
    }

    return suiteNodes[suiteIndex];
}
//---------------------------------------------------------------------------
TNodeCheckState ToNodeCheckState(TGUICheckState state)
{
    switch (state)
    {
        case TGUICheckState::Checked:
            return ncsChecked;

        case TGUICheckState::Partial:
            return ncsPartial;

        case TGUICheckState::Unchecked:
            return ncsUnchecked;
    }

    return ncsUnchecked;
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// TScopedFlag
//
// Sets a bool for the lifetime of this object, clearing it again even if
// an exception leaves the scope.
/////////////////////////////////////////////////////////////////////////////
class TScopedFlag
{
private:
    bool& m_Flag;

    TScopedFlag(TScopedFlag const&);            // Not copyable
    TScopedFlag& operator=(TScopedFlag const&); // Not copyable

public:
    explicit TScopedFlag(bool& flag)
        : m_Flag(flag)
    {
        m_Flag = true;
    }

    ~TScopedFlag()
    {
        m_Flag = false;
    }
};


/////////////////////////////////////////////////////////////////////////////
// TTreeUpdateScope
//
// Brackets a batch of tree node changes with BeginUpdate()/EndUpdate(), so
// the tree repaints once afterward, and EndUpdate() still runs if an
// exception leaves the scope.
/////////////////////////////////////////////////////////////////////////////
class TTreeUpdateScope
{
private:
    TTreeNodes* const m_Nodes;

    TTreeUpdateScope(TTreeUpdateScope const&);            // Not copyable
    TTreeUpdateScope& operator=(TTreeUpdateScope const&); // Not copyable

public:
    explicit TTreeUpdateScope(TTreeNodes* nodes)
        : m_Nodes(nodes)
    {
        m_Nodes->BeginUpdate();
    }

    ~TTreeUpdateScope()
    {
        m_Nodes->EndUpdate();
    }
};

} // namespace

//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// TFormASWUnitTestsBCBGUIMain
/////////////////////////////////////////////////////////////////////////////

TFormASWUnitTestsBCBGUIMain* FormASWUnitTestsBCBGUIMain;

//---------------------------------------------------------------------------
__fastcall TFormASWUnitTestsBCBGUIMain::TFormASWUnitTestsBCBGUIMain(TComponent* Owner)
    : inherited(Owner),
      m_CheckClickNode(nullptr),
      m_CheckClickOldState(ncsNone),
      m_CloseAfterRun(false),
      m_KeepSelection(false),
      m_LastRunExitCode(ExitCode_Success),
      m_RunAborted(false),
      m_RunFinishedCount(0),
      m_Running(false),
      m_RunStarted(false),
      m_RunTotalCount(0),
      m_StatusMessage("Ready."),
      m_SyncingChecks(false),
      m_TreeNodesStale(false)
{
    m_TreeViewWindowProc = TV_Tests->WindowProc;
    TV_Tests->WindowProc = TV_TestsWindowProc;
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::~TFormASWUnitTestsBCBGUIMain

    This form's own members (m_TestList, m_TestNodes, and so on) are destroyed before the VCL base class destroys its
    controls, and destroying a control can fire its events. So every event that reads those members is detached
    first, along with anything still queued for this form, so none can run against members that no longer exist. The
    tree's own WindowProc is restored for the same reason.
*/
__fastcall TFormASWUnitTestsBCBGUIMain::~TFormASWUnitTestsBCBGUIMain()
{
    TV_Tests->WindowProc = m_TreeViewWindowProc;

    Edt_Filter->OnChange = nullptr;
    LV_Failures->OnCustomDrawItem = nullptr;
    LV_Failures->OnDblClick = nullptr;
    TV_Tests->OnChange = nullptr;
    TV_Tests->OnCheckStateChanging = nullptr;

    TThread::RemoveQueuedEvents(static_cast<TThread*>(nullptr)); // Everything queued with ForceQueue(nullptr, ...)
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_AboutExecute(TObject* /*Sender*/)
{
    RunGuarded("About", &this->DoAbout);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_CollapseAllExecute(TObject* /*Sender*/)
{
    RunGuarded("Collapse All", &this->DoCollapseAll);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_CommandLineOptionsExecute(TObject* /*Sender*/)
{
    RunGuarded("Command Line Options", &this->DoCommandLineOptions);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_CopyDetailsExecute(TObject* /*Sender*/)
{
    RunGuarded("Copy Details", &this->DoCopyDetails);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_ExitExecute(TObject* /*Sender*/)
{
    // During a run, FormCloseQuery() stops it after the current test, then closes.
    RunGuarded("Exit", &this->DoCloseWindow);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_ExpandAllExecute(TObject* /*Sender*/)
{
    RunGuarded("Expand All", &this->DoExpandAll);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_ExportJUnitReportExecute(TObject* /*Sender*/)
{
    RunGuarded("Export JUnit Report", &this->DoExportJUnitReport);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_FocusFilterExecute(TObject* /*Sender*/)
{
    RunGuarded("Filter", &this->DoFocusFilter);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_ResetLayoutExecute(TObject* /*Sender*/)
{
    RunGuarded("Reset Layout", &this->DoResetLayout);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_RunFailedExecute(TObject* /*Sender*/)
{
    RunGuarded("Run Failed", &this->DoRunFailed);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_RunSelectedExecute(TObject* /*Sender*/)
{
    RunGuarded("Run Selected", &this->DoRunSelected);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_SelectAllExecute(TObject* /*Sender*/)
{
    RunGuarded("Select All", &this->DoSelectAll);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_SelectFailedExecute(TObject* /*Sender*/)
{
    RunGuarded("Select Failed", &this->DoSelectFailed);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_SelectNoneExecute(TObject* /*Sender*/)
{
    RunGuarded("Select None", &this->DoSelectNone);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_ShuffleOrderExecute(TObject* /*Sender*/)
{
    RunGuarded("Run in Shuffled Order", &this->DoShuffleOrder);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_ShuffleSeedExecute(TObject* /*Sender*/)
{
    RunGuarded("Shuffle Seed", &this->DoShuffleSeed);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Act_StopExecute(TObject* /*Sender*/)
{
    RunGuarded("Stop", &this->DoStop);
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::AddFailureItem(std::size_t testIndex)
{
    TTestId const& test = m_TestList.Test(testIndex);
    TGUITestResult const* const result = m_TestList.Result(testIndex);

    TListItem* const item = LV_Failures->Items->Add();
    item->Caption = FromUTF8(StatusName(m_TestList.Status(testIndex)));
    item->SubItems->Add(FromUTF8(test.GroupName + "." + test.TestName));
    item->SubItems->Add(FromUTF8((result != nullptr) ? FirstLine(result->Record.Message) : std::string()));
    item->Data = IndexToData(testIndex);
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::AppendLog(std::string const& text)
{
    // One line at a time, so each can have its own color. A trailing piece with no line break (from LogAppend()) is
    // appended as-is, and the next piece continues its line.
    std::size_t start = 0;

    while (start < text.size())
    {
        std::size_t const end = text.find('\n', start);
        bool const endsLine = (end != std::string::npos);
        std::string const line = text.substr(start, endsLine ? end - start : std::string::npos);

        // Past the end is clamped to the end, which sidesteps the rich edit counting a line break as one character
        // here but two in GetTextLen().
        RE_Log->SelStart = RE_Log->GetTextLen();
        RE_Log->SelLength = 0;
        RE_Log->SelAttributes->Color = LogLineColor(line);
        RE_Log->SelText = FromUTF8(line) + (endsLine ? UnicodeString("\r\n") : UnicodeString());

        start = endsLine ? end + 1 : text.size();
    }

    ::SendMessage(RE_Log->Handle, WM_VSCROLL, SB_BOTTOM, 0);
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::ApplyFilterBox

    The text matches anywhere in a test's "Group.Test" full name, ignoring case, and may use the same '*' and '?'
    wildcards as --filter. Unlike --filter, it's never anchored to the whole name, so "Parse?rg" finds
    "...ParseArguments...", just as plain text would.
*/
void __fastcall TFormASWUnitTestsBCBGUIMain::ApplyFilterBox()
{
    std::string const text = ToUTF8(Edt_Filter->Text.Trim());

    if (text.empty())
    {
        m_TestList.SetVisibleFilter(nullptr);
    }
    else
    {
        TPatternTestFilter const filter("*" + text + "*", true);
        m_TestList.SetVisibleFilter(&filter);
    }

    BuildTestTree();
    UpdateDetail();
    UpdateStatusBar();
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::ApplyPanelSizes(TGUILayout const& layout)
{
    // Kept within what the splitters allow, so each can still be dragged and nothing is squeezed out of view.
    // (Properties are read into locals first throughout this file: bcc32 mistypes a property read nested in a larger
    // expression, such as an argument to a reference parameter.)
    int const pixelsPerInch = CurrentPPI;

    if (layout.TestTreeWidth.HasValue())
    {
        int const minimum = Spl_Tests->MinSize;
        int const maximum = ClientWidth - Spl_Tests->Width - minimum;
        int const width = ScaleFrom96(layout.TestTreeWidth.Value(), pixelsPerInch);
        TV_Tests->Width = std::max(minimum, std::min(width, maximum));
    }

    if (layout.DetailHeight.HasValue())
    {
        int const minimum = Spl_Detail->MinSize;
        int const maximum = Pnl_Results->ClientHeight - Spl_Detail->Height - minimum;
        int const height = ScaleFrom96(layout.DetailHeight.Value(), pixelsPerInch);
        Mem_Detail->Height = std::max(minimum, std::min(height, maximum));
    }
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::ApplyQueuedLayout()
{
    ApplyPanelSizes(m_QueuedLayout);
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::BuildTestTree

    Rebuilds the tree from m_TestList: one node per group, in order, each with one child node per test, and one node
    per suite, holding the nodes of the groups and suites in it. A group's and a suite's node show just its own name.
*/
void TFormASWUnitTestsBCBGUIMain::BuildTestTree()
{
    {
        TTreeNodes* const nodes = TV_Tests->Items;
        TTreeUpdateScope const updating(nodes);

        nodes->Clear();
        m_TestNodes.assign(m_TestList.Count(), static_cast<TTreeNode*>(nullptr));

        // By index into m_TestList.SuitePaths(). A suite's node, and those of the suites holding it, are added just
        // before its first group's (see SuiteTreeNode()), so suites and groups keep m_TestList's order, where each
        // suite's groups are together.
        std::vector<TTreeNode*> suiteNodes(m_TestList.SuitePaths().size(), static_cast<TTreeNode*>(nullptr));

        // Only visible tests get nodes, and a group or suite only gets one if any of its tests are visible.
        std::vector<std::string> const& groupNames = m_TestList.GroupNames();

        for (std::size_t groupIndex = 0; groupIndex < groupNames.size(); ++groupIndex)
        {
            std::vector<std::size_t> const testIndexes = m_TestList.TestsInGroup(groupNames[groupIndex]);
            if (testIndexes.empty())
                continue;

            std::string const suitePath = m_TestList.GroupSuitePath(groupNames[groupIndex]);
            TTreeNode* const suiteNode = SuiteTreeNode(nodes, m_TestList, suitePath, suiteNodes);
            TTreeNode* const groupNode = AddTreeNode(nodes, suiteNode, NameInSuite(groupNames[groupIndex], suitePath),
                NodeData(TNodeKind::Group, groupIndex));

            for (std::size_t i = 0; i < testIndexes.size(); ++i)
            {
                std::size_t const testIndex = testIndexes[i];
                m_TestNodes[testIndex] = AddTreeNode(nodes, groupNode, m_TestList.Test(testIndex).TestName,
                    NodeData(TNodeKind::Test, testIndex));
            }
        }

        // The nodes just added are the tree's current ones, even if its window was recreated before this.
        m_TreeNodesStale = false;

        TV_Tests->FullExpand();
    }

    RefreshAllNodeImages();
    SyncTreeChecks();

    // e.g. while the filter hides every test.
    Act_CollapseAll->Enabled = (TV_Tests->Items->Count > 0);
    Act_ExpandAll->Enabled = (TV_Tests->Items->Count > 0);

    if (TV_Tests->Items->Count > 0)
        TV_Tests->Items->GetFirstNode()->MakeVisible();
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::CenterOnMonitor

    Sizes the window to 'width' x 'height' pixels and centers it on its monitor's work area.
*/
void TFormASWUnitTestsBCBGUIMain::CenterOnMonitor(int width, int height)
{
    TRect const workArea = Monitor->WorkareaRect;
    SetBounds(std::max(workArea.Left, workArea.Left + (workArea.Width() - width) / 2),
        std::max(workArea.Top, workArea.Top + (workArea.Height() - height) / 2), width, height);
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::CreateStatusImages

    Draws one colored dot per TGUITestStatus into IL_Status, in the enum's order, so a status converts directly to
    its image index. Drawn at runtime, scaled to the form's DPI, rather than stored as images.
*/
void TFormASWUnitTestsBCBGUIMain::CreateStatusImages()
{
    // In TGUITestStatus order: NotRun, Running, Passed, Failed, Skipped.
    TColor const colors[] =
    {
        static_cast<TColor>(RGB(190, 190, 190)),
        static_cast<TColor>(RGB(0, 120, 215)),
        static_cast<TColor>(RGB(16, 160, 16)),
        static_cast<TColor>(RGB(215, 40, 40)),
        static_cast<TColor>(RGB(230, 165, 0))
    };

    int const size = ScaleValue(16);
    int const inset = ScaleValue(3);

    IL_Status->Clear();
    IL_Status->SetSize(size, size);

    for (std::size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i)
    {
        std::unique_ptr<Vcl::Graphics::TBitmap> const bitmap(new Vcl::Graphics::TBitmap());
        bitmap->SetSize(size, size);
        bitmap->Canvas->Brush->Color = clFuchsia; // Transparent, via AddMasked() below
        bitmap->Canvas->FillRect(TRect(0, 0, size, size));
        bitmap->Canvas->Brush->Color = colors[i];
        bitmap->Canvas->Pen->Color = colors[i];
        bitmap->Canvas->Ellipse(inset, inset, size - inset, size - inset);
        IL_Status->AddMasked(bitmap.get(), clFuchsia);
    }
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::CurrentLayout

    The window's restored bounds come from GetWindowPlacement(), so a maximized (or minimized) window still saves the
    size it restores to.
*/
TGUILayout TFormASWUnitTestsBCBGUIMain::CurrentLayout()
{
    TGUILayout layout;
    int const pixelsPerInch = CurrentPPI;

    layout.DetailHeight = ScaleTo96(Mem_Detail->Height, pixelsPerInch);
    layout.TestTreeWidth = ScaleTo96(TV_Tests->Width, pixelsPerInch);

    WINDOWPLACEMENT placement;
    ::ZeroMemory(&placement, sizeof(placement));
    placement.length = sizeof(placement);

    if (::GetWindowPlacement(Handle, &placement))
    {
        TRect const primaryMonitor = Screen->PrimaryMonitor->BoundsRect;
        TRect const primaryWorkArea = Screen->PrimaryMonitor->WorkareaRect;
        TRect const bounds = WorkspaceToScreen(TRect(placement.rcNormalPosition), primaryMonitor, primaryWorkArea);

        TGUIWindowBounds window;
        window.Height = ScaleTo96(bounds.Height(), pixelsPerInch);
        window.Left = bounds.Left;
        window.Top = bounds.Top;
        window.Width = ScaleTo96(bounds.Width(), pixelsPerInch);
        layout.Window = window;

        layout.Maximized = (placement.showCmd == SW_SHOWMAXIMIZED) ||
            ((placement.showCmd == SW_SHOWMINIMIZED) && ((placement.flags & WPF_RESTORETOMAXIMIZED) != 0));
    }

    return layout;
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoAbout()
{
    ShowTextDialog(UnicodeString("About ") + Application->Title,
        TTestHandler::GetVersionFullStr() + "\n"
        "VCL GUI runner\n"
        "\n"
        "App bits: "
#if defined(_WIN64)
        "64"
#else
        "32"
#endif
        "\n\n"
        "https://github.com/ASWSoftware/asw-unit-tests-bcb\n"
        "\n"
        "Copyright 2026 ASW Software\n"
        "Licensed under the Apache License, Version 2.0.\n");
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::DoCheckClick

    The body of TV_TestsCheckStateChanging(), for m_CheckClickNode.
*/
void __fastcall TFormASWUnitTestsBCBGUIMain::DoCheckClick()
{
    // The run in progress already decided which tests it runs, so a change now would only mislead.
    if (m_Running || (m_CheckClickNode == nullptr))
        return;

    bool const check = (m_CheckClickOldState != ncsChecked);
    void* const data = m_CheckClickNode->Data;
    std::size_t const index = NodeIndex(data);

    switch (NodeKind(data))
    {
        case TNodeKind::Suite:
            m_TestList.SetSuiteChecked(m_TestList.SuitePaths().at(index), check);
            break;

        case TNodeKind::Group:
            m_TestList.SetGroupChecked(m_TestList.GroupNames().at(index), check);
            break;

        case TNodeKind::Test:
            m_TestList.SetChecked(index, check);
            break;
    }

    // Changing check boxes from inside the tree's own change notification isn't safe, so the update waits until this
    // click has been fully handled.
    TThread::ForceQueue(nullptr, &this->QueuedSyncTreeChecks);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoCloseWindow()
{
    Close();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoCollapseAll()
{
    {
        TTreeUpdateScope const updating(TV_Tests->Items);
        TV_Tests->FullCollapse();
    }

    // A selected node that's now hidden passes the selection to the top-level node above it, which the detail pane
    // was found not to follow on its own.
    UpdateDetail();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoCommandLineOptions()
{
    UnicodeString const caption = Application->Title + UnicodeString(" - Command Line Options");
    ShowTextDialog(caption, TGUICommandLine::HelpText());
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoCopyDetails()
{
    UnicodeString const details = Mem_Detail->Text;
    Clipboard()->AsText = details;
    m_StatusMessage = "Details copied to the clipboard.";
    UpdateStatusBar();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoExpandAll()
{
    {
        TTreeUpdateScope const updating(TV_Tests->Items);
        TV_Tests->FullExpand();
    }

    if (TV_Tests->Selected != nullptr)
        TV_Tests->Selected->MakeVisible();
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::DoExportJUnitReport

    Saves the latest run's JUnit report, the same one --report-junit writes, e.g. when --report-junit wasn't given. The
    dialog starts at --report-junit's path if there is one, and then at the last one chosen.
*/
void __fastcall TFormASWUnitTestsBCBGUIMain::DoExportJUnitReport()
{
    if (m_Running || !m_LastRunJUnitTestCases.HasValue())
        return;

    UnicodeString const lastPath = SD_JUnitReport->FileName;

    if (lastPath.IsEmpty() && !m_Options.CLI.JUnitReportPath.empty())
    {
        SD_JUnitReport->FileName = ExpandFileName(FromUTF8(m_Options.CLI.JUnitReportPath));
    }
    else if (lastPath.IsEmpty())
    {
        // e.g. "ASWUnitTestsBCB_GUI_JUnit.xml", in the dialog's default folder.
        SD_JUnitReport->FileName = ChangeFileExt(ExtractFileName(ParamStr(0)), "_JUnit.xml");
    }

    if (!SD_JUnitReport->Execute(Handle))
        return;

    UnicodeString const path = SD_JUnitReport->FileName;

    if (!WriteJUnitReport(path))
    {
        UnicodeString const message = FromUTF8("The JUnit report couldn't be written to:\n" + ToUTF8(path));
        UnicodeString const title = Application->Title;
        Application->MessageBox(message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    m_StatusMessage = "JUnit report exported.";
    UpdateStatusBar();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoFocusFilter()
{
    if (!Edt_Filter->CanFocus())
        return;

    Edt_Filter->SetFocus();
    Edt_Filter->SelectAll();
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::DoRequestCloseAfterRun

    A run can't be interrupted partway through a test, so closing during one asks it to stop after the current test,
    and closes the window once it has (see RunTests()).
*/
void __fastcall TFormASWUnitTestsBCBGUIMain::DoRequestCloseAfterRun()
{
    m_CloseAfterRun = true;
    m_Observer.RequestStop();
    m_StatusMessage = "Closing after the current test...";
    UpdateStatusBar();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoResetLayout()
{
    WindowState = wsNormal;

    // The default size, centered on the monitor the window is on now.
    if (m_DefaultLayout.Window.HasValue())
    {
        int const pixelsPerInch = CurrentPPI;
        CenterOnMonitor(ScaleFrom96(m_DefaultLayout.Window.Value().Width, pixelsPerInch),
            ScaleFrom96(m_DefaultLayout.Window.Value().Height, pixelsPerInch));
    }

    ApplyPanelSizes(m_DefaultLayout);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoRunFailed()
{
    if (m_TestList.StatusCount(TGUITestStatus::Failed) == 0)
        return;

    // Built before RunTests() resets every result, while the failures are still known.
    TGUITestNamesFilter const filter = m_TestList.FailedFilter();
    RunTests(&filter, "the previous run's failures");
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoRunOnStart()
{
    // Unlike Run Selected, runs even if nothing is checked, so --exit still reports and exits the way the console
    // runner does when its filter matches nothing.
    RunCheckedTests();

    if (m_Options.ExitAfterRun)
        Close();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoRunSelected()
{
    if (m_TestList.CheckedCount() == 0)
    {
        m_StatusMessage = "No tests are checked.";
        UpdateStatusBar();
        return;
    }

    RunCheckedTests();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoSaveOnClose()
{
    // An --exit run is automated, so nobody arranged its window.
    if (!m_Options.ExitAfterRun && !m_Options.LayoutIgnore)
        SaveLayout();

    if (m_KeepSelection)
        SaveSelection();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoSelectAll()
{
    m_TestList.SetAllChecked(true);
    SyncTreeChecks();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoSelectFailed()
{
    m_TestList.CheckOnlyFailed();
    SyncTreeChecks();
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::DoSelectFailure

    Selects the double-clicked failure's test in the tree, which shows its details.
*/
void __fastcall TFormASWUnitTestsBCBGUIMain::DoSelectFailure()
{
    TListItem* const item = LV_Failures->Selected;
    if (item == nullptr)
        return;

    std::size_t const testIndex = DataToIndex(item->Data);
    if (testIndex >= m_TestList.Count())
        return;

    // A failure the filter box is hiding can't be selected, so the filter is cleared to show it.
    if (TestNode(testIndex) == nullptr)
        Edt_Filter->Text = UnicodeString(); // Its OnChange rebuilds the tree.

    TTreeNode* const node = TestNode(testIndex);
    if (node == nullptr)
        return;

    TV_Tests->Selected = node;
    node->MakeVisible();
    TV_Tests->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoSelectNone()
{
    m_TestList.SetAllChecked(false);
    SyncTreeChecks();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoShuffleOrder()
{
    m_Shuffle.SetEnabled(!m_Shuffle.Enabled());
    ShowShuffleState();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoShuffleSeed()
{
    if (ShowShuffleSeedDialog(m_Shuffle))
        ShowShuffleState();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::DoStop()
{
    m_Observer.RequestStop();
    m_StatusMessage = "Stopping after the current test...";
    UpdateStatusBar();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::Edt_FilterChange(TObject* /*Sender*/)
{
    RunGuarded("the filter box", &this->ApplyFilterBox);
}
//---------------------------------------------------------------------------
int TFormASWUnitTestsBCBGUIMain::ExitCode() const
{
    return m_Options.ExitAfterRun ? m_LastRunExitCode : ExitCode_Success;
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::FailRunOnUnhandledException(std::string const& description)
{
    m_Observer.Flush();

    // The test that was running when the exception escaped never finished, so it's the one that failed.
    if (m_CurrentTestIndex.HasValue())
    {
        std::size_t const index = m_CurrentTestIndex.Value();
        TTestId const& test = m_TestList.Test(index);

        TTestCaseRecord record;
        record.GroupName = test.GroupName;
        record.TestName = test.TestName;
        record.DurationSeconds = 0.0;
        record.Outcome = TTestOutcome::Fail;
        record.Message = "Unhandled exception: " + description;
        record.SuitePath = test.SuitePath;

        m_TestList.SetResult(index, record, std::string());
        UpdateTestNodeImages(index);
        AddFailureItem(index);
        m_CurrentTestIndex = TOptional<std::size_t>();
    }

    PB_Progress->State = pbsError;
    m_StatusMessage = "The run was ended by an unhandled exception: " + description;
    AppendLog(m_StatusMessage + "\n");
    UpdateDetail();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::FormClose(TObject* /*Sender*/, TCloseAction& /*Action*/)
{
    RunGuarded("closing the window", &this->DoSaveOnClose);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::FormCloseQuery(TObject* /*Sender*/, bool& CanClose)
{
    if (!m_Running)
        return;

    CanClose = false;
    RunGuarded("closing the window", &this->DoRequestCloseAfterRun);
}

//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::LoadLayout

    Called from Start(), before the window is first shown. The panel sizes are applied once it's showing, since a
    window about to open maximized doesn't have the size they have to fit into until then.
*/
void TFormASWUnitTestsBCBGUIMain::LoadLayout()
{
    TGUILayout layout;

    try
    {
        UnicodeString const path = GUILayoutFilePath();
        if (!FileExists(path))
            return;

        std::unique_ptr<TMemIniFile> const ini(new TMemIniFile(path));
        layout = LoadGUILayout(*ini);
    }
    catch (...)
    {
        return; // A layout that can't be read only means the window opens with its defaults.
    }

    if (layout.Window.HasValue())
    {
        TGUIWindowBounds const& window = layout.Window.Value();
        int const pixelsPerInch = CurrentPPI;
        TRect const bounds(window.Left, window.Top, window.Left + ScaleFrom96(window.Width, pixelsPerInch),
            window.Top + ScaleFrom96(window.Height, pixelsPerInch));

        // Not where it can't be reached, e.g. on a monitor that's since been disconnected. Set as the window
        // placement's normal position, as VCL itself does when centering a form that opens maximized: that's where
        // such a window restores to, and setting BoundsRect before it's first shown doesn't change it.
        WINDOWPLACEMENT placement;
        ::ZeroMemory(&placement, sizeof(placement));
        placement.length = sizeof(placement);

        if (IsWindowReachable(bounds, MonitorWorkAreas()) && ::GetWindowPlacement(Handle, &placement))
        {
            TRect const primaryMonitor = Screen->PrimaryMonitor->BoundsRect;
            TRect const primaryWorkArea = Screen->PrimaryMonitor->WorkareaRect;
            TRect const normal = ScreenToWorkspace(bounds, primaryMonitor, primaryWorkArea);
            placement.rcNormalPosition.left = normal.Left;
            placement.rcNormalPosition.top = normal.Top;
            placement.rcNormalPosition.right = normal.Right;
            placement.rcNormalPosition.bottom = normal.Bottom;
            placement.showCmd = SW_HIDE; // Application->Run() shows it.
            ::SetWindowPlacement(Handle, &placement);
        }
    }

    if (layout.Maximized)
        WindowState = wsMaximized;

    m_QueuedLayout = layout;
    TThread::ForceQueue(nullptr, &this->QueuedApplyLayout);
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::LoadSelection()
{
    std::vector<TGUISavedTest> saved;

    try
    {
        UnicodeString const path = GUISelectionFilePath();
        if (!FileExists(path))
            return;

        saved = ParseGUISelection(ToUTF8(TFile::ReadAllText(path, TEncoding::UTF8)));
    }
    catch (...)
    {
        return; // A selection that can't be read only means every test starts out checked.
    }

    if (!saved.empty())
        AppendLog(DescribeGUISelectionRestore(RestoreGUISelection(m_TestList, saved)) + "\n");
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::LV_FailuresCustomDrawItem(TCustomListView* Sender, TListItem* Item,
    TCustomDrawState /*State*/, bool& /*DefaultDraw*/)
{
    // Not RunGuarded(): this runs while painting, where showing an error would only cause more painting. An item it
    // can't match to a test is simply drawn in the default color.
    void* const data = Item->Data;
    std::size_t const index = DataToIndex(data);
    if (index >= m_TestList.Count())
        return;

    bool const failed = (m_TestList.Status(index) == TGUITestStatus::Failed);
    Sender->Canvas->Font->Color = failed ? FailureColor : SkipColor;
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::LV_FailuresDblClick(TObject* /*Sender*/)
{
    RunGuarded("the Failures & Skips list", &this->DoSelectFailure);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::ObserverLogText(std::string const& text)
{
    AppendLog(text);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::ObserverTestFinished(TTestCaseRecord const& record,
    std::string const& testLog)
{
    TOptional<std::size_t> const index = m_TestList.IndexOf(record.GroupName, record.TestName);

    if (index.HasValue())
    {
        m_TestList.SetResult(index.Value(), record, testLog);
        UpdateTestNodeImages(index.Value());

        if (record.Outcome != TTestOutcome::Pass)
            AddFailureItem(index.Value());
    }

    m_CurrentTestIndex = TOptional<std::size_t>();
    ++m_RunFinishedCount;
    PB_Progress->Position = static_cast<int>(m_RunFinishedCount);

    if (record.Outcome == TTestOutcome::Fail)
        PB_Progress->State = pbsError;

    UpdateDetail();
    UpdateStatusBar();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::ObserverTestStarted(std::string const& groupName,
    std::string const& testName)
{
    m_CurrentTestIndex = m_TestList.IndexOf(groupName, testName);

    if (m_CurrentTestIndex.HasValue())
    {
        m_TestList.SetStatus(m_CurrentTestIndex.Value(), TGUITestStatus::Running);
        UpdateTestNodeImages(m_CurrentTestIndex.Value());
    }

    m_StatusMessage = "Running " + groupName + "." + testName;
    UpdateDetail();
    UpdateStatusBar();
}
//---------------------------------------------------------------------------
// Lets the window repaint and respond to Stop between tests, which run on this (the main) thread.
void __fastcall TFormASWUnitTestsBCBGUIMain::ObserverYield()
{
    Application->ProcessMessages();
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::QueuedApplyLayout()
{
    RunGuarded("restoring the layout", &this->ApplyQueuedLayout);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::QueuedClose()
{
    RunGuarded("closing the window", &this->DoCloseWindow);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::QueuedRunOnStart()
{
    RunGuarded("--run", &this->DoRunOnStart);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::QueuedSyncTreeChecks()
{
    RunGuarded("a test tree check box", &this->SyncTreeChecks);
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::RebuildNodeCache

    A recreated window loads its nodes back from what the old one saved, each with the Data BuildTestTree() gave it,
    which says whether it's a test node, and its index into m_TestList. Reading the nodes also creates the window, if
    it hasn't been yet, so it always sees the loaded ones.
*/
void TFormASWUnitTestsBCBGUIMain::RebuildNodeCache()
{
    m_TestNodes.assign(m_TestList.Count(), static_cast<TTreeNode*>(nullptr));

    for (TTreeNode* node = TV_Tests->Items->GetFirstNode(); node != nullptr; node = node->GetNext())
    {
        void* const data = node->Data;
        std::size_t const index = NodeIndex(data);

        if ((NodeKind(data) == TNodeKind::Test) && (index < m_TestNodes.size()))
            m_TestNodes[index] = node;
    }

    m_TreeNodesStale = false;
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::RefreshAllNodeImages()
{
    TTreeNodes* const nodes = TV_Tests->Items;
    TTreeUpdateScope const updating(nodes);

    for (TTreeNode* node = nodes->GetFirstNode(); node != nullptr; node = node->GetNext())
        SetNodeImage(node);
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::ResetSavedLayout

    For --layout-reset. The file is deleted now, rather than only not loaded, so a layout that caused a problem is gone
    even if the window doesn't get to close normally this time.
*/
void TFormASWUnitTestsBCBGUIMain::ResetSavedLayout()
{
    UnicodeString const path = GUILayoutFilePath();
    std::string outcome;

    if (!FileExists(path))
    {
        outcome = "there was no saved window layout to delete";
    }
    else
    {
        try
        {
            TFile::Delete(path);
        }
        catch (...)
        {
            // Reported below, from whether the file is still there.
        }

        outcome = FileExists(path) ?
                "the saved window layout couldn't be deleted (" + ToUTF8(path) + "), so it's ignored this time" :
                std::string("the saved window layout was deleted, so the window opens with the defaults");
    }

    AppendLog("Note: --layout-reset: " + outcome + ".\n");
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::RunCheckedTests()
{
    TGUITestNamesFilter const filter = m_TestList.CheckedFilter();
    RunTests(&filter, "the checked tests");
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::RunGuarded

    Runs an event handler's body. A C++ exception can't be reported by the VCL, which can only show one that escapes
    an event handler as an unexplained "External exception EEFFACE", so one is caught here and reported with its
    message and 'where' it happened, both in a message box and in the log. An RTL exception is left to the VCL, which
    shows it as usual.
*/
void TFormASWUnitTestsBCBGUIMain::RunGuarded(char const* where, TThreadMethod body)
{
    std::string description;

    try
    {
        body();
        return;
    }
    catch (Exception&)
    {
        throw;
    }
    catch (std::exception const& ex)
    {
        description = ex.what();
    }
    catch (...)
    {
        description = "an object that isn't a std::exception or an RTL Exception";
    }

    std::string const message = std::string("An unexpected error occurred in ") + where + ": " + description;
    AppendLog(message + "\n");
    Application->MessageBox(FromUTF8(message).c_str(), Application->Title.c_str(), MB_OK | MB_ICONERROR);
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::RunTests

    Runs on this (the main) thread, so tests that create VCL forms or controls work (unless --test-timeout-seconds
    moves each test to a worker thread); the observer's YieldCallback lets the window repaint and respond to Stop
    between tests. Every test's result, the Failures & Skips list, and the log are cleared first, so they only ever
    show one run.

    A shuffled run's seed comes from m_Shuffle, rather than being left to TTestHandler::Run() to choose, so the GUI
    knows it and can show it, and Shuffle Seed... can offer it to repeat this run's order.

    After a test times out, it may still be running on its abandoned thread, using its group, and after a stack
    overflow the process may be damaged, so no more tests run in this session (see m_RunAborted), and the process
    ends without its usual cleanup (see WasRunAborted()), as the console runner does.
*/
void TFormASWUnitTestsBCBGUIMain::RunTests(ITestFilter const* filter, std::string const& filterDescription)
{
    if (m_Running || m_RunAborted)
        return;

    m_TestList.ResetResults();
    RefreshAllNodeImages();
    LV_Failures->Items->Clear();
    RE_Log->Clear();
    UpdateDetail();

    m_RunTotalCount = m_TestList.CountMatching(filter);
    m_RunFinishedCount = 0;
    m_CurrentTestIndex = TOptional<std::size_t>();
    m_RunStarted = true;
    m_RunStopwatch = TStopwatch::StartNew();
    m_LastRunJUnitTestCases = TOptional<std::vector<TJUnitTestCase> >();

    PB_Progress->Max = static_cast<int>(std::max<std::size_t>(m_RunTotalCount, 1));
    PB_Progress->Position = 0;
    PB_Progress->State = pbsNormal;

    m_Observer.ResetStop();
    m_StatusMessage = "Running...";
    TOptional<unsigned int> const shuffleSeed = m_Shuffle.NextRunSeed();
    SetRunning(true); // Also shows the seed just chosen.

    // As in the console runner, an unhandled exception ends the run without a report, with its own exit code.
    try
    {
        TTestResults const results = m_Handler->Run(filter, filterDescription, shuffleSeed.HasValue(), shuffleSeed,
            m_Options.CLI.TestTimeoutSeconds, m_Options.CLI.CatchCrashes);
        m_Observer.Flush();

        if (results.TimedOut)
        {
            m_RunAborted = true;
            m_StatusMessage = "A test timed out, so the rest of the run was abandoned. Restart to run tests again.";
        }
        else if (results.Crashed)
        {
            m_RunAborted = true;
            m_StatusMessage = "A test crashed, so the rest of the run was abandoned. Restart to run tests again.";
        }
        else if (results.Stopped)
        {
            m_StatusMessage = "Stopped before every test ran.";
        }
        else
        {
            m_StatusMessage = "Done.";
        }

        m_LastRunExitCode = ExitCodeForResults(results);
        m_LastRunJUnitTestCases = ToJUnitTestCases(results.CaseRecords);

        // A failure is only noted in the log, so it can't hold up --exit.
        if (!m_Options.CLI.JUnitReportPath.empty())
            WriteJUnitReport(FromUTF8(m_Options.CLI.JUnitReportPath));
    }
    catch (TExceptFatalError const& fatalEx)
    {
        m_LastRunExitCode = fatalEx.IsUnknownException() ? ExitCode_UnhandledExceptionUnknown :
                ExitCode_UnhandledException;
        FailRunOnUnhandledException(fatalEx.what());
    }
    catch (std::exception const& stdEx)
    {
        m_LastRunExitCode = ExitCode_UnhandledException;
        FailRunOnUnhandledException(DescribeException(stdEx));
    }
    catch (Exception& rtlEx)
    {
        m_LastRunExitCode = ExitCode_UnhandledException;
        FailRunOnUnhandledException(DescribeException(rtlEx));
    }
    catch (...)
    {
        m_LastRunExitCode = ExitCode_UnhandledExceptionUnknown;
        FailRunOnUnhandledException(DescribeUnknownException());
    }

    m_RunStopwatch.Stop();
    SetRunning(false);

    if (m_CloseAfterRun)
    {
        // Not Close() directly: this may still be inside a button's click handling.
        TThread::ForceQueue(nullptr, &this->QueuedClose);
    }
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::SaveLayout()
{
    try
    {
        UnicodeString const path = GUILayoutFilePath();
        ForceDirectories(ExtractFileDir(path));

        std::unique_ptr<TMemIniFile> const ini(new TMemIniFile(path));
        SaveGUILayout(*ini, CurrentLayout());
        ini->UpdateFile();
    }
    catch (...)
    {
        // Not saving it (e.g. a read-only profile) only means the next start uses the defaults.
    }
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::SaveSelection()
{
    try
    {
        UnicodeString const path = GUISelectionFilePath();
        ForceDirectories(ExtractFileDir(path));

        TFile::WriteAllText(path, FromUTF8(FormatGUISelection(CaptureGUISelection(m_TestList))), TEncoding::UTF8);
    }
    catch (...)
    {
        // Not saving it (e.g. a read-only profile) only means the next start checks every test.
    }
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::SetNodeImage(TTreeNode* node)
{
    void* const data = node->Data;
    std::size_t const index = NodeIndex(data);
    TGUITestStatus status = TGUITestStatus::NotRun;

    switch (NodeKind(data))
    {
        case TNodeKind::Suite:
            status = m_TestList.SuiteStatus(m_TestList.SuitePaths().at(index));
            break;

        case TNodeKind::Group:
            status = m_TestList.GroupStatus(m_TestList.GroupNames().at(index));
            break;

        case TNodeKind::Test:
            status = m_TestList.Status(index);
            break;
    }

    int const image = static_cast<int>(status);
    node->ImageIndex = image;
    node->SelectedIndex = image;
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::SetRunning(bool running)
{
    m_Running = running;

    bool const canRun = !running && !m_RunAborted;
    bool const anyFailed = (m_TestList.StatusCount(TGUITestStatus::Failed) > 0);

    // A modal dialog opened mid-run would hold up the rest of the run until it was closed, since the tests run on
    // this thread.
    Act_About->Enabled = !running;
    Act_CommandLineOptions->Enabled = !running;
    Act_ExportJUnitReport->Enabled = !running && m_LastRunJUnitTestCases.HasValue();

    Act_RunFailed->Enabled = canRun && anyFailed;
    Act_RunSelected->Enabled = canRun;
    Act_SelectAll->Enabled = !running;
    Act_SelectFailed->Enabled = !running && anyFailed;
    Act_SelectNone->Enabled = !running;
    Act_Stop->Enabled = running;

    // The run in progress already has its order, so a change now would only mislead.
    Act_ShuffleOrder->Enabled = !running;
    Act_ShuffleSeed->Enabled = !running;

    // Rebuilding the tree mid-run would discard the nodes the run is updating.
    Act_FocusFilter->Enabled = !running;
    Edt_Filter->Enabled = !running;

    UpdateStatusBar();
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::ShowShuffleState()
{
    Act_ShuffleOrder->Checked = m_Shuffle.Enabled();
    UpdateStatusBar();
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::Start

    Loads the registered tests and applies the command line's options: --project-name to the caption, --filter and
    the partition options to which tests start out checked, --filter to the filter box, and --shuffle and
    --shuffle-seed to the Options menu. Also restores the window layout saved when it last closed.
*/
void TFormASWUnitTestsBCBGUIMain::Start(TGUIOptions const& options)
{
    m_Options = options;

    // The observer always gets the log without color codes, whatever --color says.
    m_Observer.LogTextCallback = &this->ObserverLogText;
    m_Observer.TestStartedCallback = &this->ObserverTestStarted;
    m_Observer.TestFinishedCallback = &this->ObserverTestFinished;
    m_Observer.YieldCallback = &this->ObserverYield;

    m_Handler.reset(new TTestHandler());
    m_Handler->SetRunObserver(&m_Observer); // Before Initialize(), so its output goes to the observer too.
    m_Handler->Initialize(m_Options.CLI.ProjectName);
    m_Observer.Flush();

    m_TestList.Load(m_Handler->GetTests());

    std::string filterDescription;
    std::unique_ptr<ITestFilter> const initialFilter(TCLIParser::BuildTestFilter(*m_Handler, m_Options.CLI,
        filterDescription));
    m_TestList.CheckMatching(initialFilter.get());

    // Notes about the command line, in the log, where they stay until the first run clears it.
    if (initialFilter.get() != nullptr)
        AppendLog("Initially checked: " + filterDescription + "\n");

    // Only when the command line chooses no tests, so an automated --run or a --filter launch never replaces the
    // selection saved interactively. Before the tree is built below.
    m_KeepSelection = !m_Options.RunOnStart && (initialFilter.get() == nullptr);

    if (m_KeepSelection)
        LoadSelection();

    for (std::vector<std::string>::const_iterator it = m_Options.IgnoredOptions.begin();
         it != m_Options.IgnoredOptions.end(); ++it)
    {
        AppendLog("Note: " + *it + " has no effect in the GUI, so it was ignored.\n");
    }

    if (m_Options.CLI.TestTimeoutSeconds.HasValue())
    {
        AppendLog("Note: --test-timeout-seconds runs each test on a worker thread, so a test that creates VCL forms or "
            "controls won't work with it.\n");
    }

    if (m_Options.LayoutIgnore)
        AppendLog("Note: --layout-ignore: the window layout isn't loaded or saved, so it opens with the defaults.\n");

    if (m_Options.LayoutReset)
        ResetSavedLayout();

    UpdateCaption();
    CreateStatusImages();

    m_Shuffle.ApplyCommandLine(m_Options.CLI.Shuffle, m_Options.CLI.ShuffleSeed);
    ShowShuffleState();

    // The filter box's case-insensitive contains-match shows every test --filter's pattern matches (and maybe a few
    // more, left unchecked), so seeding it with the pattern never hides a checked test.
    if (m_Options.CLI.HasFilter)
        Edt_Filter->Text = FromUTF8(m_Options.CLI.FilterPattern);

    // Called directly, since setting the text above doesn't necessarily raise OnChange before the window is showing.
    // Builds the tree.
    ApplyFilterBox();

    // Centered here rather than with the DFM's Position, which is poDesigned because LoadLayout() would otherwise
    // have to change Position at runtime, which needlessly recreates the whole window.
    CenterOnMonitor(Width, Height);

    // The DFM's layout, captured before LoadLayout() changes it, for Reset Layout. Loaded before --run is queued
    // below, so the panel sizes LoadLayout() queues are applied before the run starts.
    m_DefaultLayout = CurrentLayout();

    if (!m_Options.LayoutIgnore && !m_Options.LayoutReset)
        LoadLayout();

    // Queued, so it starts once Application->Run() is processing messages and the window is showing.
    if (m_Options.RunOnStart)
        TThread::ForceQueue(nullptr, &this->QueuedRunOnStart);
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::SyncTreeChecks

    Sets every node's check box from m_TestList: a test's to whether it's checked, and a group's or suite's to whether
    all, none, or only some of its tests are.
*/
void __fastcall TFormASWUnitTestsBCBGUIMain::SyncTreeChecks()
{
    TScopedFlag const syncing(m_SyncingChecks);

    for (TTreeNode* node = TV_Tests->Items->GetFirstNode(); node != nullptr; node = node->GetNext())
    {
        void* const data = node->Data;
        std::size_t const index = NodeIndex(data);

        switch (NodeKind(data))
        {
            case TNodeKind::Suite:
                node->CheckState = ToNodeCheckState(m_TestList.SuiteCheckState(m_TestList.SuitePaths().at(index)));
                break;

            case TNodeKind::Group:
                node->CheckState = ToNodeCheckState(m_TestList.GroupCheckState(m_TestList.GroupNames().at(index)));
                break;

            case TNodeKind::Test:
                node->CheckState = m_TestList.IsChecked(index) ? ncsChecked : ncsUnchecked;
                break;
        }
    }

    UpdateStatusBar(); // For its checked count
}
//---------------------------------------------------------------------------
TTreeNode* TFormASWUnitTestsBCBGUIMain::TestNode(std::size_t testIndex)
{
    if (m_TreeNodesStale)
        RebuildNodeCache();

    return (testIndex < m_TestNodes.size()) ? m_TestNodes[testIndex] : nullptr;
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::TV_TestsChange(TObject* /*Sender*/, TTreeNode* /*Node*/)
{
    RunGuarded("the test tree", &this->UpdateDetail);
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::TV_TestsCheckStateChanging

    With partial check boxes enabled, a click can cycle a check box into its partial state, which only a group's own
    tests should decide. So every click is taken over here (see DoCheckClick()): it toggles the clicked test, or all of
    the clicked group's tests, in m_TestList, and the tree is then updated from m_TestList, which also works out each
    group's partial state.
*/
void __fastcall TFormASWUnitTestsBCBGUIMain::TV_TestsCheckStateChanging(TCustomTreeView* /*Sender*/,
    TTreeNode* Node, TNodeCheckState /*NewCheckState*/, TNodeCheckState OldCheckState, bool& AllowChange)
{
    if (m_SyncingChecks)
        return;

    AllowChange = false;

    m_CheckClickNode = Node;
    m_CheckClickOldState = OldCheckState;
    RunGuarded("a test tree check box", &this->DoCheckClick);
    m_CheckClickNode = nullptr;
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::TV_TestsWindowProc

    VCL recreates the tree's window when Windows' system colors change (e.g. switching high contrast on or off), and
    when some of its properties change. Recreating it frees every TTreeNode, and loads new ones in their place,
    without the OnDeletion event that would otherwise say so.
*/
void __fastcall TFormASWUnitTestsBCBGUIMain::TV_TestsWindowProc(TMessage& message)
{
    if (message.Msg == WM_DESTROY)
        m_TreeNodesStale = true;

    m_TreeViewWindowProc(message);
}
//---------------------------------------------------------------------------
/*
    TFormASWUnitTestsBCBGUIMain::UpdateCaption

    "ASWUnitTestsBCB - Framework version: <version>", plus " - <project name>" when --project-name gave one other than
    the default.
*/
void TFormASWUnitTestsBCBGUIMain::UpdateCaption()
{
    std::string caption = "ASWUnitTestsBCB - Framework version: " + TTestHandler::GetVersionStr();

    if (m_Options.CLI.ProjectName != TCLIOptions().ProjectName)
        caption += " - " + m_Options.CLI.ProjectName;

    Caption = FromUTF8(caption);
}
//---------------------------------------------------------------------------
void __fastcall TFormASWUnitTestsBCBGUIMain::UpdateDetail()
{
    TTreeNode* const node = TV_Tests->Selected;
    Act_CopyDetails->Enabled = (node != nullptr);

    if (node == nullptr)
    {
        Mem_Detail->Clear();
        return;
    }

    void* const data = node->Data;
    std::size_t const index = NodeIndex(data);
    std::string text;

    switch (NodeKind(data))
    {
        case TNodeKind::Suite:
            text = m_TestList.SuiteDetailText(m_TestList.SuitePaths().at(index));
            break;

        case TNodeKind::Group:
            text = m_TestList.GroupDetailText(m_TestList.GroupNames().at(index));
            break;

        case TNodeKind::Test:
            text = m_TestList.DetailText(index);
            break;
    }

    Mem_Detail->Text = AdjustLineBreaks(FromUTF8(text));
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::UpdateStatusBar()
{
    // Counts only the tests the filter box shows, since those are the ones Run Selected runs.
    std::string checked = "Checked: " + UIntToString(static_cast<unsigned int>(m_TestList.CheckedCount())) + " / " +
        UIntToString(static_cast<unsigned int>(m_TestList.VisibleCount()));

    if (m_TestList.VisibleCount() != m_TestList.Count())
        checked += " shown";

    std::vector<std::string> panels;
    panels.push_back(checked);
    panels.push_back("Run: " + UIntToString(static_cast<unsigned int>(m_RunFinishedCount)) + " / " +
        UIntToString(static_cast<unsigned int>(m_RunTotalCount)));
    panels.push_back("Passed: " + UIntToString(static_cast<unsigned int>(m_TestList.StatusCount(
        TGUITestStatus::Passed))));
    panels.push_back("Failed: " + UIntToString(static_cast<unsigned int>(m_TestList.StatusCount(
        TGUITestStatus::Failed))));
    panels.push_back("Skipped: " + UIntToString(static_cast<unsigned int>(m_TestList.StatusCount(
        TGUITestStatus::Skipped))));
    std::string elapsed;

    if (m_RunStarted)
    {
        double const seconds = m_RunStopwatch.Elapsed.TotalSeconds;
        elapsed = FormatSeconds(seconds);
    }

    panels.push_back(elapsed);
    panels.push_back(m_Shuffle.StatusText());
    panels.push_back(m_StatusMessage);

    for (std::size_t i = 0; i < panels.size(); ++i)
        SB_Status->Panels->Items[static_cast<int>(i)]->Text = FromUTF8(panels[i]);
}
//---------------------------------------------------------------------------
void TFormASWUnitTestsBCBGUIMain::UpdateTestNodeImages(std::size_t testIndex)
{
    TTreeNode* const testNode = TestNode(testIndex);
    if (testNode == nullptr)
        return; // Hidden by the filter box, e.g. a hidden failure rerun by Run Failed.

    // The test's own image, then those of its group and the suites holding it, whose status depends on it.
    for (TTreeNode* node = testNode; node != nullptr; node = node->Parent)
        SetNodeImage(node);
}
//---------------------------------------------------------------------------
bool TFormASWUnitTestsBCBGUIMain::WasRunAborted() const
{
    return m_RunAborted;
}
//---------------------------------------------------------------------------
bool TFormASWUnitTestsBCBGUIMain::WriteJUnitReport(UnicodeString const& path)
{
    bool const written = m_LastRunJUnitTestCases.HasValue() && TJUnitReportWriter::Write(ToUTF8(path),
        m_Handler->GetProjectName(), m_LastRunJUnitTestCases.Value());

    if (written)
        AppendLog("JUnit report written to: " + ToUTF8(path) + "\n");
    else
        AppendLog("Error: could not write JUnit report to: " + ToUTF8(path) + "\n");

    return written;
}
//---------------------------------------------------------------------------
