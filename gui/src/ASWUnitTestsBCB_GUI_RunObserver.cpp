/* **************************************************************************
ASWUnitTestsBCB_GUI_RunObserver.cpp
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
#include "ASWUnitTestsBCB_GUI_RunObserver.h"
//---------------------------------------------------------------------------
#include <System.SyncObjs.hpp>
#include <windows.h>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Syncobjs;
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TLockScope
//
// Holds a critical section for its lifetime, releasing it even if an
// exception leaves the scope.
/////////////////////////////////////////////////////////////////////////////
class TLockScope
{
private:
    TCriticalSection& m_Lock;

    TLockScope(TLockScope const&);            // Not copyable
    TLockScope& operator=(TLockScope const&); // Not copyable

public:
    explicit TLockScope(TCriticalSection& lock)
        : m_Lock(lock)
    {
        m_Lock.Acquire();
    }

    ~TLockScope()
    {
        m_Lock.Release();
    }
};

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TGUIRunObserver
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TGUIRunObserver::TGUIRunObserver()
    : m_PendingLogLock(new TCriticalSection()),
      m_StopRequested(0),
      LogTextCallback(nullptr),
      TestFinishedCallback(nullptr),
      TestStartedCallback(nullptr),
      YieldCallback(nullptr)
{
}
//---------------------------------------------------------------------------
TGUIRunObserver::~TGUIRunObserver()
{
}
//---------------------------------------------------------------------------
void TGUIRunObserver::Flush()
{
    std::string const text = TakePendingLog();

    if (!text.empty() && (LogTextCallback != nullptr))
        LogTextCallback(text);
}
//---------------------------------------------------------------------------
void TGUIRunObserver::OnLog(std::string const& text)
{
    TLockScope const lock(*m_PendingLogLock);
    m_PendingLog += text;
}
//---------------------------------------------------------------------------
void TGUIRunObserver::OnTestFinished(TTestCaseRecord const& record)
{
    // Everything logged since the test started belongs to it, including its own "Finished test" line.
    std::string const text = TakePendingLog();
    m_CurrentTestLog += text;

    if (!text.empty() && (LogTextCallback != nullptr))
        LogTextCallback(text);

    if (TestFinishedCallback != nullptr)
        TestFinishedCallback(record, m_CurrentTestLog);

    m_CurrentTestLog.clear();

    if (YieldCallback != nullptr)
        YieldCallback();
}
//---------------------------------------------------------------------------
void TGUIRunObserver::OnTestStarted(std::string const& groupName, std::string const& testName)
{
    // Anything logged before the test started (e.g. its group's setup) doesn't belong to it.
    Flush();
    m_CurrentTestLog.clear();

    if (TestStartedCallback != nullptr)
        TestStartedCallback(groupName, testName);

    if (YieldCallback != nullptr)
        YieldCallback();
}
//---------------------------------------------------------------------------
void TGUIRunObserver::RequestStop()
{
    ::InterlockedExchange(&m_StopRequested, 1);
}
//---------------------------------------------------------------------------
void TGUIRunObserver::ResetStop()
{
    ::InterlockedExchange(&m_StopRequested, 0);
}
//---------------------------------------------------------------------------
bool TGUIRunObserver::StopRequested()
{
    return ::InterlockedCompareExchange(&m_StopRequested, 0, 0) != 0;
}
//---------------------------------------------------------------------------
std::string TGUIRunObserver::TakePendingLog()
{
    TLockScope const lock(*m_PendingLogLock);

    std::string text;
    text.swap(m_PendingLog);
    return text;
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
