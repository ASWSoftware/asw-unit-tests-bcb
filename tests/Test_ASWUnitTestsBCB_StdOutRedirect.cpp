/* **************************************************************************
Test_ASWUnitTestsBCB_StdOutRedirect.cpp
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_StdOutRedirect.h.

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
#include "Test_ASWUnitTestsBCB_StdOutRedirect.h"
//---------------------------------------------------------------------------
#include <iostream>
#include <string>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_StdOutRedirect.h"
//---------------------------------------------------------------------------
using namespace System;
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_StdOutRedirect
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_StdOutRedirect::TTest_ASWUnitTestsBCB_StdOutRedirect()
    : inherited("ASWUnitTestsBCB_StdOutRedirect_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_StdOutRedirect::Test_Capture_ResumesCapturingWithFreshBuffer()
{
    // Arrange: 'outer' catches whatever escapes while 'inner' is restored below.
    TStdOutRedirect outer;
    TStdOutRedirect inner;

    // Act
    std::cout << "first-capture";
    std::string const firstCaptured = inner.Str();
    inner.Restore();
    std::cout << "escapes-to-outer";
    inner.Capture();
    std::cout << "second-capture";
    std::string const secondCaptured = inner.Str();
    inner.Capture(); // Already capturing: does nothing, rather than clearing "second-capture"
    std::string const afterRedundantCapture = inner.Str();

    // Assert
    CheckEquals("first-capture", firstCaptured, ASWBCB_HERE, "the first capture's content");
    CheckEquals("second-capture", secondCaptured, ASWBCB_HERE,
        "Capture() after Restore() starts a fresh buffer, discarding the first capture's content");
    CheckEquals("second-capture", afterRedundantCapture, ASWBCB_HERE,
        "Capture() while already capturing doesn't clear the buffer");
    CheckEquals("escapes-to-outer", outer.Str(), ASWBCB_HERE,
        "what was written between Restore() and Capture() reached the stream before");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_StdOutRedirect::Test_Restore_StopsCapturingAndIsIdempotent()
{
    // Arrange: 'outer' catches whatever escapes once 'inner' restores below, so nothing reaches the console.
    TStdOutRedirect outer;
    std::string innerCaptured;

    // Act
    {
        TStdOutRedirect inner;
        std::cout << "captured-by-inner";
        inner.Restore();
        std::cout << "not-captured-by-inner;";
        inner.Restore(); // A second call does nothing
        innerCaptured = inner.Str();
    } // The destructor's Restore() does nothing either

    std::cout << "after-inner-destroyed";
    outer.Restore();

    // Assert
    CheckEquals("captured-by-inner", innerCaptured, ASWBCB_HERE, "only what was written before Restore()");
    CheckEquals("not-captured-by-inner;after-inner-destroyed", outer.Str(), ASWBCB_HERE,
        "what was written after Restore(), and after the destructor, reached the stream before (and Str() keeps "
        "the capture Restore() ended)");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_StdOutRedirect::Test_Str_CapturesWrittenOutput()
{
    // Arrange
    TStdOutRedirect redirect;

    // Act
    std::cout << "hello " << 42 << std::endl;

    // Assert
    CheckEquals("hello 42\n", redirect.Str(), ASWBCB_HERE, "exactly what was written");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_StdOutRedirect, "SelfTests")
