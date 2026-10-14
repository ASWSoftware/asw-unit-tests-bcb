/* **************************************************************************
ASWUnitTestsBCB_GUI_RunObserver.h
Author: Anthony S. West - ASW Software

The VCL GUI runner's ITestRunObserver.

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

#ifndef ASWUnitTestsBCB_GUI_RunObserverH
#define ASWUnitTestsBCB_GUI_RunObserverH
//---------------------------------------------------------------------------
#include <memory>
#include <string>
//---------------------------------------------------------------------------
#include <System.hpp>
#include <System.SyncObjs.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TGUIRunObserver
//
// The VCL GUI runner's ITestRunObserver. It contains no VCL code itself: it
// hands each event to the closures below, which the main form sets, so its
// behavior can be unit tested without a window (see
// tests/Test_ASWUnitTestsBCB_GUI_RunObserver.cpp). A closure left nullptr
// isn't called.
//
// Log text can arrive from a test's worker thread (see ITestRunObserver and
// --test-timeout-seconds), so OnLog() only appends it to a buffer under a
// lock. The buffer is handed to LogTextCallback when a test starts or
// finishes, and by Flush(), so every closure runs on the thread that called
// TTestHandler::Run(), which for the GUI is its main (VCL) thread.
//
// Text logged between a test's start and finish is also collected as that
// test's own log and passed to TestFinishedCallback, for the detail pane.
/////////////////////////////////////////////////////////////////////////////
class TGUIRunObserver : public ITestRunObserver
{
public:
    typedef void __fastcall (__closure* TLogTextEvent)(std::string const& text);
    typedef void __fastcall (__closure* TTestFinishedEvent)(TTestCaseRecord const& record, std::string const& testLog);
    typedef void __fastcall (__closure* TTestStartedEvent)(std::string const& groupName, std::string const& testName);
    typedef void __fastcall (__closure* TYieldEvent)();

private:
    std::string m_CurrentTestLog;
    std::string m_PendingLog;
    std::unique_ptr<System::Syncobjs::TCriticalSection> const m_PendingLogLock;
    volatile long m_StopRequested; // Nonzero once RequestStop() is called; changed with interlocked calls

private:
    TGUIRunObserver(TGUIRunObserver const&);            // Not copyable
    TGUIRunObserver& operator=(TGUIRunObserver const&); // Not copyable

    std::string TakePendingLog();

public:
    // Receives log text, in the order it was logged.
    TLogTextEvent LogTextCallback;
    // Receives each test's record, plus the text it logged between starting and finishing.
    TTestFinishedEvent TestFinishedCallback;
    TTestStartedEvent TestStartedCallback;
    // Called after each test starts and after it finishes, so the GUI can process pending messages (repaint, a click
    // on Stop) while tests run on its main thread.
    TYieldEvent YieldCallback;

public:
    TGUIRunObserver();
    virtual ~TGUIRunObserver();

    // Hands any buffered log text to LogTextCallback. Call after TTestHandler::Initialize() or Run() returns, for the
    // text logged after the last test event.
    void Flush();
    virtual void OnLog(std::string const& text);
    virtual void OnTestFinished(TTestCaseRecord const& record);
    virtual void OnTestStarted(std::string const& groupName, std::string const& testName);
    // Asks the run to stop before its next test. Safe to call from any thread.
    void RequestStop();
    // Clears an earlier RequestStop(), ready for the next run.
    void ResetStop();
    virtual bool StopRequested();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_GUI_RunObserverH
