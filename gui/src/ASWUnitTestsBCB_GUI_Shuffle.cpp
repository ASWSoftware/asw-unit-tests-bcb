/* **************************************************************************
ASWUnitTestsBCB_GUI_Shuffle.cpp
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
#include "ASWUnitTestsBCB_GUI_Shuffle.h"
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_CLI.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TGUIShuffle
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TGUIShuffle::TGUIShuffle(TSeedSource newSeed)
    : m_Enabled(false),
      m_NewSeed(newSeed)
{
}
//---------------------------------------------------------------------------
void TGUIShuffle::ApplyCommandLine(bool shuffle, TOptional<unsigned int> seed)
{
    m_Enabled = shuffle || seed.HasValue();
    m_FixedSeed = seed;
}
//---------------------------------------------------------------------------
bool TGUIShuffle::ApplySeedChoice(bool newSeedEachRun, std::string const& seedText, std::string& error)
{
    error.clear();
    TOptional<unsigned int> seed;

    if (!newSeedEachRun)
    {
        std::string::size_type const first = seedText.find_first_not_of(" \t");
        std::string::size_type const last = seedText.find_last_not_of(" \t");

        if (first != std::string::npos)
            seed = TCLIParser::ParseUnsignedInt(seedText.substr(first, last - first + 1));

        if (!seed.HasValue())
        {
            error = "The seed must be a whole number from 0 to 4294967295.";
            return false;
        }
    }

    m_Enabled = true;
    m_FixedSeed = seed;

    return true;
}
//---------------------------------------------------------------------------
bool TGUIShuffle::Enabled() const
{
    return m_Enabled;
}
//---------------------------------------------------------------------------
TOptional<unsigned int> TGUIShuffle::FixedSeed() const
{
    return m_FixedSeed;
}
//---------------------------------------------------------------------------
TOptional<unsigned int> TGUIShuffle::LastSeed() const
{
    return m_LastSeed;
}
//---------------------------------------------------------------------------
unsigned int TGUIShuffle::NewSeed() const
{
    return (m_NewSeed != nullptr) ? m_NewSeed() : NewShuffleSeed();
}
//---------------------------------------------------------------------------
TOptional<unsigned int> TGUIShuffle::NextRunSeed()
{
    if (!m_Enabled)
        return TOptional<unsigned int>();

    m_LastSeed = m_FixedSeed.HasValue() ? m_FixedSeed.Value() : NewSeed();

    return m_LastSeed;
}
//---------------------------------------------------------------------------
unsigned int TGUIShuffle::SeedToShow() const
{
    if (m_FixedSeed.HasValue())
        return m_FixedSeed.Value();

    if (m_LastSeed.HasValue())
        return m_LastSeed.Value();

    return NewSeed();
}
//---------------------------------------------------------------------------
void TGUIShuffle::SetEnabled(bool enabled)
{
    m_Enabled = enabled;
}
//---------------------------------------------------------------------------
std::string TGUIShuffle::StatusText() const
{
    if (!m_Enabled)
        return "Shuffle: off";

    if (m_FixedSeed.HasValue())
        return "Shuffle: seed " + UIntToString(m_FixedSeed.Value());

    if (m_LastSeed.HasValue())
        return "Shuffle: random (last " + UIntToString(m_LastSeed.Value()) + ")";

    return "Shuffle: random";
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
