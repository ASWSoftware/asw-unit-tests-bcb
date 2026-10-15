/* **************************************************************************
Test_ASWUnitTestsBCB_Version.cpp
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_Version.h.

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
#include "Test_ASWUnitTestsBCB_Version.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <string>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Handler.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Utils.h"
#include "ASWUnitTestsBCB_Version.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_Version
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_Version::TTest_ASWUnitTestsBCB_Version()
    : inherited("ASWUnitTestsBCB_Version_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Version::Test_GetVersionStr_ReturnsVersion()
{
    CheckEquals(ASWUNITTESTSBCB_VERSION_STRING, TTestHandler::GetVersionStr(), ASWBCB_HERE);
    CheckEquals("ASWUnitTestsBCB - Version " ASWUNITTESTSBCB_VERSION_STRING, TTestHandler::GetVersionFullStr(),
        ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Version::Test_PreRelease_IsValidSemVer()
{
    // Semantic Versioning: dot-separated identifiers of [0-9A-Za-z-], none empty (https://semver.org, item 9).
    std::string const preRelease = ASWUNITTESTSBCB_VERSION_PRERELEASE;

    if (preRelease.empty())
        return; // A release

    bool valid = (preRelease[0] != '.') && (preRelease[preRelease.size() - 1] != '.');

    for (std::size_t i = 0; i < preRelease.size(); ++i)
    {
        char const c = preRelease[i];
        bool const allowed = ((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) ||
            (c == '-') || (c == '.');

        valid = valid && allowed && !((i > 0) && (c == '.') && (preRelease[i - 1] == '.'));
    }

    CheckTrue(valid, ASWBCB_HERE, "pre-release \"" + preRelease + "\"");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_Version::Test_VersionString_MatchesParts()
{
    std::string const preRelease = ASWUNITTESTSBCB_VERSION_PRERELEASE;
    std::string const expected = IntToString(ASWUNITTESTSBCB_VERSION_MAJOR) + "." +
        IntToString(ASWUNITTESTSBCB_VERSION_MINOR) + "." + IntToString(ASWUNITTESTSBCB_VERSION_PATCH) +
        (preRelease.empty() ? std::string() : ("-" + preRelease));

    CheckEquals(expected, ASWUNITTESTSBCB_VERSION_STRING, ASWBCB_HERE);
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_Version, "SelfTests")
