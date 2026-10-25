/* **************************************************************************
Test_ASWUnitTestsBCB_GUI_Shuffle.cpp
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
#include "Test_ASWUnitTestsBCB_GUI_Shuffle.h"
//---------------------------------------------------------------------------
#include <cstddef>
#include <memory>
#include <string>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_GUI_Shuffle.h"
#include "ASWUnitTestsBCB_Registry.h"
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
//---------------------------------------------------------------------------

namespace
{

/////////////////////////////////////////////////////////////////////////////
// TSeedSequence
//
// New seeds for a TGUIShuffle: 'first', then 'second' from then on,
// counting the calls.
/////////////////////////////////////////////////////////////////////////////
class TSeedSequence : public TObject
{
private:
    unsigned int const m_First;
    unsigned int const m_Second;

public:
    std::size_t CallCount;

public:
    __fastcall TSeedSequence(unsigned int first, unsigned int second)
        : m_First(first),
          m_Second(second),
          CallCount(0)
    {
    }

    unsigned int __fastcall Next()
    {
        ++CallCount;
        return (CallCount == 1) ? m_First : m_Second;
    }
};

} // namespace

//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_GUI_Shuffle
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
__fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::TTest_ASWUnitTestsBCB_GUI_Shuffle()
    : inherited("ASWUnitTestsBCB_GUI_Shuffle_Tests")
{
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::Test_ApplyCommandLine_SeedTurnsShufflingOn()
{
    // Arrange
    TGUIShuffle defaults;
    TGUIShuffle shuffleOnly;
    TGUIShuffle seedOnly;

    // Act
    shuffleOnly.ApplyCommandLine(true, TOptional<unsigned int>());
    seedOnly.ApplyCommandLine(false, 42u);

    // Assert
    CheckFalse(defaults.Enabled(), ASWBCB_HERE, "shuffling starts out off");
    CheckTrue(shuffleOnly.Enabled(), ASWBCB_HERE, "--shuffle turns it on");
    CheckFalse(shuffleOnly.FixedSeed().HasValue(), ASWBCB_HERE, "with a new seed for each run");
    CheckTrue(seedOnly.Enabled(), ASWBCB_HERE, "--shuffle-seed turns it on too");
    CheckTrue(seedOnly.FixedSeed().HasValue(), ASWBCB_HERE, "with that seed for every run");
    CheckEquals(42u, seedOnly.FixedSeed().Value(), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::Test_ApplySeedChoice_FixedSeed()
{
    // Arrange
    TGUIShuffle shuffle;
    std::string error;

    // Act
    bool const applied = shuffle.ApplySeedChoice(false, " 12345\t", error);

    // Assert
    CheckTrue(applied, ASWBCB_HERE, "a valid seed, with spaces around it, is accepted");
    CheckEmpty(error, ASWBCB_HERE, "with no error");
    CheckTrue(shuffle.Enabled(), ASWBCB_HERE, "choosing a seed turns shuffling on");
    CheckTrue(shuffle.FixedSeed().HasValue(), ASWBCB_HERE, "and every run uses that seed");
    CheckEquals(12345u, shuffle.FixedSeed().Value(), ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::Test_ApplySeedChoice_NewSeedEachRun()
{
    // Arrange
    TGUIShuffle shuffle;
    shuffle.ApplyCommandLine(false, 42u);
    shuffle.SetEnabled(false);
    std::string error;

    // Act
    bool const applied = shuffle.ApplySeedChoice(true, "not a seed", error);

    // Assert
    CheckTrue(applied, ASWBCB_HERE, "the seed text isn't needed, so it isn't checked");
    CheckEmpty(error, ASWBCB_HERE, "with no error");
    CheckTrue(shuffle.Enabled(), ASWBCB_HERE, "shuffling is turned back on");
    CheckFalse(shuffle.FixedSeed().HasValue(), ASWBCB_HERE, "and the fixed seed is gone");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::Test_ApplySeedChoice_RejectsInvalidSeed()
{
    char const* const invalidSeeds[] = { "", "   ", "abc", "-1", "12 34", "4294967296" };

    for (std::size_t i = 0; i < sizeof(invalidSeeds) / sizeof(invalidSeeds[0]); ++i)
    {
        // Arrange
        std::string const seedText = invalidSeeds[i];
        TGUIShuffle shuffle;
        shuffle.ApplyCommandLine(false, 7u);
        shuffle.SetEnabled(false);
        std::string error;

        // Act
        bool const applied = shuffle.ApplySeedChoice(false, seedText, error);

        // Assert
        std::string const what = "\"" + seedText + "\"";
        CheckFalse(applied, ASWBCB_HERE, what + " is rejected");
        CheckEquals("The seed must be a whole number from 0 to 4294967295.", error, ASWBCB_HERE,
            what + " has an error saying why");
        CheckFalse(shuffle.Enabled(), ASWBCB_HERE, what + " leaves shuffling off");
        CheckEquals(7u, shuffle.FixedSeed().Value(), ASWBCB_HERE, what + " leaves the seed as it was");
    }
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::Test_NewSeed_DefaultsToNewShuffleSeed()
{
    // Arrange: without a seed source, new seeds come from NewShuffleSeed(), so they differ from run to run.
    TGUIShuffle shuffle;
    shuffle.SetEnabled(true);

    // Act
    unsigned int const first = shuffle.NextRunSeed().Value();
    bool varied = false;

    for (int i = 0; (i < 10) && !varied; ++i)
        varied = (shuffle.NextRunSeed().Value() != first);

    // Assert
    CheckTrue(varied, ASWBCB_HERE);
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::Test_NextRunSeed_FixedSeedRepeats()
{
    // Arrange
    std::unique_ptr<TSeedSequence> const sequence(new TSeedSequence(100u, 100u));
    TGUIShuffle shuffle(&sequence->Next);
    shuffle.ApplyCommandLine(true, 7u);

    // Act
    TOptional<unsigned int> const first = shuffle.NextRunSeed();
    TOptional<unsigned int> const second = shuffle.NextRunSeed();

    // Assert
    CheckEquals(7u, first.Value(), ASWBCB_HERE, "the first run uses the fixed seed");
    CheckEquals(7u, second.Value(), ASWBCB_HERE, "and so does the next");
    CheckEquals(7u, shuffle.LastSeed().Value(), ASWBCB_HERE, "which is the last seed");
    CheckEquals(0, sequence->CallCount, ASWBCB_HERE, "no new seed was needed");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::Test_NextRunSeed_NewSeedEachRun()
{
    // Arrange
    std::unique_ptr<TSeedSequence> const sequence(new TSeedSequence(10u, 20u));
    TGUIShuffle shuffle(&sequence->Next);
    shuffle.SetEnabled(true);

    // Act
    TOptional<unsigned int> const first = shuffle.NextRunSeed();
    TOptional<unsigned int> const second = shuffle.NextRunSeed();

    // Assert
    CheckTrue(first.HasValue() && second.HasValue(), ASWBCB_HERE);
    CheckEquals(10u, first.Value(), ASWBCB_HERE, "the first run gets a new seed");
    CheckEquals(20u, second.Value(), ASWBCB_HERE, "and the next run another");
    CheckEquals(20u, shuffle.LastSeed().Value(), ASWBCB_HERE, "the latest is kept, to repeat that run's order");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::Test_NextRunSeed_OffReturnsNothing()
{
    // Arrange
    std::unique_ptr<TSeedSequence> const sequence(new TSeedSequence(10u, 20u));
    TGUIShuffle shuffle(&sequence->Next);
    shuffle.SetEnabled(true);
    shuffle.NextRunSeed();
    shuffle.SetEnabled(false);

    // Act
    TOptional<unsigned int> const seed = shuffle.NextRunSeed();

    // Assert
    CheckFalse(seed.HasValue(), ASWBCB_HERE, "an unshuffled run has no seed");
    CheckEquals(10u, shuffle.LastSeed().Value(), ASWBCB_HERE, "and the last shuffled run's seed is kept");
    CheckEquals(1, sequence->CallCount, ASWBCB_HERE, "no new seed was taken for it");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::Test_SeedToShow_PrefersFixedThenLastSeed()
{
    // Arrange
    std::unique_ptr<TSeedSequence> const sequence(new TSeedSequence(10u, 20u));
    TGUIShuffle shuffle(&sequence->Next);

    // Act & Assert
    CheckEquals(10u, shuffle.SeedToShow(), ASWBCB_HERE, "with no seed yet, a new one");

    shuffle.SetEnabled(true);
    shuffle.NextRunSeed(); // Takes 20.
    CheckEquals(20u, shuffle.SeedToShow(), ASWBCB_HERE, "after a run, that run's seed");

    shuffle.ApplyCommandLine(true, 7u);
    CheckEquals(7u, shuffle.SeedToShow(), ASWBCB_HERE, "with a fixed seed, that seed");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::Test_SetEnabled_KeepsSeeds()
{
    // Arrange
    TGUIShuffle shuffle;
    shuffle.ApplyCommandLine(true, 7u);
    shuffle.NextRunSeed();

    // Act
    shuffle.SetEnabled(false);
    shuffle.SetEnabled(true);

    // Assert
    CheckEquals(7u, shuffle.FixedSeed().Value(), ASWBCB_HERE, "turning shuffling off and on keeps the fixed seed");
    CheckEquals(7u, shuffle.LastSeed().Value(), ASWBCB_HERE, "and the last seed");
}
//---------------------------------------------------------------------------
void __fastcall TTest_ASWUnitTestsBCB_GUI_Shuffle::Test_StatusText_DescribesState()
{
    // Arrange
    std::unique_ptr<TSeedSequence> const sequence(new TSeedSequence(10u, 10u));
    TGUIShuffle shuffle(&sequence->Next);

    // Act & Assert
    CheckEquals("Shuffle: off", shuffle.StatusText(), ASWBCB_HERE, "off");

    shuffle.SetEnabled(true);
    CheckEquals("Shuffle: random", shuffle.StatusText(), ASWBCB_HERE, "on, before any shuffled run");

    shuffle.NextRunSeed();
    CheckEquals("Shuffle: random (last 10)", shuffle.StatusText(), ASWBCB_HERE, "on, after a shuffled run");

    shuffle.ApplyCommandLine(true, 4294967295u);
    CheckEquals("Shuffle: seed 4294967295", shuffle.StatusText(), ASWBCB_HERE, "with a fixed seed");
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB

ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_ASWUnitTestsBCB_GUI_Shuffle, "SelfTests.GUI")
