/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_RunObserver.cpp
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
#include "Test_ASWUnitTestsBCB_GUI_RunObserver.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_RunObserver.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Classes;
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TCallbackRecorder
//
// Wires a TGUIRunObserver's closures up to record, in order, everything
// they're called with, as "log:<text>", "started:<test>",
// "finished:<test>" and "yield" entries; each finished test's own log is
// kept separately. Also tracks whether every closure ran on the thread that
// constructed it.
/////////////////////////////////////////////////////////////////////////////
class TCallbackRecorder : public TObject
{
private:
    DWORD const m_ConstructingThread;

    void Record(std::string const& call)
    {
        CallbacksOnConstructingThread = CallbacksOnConstructingThread &&
            (::GetCurrentThreadId() == m_ConstructingThread);
        Calls += call + "|";
    }

public:
    std::string Calls; // Each call followed by "|"
    bool CallbacksOnConstructingThread;
    std::vector<std::string> TestLogs;

public:
    explicit __fastcall TCallbackRecorder(ASWUnitTestsBCB::TGUIRunObserver& observer)
        : m_ConstructingThread(::GetCurrentThreadId()),
          CallbacksOnConstructingThread(true)
    {
        observer.LogTextCallback = LogText;
        observer.TestFinishedCallback = TestFinished;
        observer.TestStartedCallback = TestStarted;
        observer.YieldCallback = Yield;
    }

    void __fastcall LogText(std::string const& text)
    {
        Record("log:" + text);
    }

    void __fastcall TestFinished(ASWUnitTestsBCB::TTestCaseRecord const& record, std::string const& testLog)
    {
        Record("finished:" + record.TestName);
        TestLogs.push_back(testLog);
    }

    void __fastcall TestStarted(std::string const& /*groupName*/, std::string const& testName)
    {
        Record("started:" + testName);
    }

    void __fastcall Yield()
    {
        Record("yield");
    }
};


/////////////////////////////////////////////////////////////////////////////
// TObserverThread
//
// Calls one of a TGUIRunObserver's methods from a thread of its own.
/////////////////////////////////////////////////////////////////////////////
class TObserverThread : public TThread
{
public:
    enum TAction
    {
        LogFromWorker,
        RequestStop
    };

private:
    ASWUnitTestsBCB::TGUIRunObserver& m_Observer;
    TAction const m_Action;

protected:
    virtual void __fastcall Execute()
    {
        if (m_Action == LogFromWorker)
            m_Observer.OnLog("from a worker thread\n");
        else
            m_Observer.RequestStop();
    }

public:
    __fastcall TObserverThread(ASWUnitTestsBCB::TGUIRunObserver& observer, TAction action)
        : TThread(true),
          m_Observer(observer),
          m_Action(action)
    {
    }
};

void CallOnWorker(ASWUnitTestsBCB::TGUIRunObserver& observer, TObserverThread::TAction action);

//---------------------------------------------------------------------------
// Runs 'action' on a new thread and waits for it to end.
void CallOnWorker(ASWUnitTestsBCB::TGUIRunObserver& observer, TObserverThread::TAction action)
{
    std::unique_ptr<TObserverThread> const thread(new TObserverThread(observer, action));
    thread->Start();
    thread->WaitFor();
}
//---------------------------------------------------------------------------

} // namespace


