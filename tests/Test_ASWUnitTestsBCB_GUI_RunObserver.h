/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_RunObserver.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_GUI_RunObserver.h.

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

//---------------------------------------------------------------------------
#ifndef Test_ASWUnitTestsBCB_GUI_RunObserverH
#define Test_ASWUnitTestsBCB_GUI_RunObserverH
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_RunObserver
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_GUI_RunObserver : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

public:
    __fastcall TTest_ASWUnitTestsBCB_GUI_RunObserver();

__published:
    void __fastcall Test_Events_ArriveInOrderWithEachTestsOwnLog();
    void __fastcall Test_Flush_HandsOverLogAfterLastTest();
    void __fastcall Test_OnLog_FromWorkerThreadWaitsForNextEvent();
    void __fastcall Test_Run_ReportsEachTestWithItsOwnLogUnderTimeout();
    void __fastcall Test_StopRequest_CanBeRequestedAndReset();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_GUI_RunObserverH
