/* **************************************************************************
Test_ASWUnitTestsBCB_Values.h
Author: Anthony S. West - ASW Software

Tests for ASWUnitTestsBCB_Values.h: how assertions compare and describe values.

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
#ifndef Test_ASWUnitTestsBCB_ValuesH
#define Test_ASWUnitTestsBCB_ValuesH
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

/////////////////////////////////////////////////////////////////////////////
// TTest_ASWUnitTestsBCB_Values
/////////////////////////////////////////////////////////////////////////////
class TTest_ASWUnitTestsBCB_Values : public TTestGroupBase
{
private:
    typedef TTestGroupBase inherited;

public:
    __fastcall TTest_ASWUnitTestsBCB_Values();

__published:
    void __fastcall Test_Bool();
    void __fastcall Test_ClassNames();
    void __fastcall Test_Emptiness();
    void __fastcall Test_Enums();
    void __fastcall Test_Enums_Scoped();
    void __fastcall Test_FloatingPoint_Fixed();
    void __fastcall Test_Integers_MixedTypes();
    void __fastcall Test_Integers_SameTypePairs();
    void __fastcall Test_Integers_Signedness();
    void __fastcall Test_MemoryFailure();
    void __fastcall Test_NarrowStrings();
    void __fastcall Test_NarrowStrings_Null();
    void __fastcall Test_Null();
    void __fastcall Test_Ordering_Compare();
    void __fastcall Test_Ordering_Describe();
    void __fastcall Test_Ordering_Holds();
    void __fastcall Test_Pointers();
    void __fastcall Test_RegularExpressions();
    void __fastcall Test_RTLStrings_AndMixes();
    void __fastcall Test_Same();
    void __fastcall Test_Texts_HasPart();
    void __fastcall Test_Texts_IgnoringASCIICase();
    void __fastcall Test_TypeCheckFailure();
    void __fastcall Test_Unicode_AcrossTypes();
    void __fastcall Test_WideStrings();
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------
#endif // #ifndef Test_ASWUnitTestsBCB_ValuesH