namespace ASWUnitTestsBCB
{

// The inner group the tests below run. It isn't registered, so the program's own run never includes it.

/////////////////////////////////////////////////////////////////////////////
// TInnerGUIObs_Observed: one test that fails a Check and one that passes,
// so each logs distinct text
/////////////////////////////////////////////////////////////////////////////
class TInnerGUIObs_Observed : public TTestGroupBase
{
public:
    __fastcall TInnerGUIObs_Observed()
        : TTestGroupBase("Fixture_GUIObserved")
    {
    }

__published:
    void __fastcall Test_FailsCheck();
    void __fastcall Test_Passes();
};

void __fastcall TInnerGUIObs_Observed::Test_FailsCheck()
{
    CheckTrue(false, ASWBCB_HERE, "deliberate Check failure, fixture test");
}

void __fastcall TInnerGUIObs_Observed::Test_Passes()
{
    CheckTrue(true, ASWBCB_HERE, "trivially passes");
}


/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_RunObserver
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_GUI_RunObserver::TTest_ASWUnitTestsBCB_GUI_RunObserver()
    : inherited("ASWUnitTestsBCB_GUI_RunObserver_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_RunObserver::Test_Events_ArriveInOrderWithEachTestsOwnLog()
{
    // Arrange
    TGUIRunObserver observer;
    std::unique_ptr<TCallbackRecorder> const recorder(new TCallbackRecorder(observer));
    TTestCaseRecord record;
    record.GroupName = "Group";
    record.TestName = "Test";

    // Act
    observer.OnLog("before\n");
    observer.OnTestStarted("Group", "Test");
    observer.OnLog("during\n");
    observer.OnTestFinished(record);

    // Assert
    CheckEquals("log:before\n|started:Test|yield|log:during\n|finished:Test|yield|", recorder->Calls, ASWBCB_HERE,
        "earlier log text is handed over before the test starts, and the test's own before it finishes");
    AssertEquals(1, recorder->TestLogs.size(), ASWBCB_HERE, "one finished test");
    CheckEquals("during\n", recorder->TestLogs[0], ASWBCB_HERE,
        "its own log holds only what was logged while it ran");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_RunObserver::Test_Flush_HandsOverLogAfterLastTest()
{
    // Arrange
    TGUIRunObserver observer;
    std::unique_ptr<TCallbackRecorder> const recorder(new TCallbackRecorder(observer));

    // Act
    observer.OnLog("Tests done\n");
    std::string const callsBeforeFlush = recorder->Calls;
    observer.Flush();
    observer.Flush(); // Nothing left, so no second call.

    // Assert
    CheckEquals("", callsBeforeFlush, ASWBCB_HERE, "logged text waits to be handed over");
    CheckEquals("log:Tests done\n|", recorder->Calls, ASWBCB_HERE, "Flush() hands it over, once");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_RunObserver::Test_OnLog_FromWorkerThreadWaitsForNextEvent()
{
    // Arrange
    TGUIRunObserver observer;
    std::unique_ptr<TCallbackRecorder> const recorder(new TCallbackRecorder(observer));

    // Act
    CallOnWorker(observer, TObserverThread::LogFromWorker);
    std::string const callsAfterWorker = recorder->Calls;
    observer.Flush();

    // Assert
    CheckEquals("", callsAfterWorker, ASWBCB_HERE, "text logged on a worker thread isn't handed over there");
    CheckEquals("log:from a worker thread\n|", recorder->Calls, ASWBCB_HERE, "but by the next call here");
    CheckTrue(recorder->CallbacksOnConstructingThread, ASWBCB_HERE, "on the thread that made the call");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_RunObserver::Test_Run_ReportsEachTestWithItsOwnLogUnderTimeout()
{
    // Arrange: a timeout runs each test on a worker thread, where its log text is written, so this covers the path
    // the GUI relies on most: log text from another thread, closures on this one.
    std::unique_ptr<TInnerGUIObs_Observed> const fixture(new TInnerGUIObs_Observed());
    TGUIRunObserver observer;
    std::unique_ptr<TCallbackRecorder> const recorder(new TCallbackRecorder(observer));
    fixture->SetRunObserver(&observer);

    // Act
    fixture->Run(nullptr, TOptional<unsigned int>(), 30u); // Only long enough never to end a test.
    observer.Flush();

    // Assert
    AssertEquals(2, recorder->TestLogs.size(), ASWBCB_HERE, "both tests were reported");
    CheckContains(recorder->TestLogs[0], "Running test: Fixture_GUIObserved.FailsCheck", ASWBCB_HERE,
        "the first test's log starts with its own Running line");
    CheckContains(recorder->TestLogs[0], "deliberate Check failure", ASWBCB_HERE, "including its Check failure");
    CheckContains(recorder->TestLogs[1], "Running test: Fixture_GUIObserved.Passes", ASWBCB_HERE,
        "the second test's log has its own Running line");
    CheckNotContains(recorder->TestLogs[1], "deliberate Check failure", ASWBCB_HERE,
        "and none of the first test's text");
    CheckTrue(recorder->CallbacksOnConstructingThread, ASWBCB_HERE,
        "every closure ran on this thread, though the tests logged on a worker");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_RunObserver::Test_StopRequest_CanBeRequestedAndReset()
{
    // Arrange
    TGUIRunObserver observer;
    bool const initially = observer.StopRequested();

    // Act
    CallOnWorker(observer, TObserverThread::RequestStop);
    bool const afterRequest = observer.StopRequested();
    observer.ResetStop();
    bool const afterReset = observer.StopRequested();

    // Assert
    CheckFalse(initially, ASWBCB_HERE, "no stop is requested initially");
    CheckTrue(afterRequest, ASWBCB_HERE, "a stop requested from another thread is seen");
    CheckFalse(afterReset, ASWBCB_HERE, "ResetStop() clears it for the next run");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_GUI_RunObserver)
