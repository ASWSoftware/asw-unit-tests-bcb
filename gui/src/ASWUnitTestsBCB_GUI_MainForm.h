/* **************************************************************************
ASWUnitTestsBCB_GUI_MainForm.h
Author: Anthony S. West - ASW Software

The VCL GUI runner's main window.

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

#ifndef ASWUnitTestsBCB_GUI_MainFormH
#define ASWUnitTestsBCB_GUI_MainFormH
//---------------------------------------------------------------------------
#include <System.Actions.hpp>
#include <System.Classes.hpp>
#include <System.ImageList.hpp>
#include <Vcl.ActnList.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ImgList.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.StdCtrls.hpp>
//---------------------------------------------------------------------------
// IDE includes above here
//---------------------------------------------------------------------------
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.Diagnostics.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_CommandLine.h"
#include "ASWUnitTestsBCB_GUI_Layout.h"
#include "ASWUnitTestsBCB_GUI_RunObserver.h"
#include "ASWUnitTestsBCB_GUI_Shuffle.h"
#include "ASWUnitTestsBCB_GUI_TestList.h"
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_JUnitReport.h"
#include "ASWUnitTestsBCB_TestBase.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// TFormASWUnitTestsBCBGUIMain
//
// The IDE manages the __published section, which names VCL types
// unqualified, as it writes them.
/////////////////////////////////////////////////////////////////////////////
class TFormASWUnitTestsBCBGUIMain : public TForm
{
private:
    typedef TForm inherited;

__published: // IDE-managed Components
    TAction* Act_About;
    TAction* Act_CollapseAll;
    TAction* Act_CommandLineOptions;
    TAction* Act_CopyDetails;
    TAction* Act_Exit;
    TAction* Act_ExpandAll;
    TAction* Act_ExportJUnitReport;
    TAction* Act_FocusFilter;
    TAction* Act_ResetLayout;
    TAction* Act_RunFailed;
    TAction* Act_RunSelected;
    TAction* Act_SelectAll;
    TAction* Act_SelectFailed;
    TAction* Act_SelectNone;
    TAction* Act_ShuffleOrder;
    TAction* Act_ShuffleSeed;
    TAction* Act_Stop;
    TActionList* AL_Main;
    TButton* Btn_CopyDetails;
    TButton* Btn_RunFailed;
    TButton* Btn_RunSelected;
    TButton* Btn_SelectAll;
    TButton* Btn_SelectFailed;
    TButton* Btn_SelectNone;
    TButton* Btn_Stop;
    TEdit* Edt_Filter;
    TImageList* IL_Status;
    TLabel* Lbl_Filter;
    TListView* LV_Failures;
    TMemo* Mem_Detail;
    TMenuItem* MI_File;
    TMenuItem* MI_FileExit;
    TMenuItem* MI_FileExportJUnitReport;
    TMenuItem* MI_FileSeparator1;
    TMenuItem* MI_Help;
    TMenuItem* MI_HelpAbout;
    TMenuItem* MI_HelpCommandLineOptions;
    TMenuItem* MI_Options;
    TMenuItem* MI_OptionsShuffleOrder;
    TMenuItem* MI_OptionsShuffleSeed;
    TMenuItem* MI_Run;
    TMenuItem* MI_RunFailed;
    TMenuItem* MI_RunSelected;
    TMenuItem* MI_RunStop;
    TMenuItem* MI_Tests;
    TMenuItem* MI_TestsCopyDetails;
    TMenuItem* MI_TestsFilter;
    TMenuItem* MI_TestsSelectAll;
    TMenuItem* MI_TestsSelectFailed;
    TMenuItem* MI_TestsSelectNone;
    TMenuItem* MI_TestsSeparator1;
    TMenuItem* MI_View;
    TMenuItem* MI_ViewCollapseAll;
    TMenuItem* MI_ViewExpandAll;
    TMenuItem* MI_ViewResetLayout;
    TMenuItem* MI_ViewSeparator1;
    TMainMenu* MM_Main;
    TProgressBar* PB_Progress;
    TPageControl* PC_Results;
    TPanel* Pnl_Results;
    TPanel* Pnl_Toolbar;
    TRichEdit* RE_Log;
    TStatusBar* SB_Status;
    TSaveDialog* SD_JUnitReport;
    TSplitter* Spl_Detail;
    TSplitter* Spl_Tests;
    TTabSheet* TS_Failures;
    TTabSheet* TS_Log;
    TTreeView* TV_Tests;
    void __fastcall Act_AboutExecute(TObject* Sender);
    void __fastcall Act_CollapseAllExecute(TObject* Sender);
    void __fastcall Act_CommandLineOptionsExecute(TObject* Sender);
    void __fastcall Act_CopyDetailsExecute(TObject* Sender);
    void __fastcall Act_ExitExecute(TObject* Sender);
    void __fastcall Act_ExpandAllExecute(TObject* Sender);
    void __fastcall Act_ExportJUnitReportExecute(TObject* Sender);
    void __fastcall Act_FocusFilterExecute(TObject* Sender);
    void __fastcall Act_ResetLayoutExecute(TObject* Sender);
    void __fastcall Act_RunFailedExecute(TObject* Sender);
    void __fastcall Act_RunSelectedExecute(TObject* Sender);
    void __fastcall Act_SelectAllExecute(TObject* Sender);
    void __fastcall Act_SelectFailedExecute(TObject* Sender);
    void __fastcall Act_SelectNoneExecute(TObject* Sender);
    void __fastcall Act_ShuffleOrderExecute(TObject* Sender);
    void __fastcall Act_ShuffleSeedExecute(TObject* Sender);
    void __fastcall Act_StopExecute(TObject* Sender);
    void __fastcall Edt_FilterChange(TObject* Sender);
    void __fastcall FormClose(TObject* Sender, TCloseAction& Action);
    void __fastcall FormCloseQuery(TObject* Sender, bool& CanClose);
    void __fastcall LV_FailuresCustomDrawItem(TCustomListView* Sender, TListItem* Item, TCustomDrawState State,
        bool& DefaultDraw);
    void __fastcall LV_FailuresDblClick(TObject* Sender);
    void __fastcall TV_TestsChange(TObject* Sender, TTreeNode* Node);
    void __fastcall TV_TestsCheckStateChanging(TCustomTreeView* Sender, TTreeNode* Node,
        TNodeCheckState NewCheckState, TNodeCheckState OldCheckState, bool& AllowChange);

private: // User declarations
    // Declared before m_Handler, so it's destroyed after the handler that points at it.
    ASWUnitTestsBCB::TGUIRunObserver m_Observer;
    std::unique_ptr<ASWUnitTestsBCB::TTestHandler> m_Handler;
    ASWUnitTestsBCB::TGUIOptions m_Options;
    ASWUnitTestsBCB::TGUITestList m_TestList;
    // The tree node a check box click is being handled for, and its check state before the click; see
    // TV_TestsCheckStateChanging().
    TTreeNode* m_CheckClickNode;
    TNodeCheckState m_CheckClickOldState;
    // Set when the window was asked to close during a run, which then closes it once the run has stopped.
    bool m_CloseAfterRun;
    // The test that has started but not yet finished, if any.
    ASWUnitTestsBCB::TOptional<std::size_t> m_CurrentTestIndex;
    // The layout from the DFM, at 96 DPI, captured before the saved one is applied, for Reset Layout.
    ASWUnitTestsBCB::TGUILayout m_DefaultLayout;
    // Set when the command line doesn't choose the tests itself (no --run, --filter, or partition options), so the
    // saved test selection is restored at start and saved again at close.
    bool m_KeepSelection;
    // The exit code the console runner would have returned for the latest run; see ExitCode().
    int m_LastRunExitCode;
    // The latest run's JUnit report entries, for --report-junit and Export JUnit Report. No value until a run finishes,
    // and after a run an unhandled exception ended, which has no report, as in the console runner.
    ASWUnitTestsBCB::TOptional<std::vector<ASWUnitTestsBCB::TJUnitTestCase> > m_LastRunJUnitTestCases;
    // The layout LoadLayout() queues the panel sizes of, for ApplyQueuedLayout().
    ASWUnitTestsBCB::TGUILayout m_QueuedLayout;
    // Set once a test has timed out (and may still be running on its abandoned thread) or a stack overflow has ended
    // a run, which leaves the process in no state to run tests again; see RunTests().
    bool m_RunAborted;
    std::size_t m_RunFinishedCount;
    bool m_Running;
    bool m_RunStarted; // Whether any run has started, so m_RunStopwatch has a time to show
    System::Diagnostics::TStopwatch m_RunStopwatch;
    std::size_t m_RunTotalCount;
    // Whether runs are shuffled, and with which seed; starts from --shuffle and --shuffle-seed.
    ASWUnitTestsBCB::TGUIShuffle m_Shuffle;
    std::string m_StatusMessage;
    // True while the tree's check boxes are being set from m_TestList, so TV_TestsCheckStateChanging() lets those
    // changes through instead of treating them as clicks.
    bool m_SyncingChecks;
    // By index into m_TestList; nullptr while hidden. Read through TestNode(), which rebuilds it when the tree's window
    // has been recreated.
    std::vector<TTreeNode*> m_TestNodes;
    // Set when the tree's window is destroyed, which frees every TTreeNode, so m_TestNodes points at freed nodes until
    // RebuildNodeCache() finds the ones the recreated window loaded in their place.
    bool m_TreeNodesStale;
    TWndMethod m_TreeViewWindowProc; // TV_Tests' own WindowProc, which TV_TestsWindowProc() passes messages on to

private:
    // Adds a failed or skipped test to the Failures & Skips list.
    void AddFailureItem(std::size_t testIndex);
    // Appends log text to the Log tab, coloring failure and skip lines.
    void AppendLog(std::string const& text);
    // Shows only the tests matching the filter box, then rebuilds the tree.
    void __fastcall ApplyFilterBox();
    // Sets the test tree width and detail pane height that 'layout' has, fitted to the window's current size.
    void ApplyPanelSizes(ASWUnitTestsBCB::TGUILayout const& layout);
    void __fastcall ApplyQueuedLayout();
    void BuildTestTree();
    void CenterOnMonitor(int width, int height);
    void CreateStatusImages();
    // The window's layout as it is now, for saving.
    ASWUnitTestsBCB::TGUILayout CurrentLayout();
    // The bodies of the actions, run through RunGuarded().
    void __fastcall DoAbout();
    void __fastcall DoCheckClick();
    void __fastcall DoCloseWindow();
    void __fastcall DoCollapseAll();
    void __fastcall DoCommandLineOptions();
    void __fastcall DoCopyDetails();
    void __fastcall DoExpandAll();
    void __fastcall DoExportJUnitReport();
    void __fastcall DoFocusFilter();
    void __fastcall DoRequestCloseAfterRun();
    void __fastcall DoResetLayout();
    void __fastcall DoRunFailed();
    void __fastcall DoRunOnStart();
    void __fastcall DoRunSelected();
    void __fastcall DoSaveOnClose();
    void __fastcall DoSelectAll();
    void __fastcall DoSelectFailed();
    void __fastcall DoSelectFailure();
    void __fastcall DoSelectNone();
    void __fastcall DoShuffleOrder();
    void __fastcall DoShuffleSeed();
    void __fastcall DoStop();
    // Ends a run that an unhandled exception escaped from, described by 'description'.
    void FailRunOnUnhandledException(std::string const& description);
    // Restores the layout SaveLayout() saved, if any; see the definition.
    void LoadLayout();
    // Restores the test selection SaveSelection() saved, if any, noting what it restored in the log.
    void LoadSelection();
    // The observer's closures (see TGUIRunObserver).
    void __fastcall ObserverLogText(std::string const& text);
    void __fastcall ObserverTestFinished(ASWUnitTestsBCB::TTestCaseRecord const& record, std::string const& testLog);
    void __fastcall ObserverTestStarted(std::string const& groupName, std::string const& testName);
    void __fastcall ObserverYield();
    // Queued with TThread::ForceQueue(), to run once the current message has been handled.
    void __fastcall QueuedApplyLayout();
    void __fastcall QueuedClose();
    void __fastcall QueuedRunOnStart();
    void __fastcall QueuedSyncTreeChecks();
    // Refills m_TestNodes from the tree's test nodes, by the index each one holds in its Data.
    void RebuildNodeCache();
    void RefreshAllNodeImages();
    // Deletes the saved layout, for --layout-reset, noting the outcome in the log.
    void ResetSavedLayout();
    // Runs the checked tests that are shown, as Run Selected and --run do.
    void RunCheckedTests();
    // Runs an event handler's body, reporting any C++ exception from it by name; see the definition.
    void RunGuarded(char const* where, System::Classes::TThreadMethod body);
    // Runs the tests 'filter' matches, updating the tree and status as each one starts and finishes.
    void RunTests(ASWUnitTestsBCB::ITestFilter const* filter, std::string const& filterDescription);
    // Saves the window's layout for the next start, ignoring any error.
    void SaveLayout();
    // Saves which tests are checked and shown for the next start, ignoring any error.
    void SaveSelection();
    // Sets a tree node's status image from m_TestList: a test's own status, or a group's or suite's, from its tests.
    void SetNodeImage(TTreeNode* node);
    void SetRunning(bool running);
    // Shows m_Shuffle's state: Run in Shuffled Order's check mark, and the status bar.
    void ShowShuffleState();
    void __fastcall SyncTreeChecks();
    // The test's tree node, or nullptr while it's hidden.
    TTreeNode* TestNode(std::size_t testIndex);
    // Watches for the tree's window being destroyed; see m_TreeNodesStale.
    void __fastcall TV_TestsWindowProc(TMessage& message);
    void UpdateCaption();
    // Shows the selected test's, group's or suite's details in the detail pane.
    void __fastcall UpdateDetail();
    void UpdateStatusBar();
    // Updates a test node's status image, and those of its group's and suites' nodes, if they're shown.
    void UpdateTestNodeImages(std::size_t testIndex);
    // Writes the latest run's JUnit report to 'path', noting the outcome in the log. Returns false if it couldn't be
    // written.
    bool WriteJUnitReport(System::UnicodeString const& path);

public: // User declarations
    __fastcall TFormASWUnitTestsBCBGUIMain(TComponent* Owner);
    __fastcall virtual ~TFormASWUnitTestsBCBGUIMain();

    // The process exit code: with --exit, what the console runner would have returned for the --run run; otherwise
    // ExitCode_Success, since the GUI was used interactively.
    int ExitCode() const;
    // Loads the registered tests and applies 'options' (see WinMain()). Call once, right after creating the form.
    void Start(ASWUnitTestsBCB::TGUIOptions const& options);
    // Whether a run was aborted by a timeout or a stack overflow, after which the process must end without its usual
    // cleanup (see WinMain()), as the console runner does: an abandoned test may still be running.
    bool WasRunAborted() const;
};
//---------------------------------------------------------------------------
extern PACKAGE TFormASWUnitTestsBCBGUIMain* FormASWUnitTestsBCBGUIMain;
//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_GUI_MainFormH
