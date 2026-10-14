/* **************************************************************************
ASWUnitTestsBCB_StdOutRedirect.cpp
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
#include "ASWUnitTestsBCB_StdOutRedirect.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <iostream>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------
using namespace System;
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TStdOutRedirect
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TStdOutRedirect::TStdOutRedirect()
    : m_OriginalBuffer(nullptr)
{
    Capture();
}
//---------------------------------------------------------------------------
TStdOutRedirect::~TStdOutRedirect()
{
    Restore();
}
//---------------------------------------------------------------------------
void TStdOutRedirect::Capture()
{
    if (m_OriginalBuffer != nullptr)
        return;

    m_Captured.str("");
    m_Captured.clear();
    m_OriginalBuffer = std::cout.rdbuf(m_Captured.rdbuf());
}
//---------------------------------------------------------------------------
void TStdOutRedirect::Restore()
{
    if (m_OriginalBuffer == nullptr)
        return;

    std::cout.rdbuf(m_OriginalBuffer);
    m_OriginalBuffer = nullptr;
}
//---------------------------------------------------------------------------
std::string TStdOutRedirect::Str() const
{
    return m_Captured.str();
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
