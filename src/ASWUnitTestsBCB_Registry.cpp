/* **************************************************************************
ASWUnitTestsBCB_Registry.cpp
Author: Anthony S. West - ASW Software

Self-registration of test groups.

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
#include "ASWUnitTestsBCB_Registry.h"
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------
using namespace System;
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

namespace
{

// The most recently constructed registrar. A namespace-scope pointer without an initializer is zero-initialized
// before any constructor runs, so it's nullptr until the first registrar sets it, whichever translation unit's
// registrars are constructed first.
TTestGroupRegistrar const* g_LastRegistrar;

} // namespace

//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// TTestGroupRegistrar
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
TTestGroupRegistrar::TTestGroupRegistrar(TTestGroupFactory factory, TClass groupClass,
    char const* groupClassName, char const* suitePath, int order)
    : Factory(factory),
      GroupClass(groupClass),
      GroupClassName(groupClassName),
      Order(order),
      Next(g_LastRegistrar),
      SuitePath(suitePath)
{
    g_LastRegistrar = this;
}
//---------------------------------------------------------------------------
TTestGroupRegistrar::TTestGroupRegistrar(TTestGroupFactory factory, TClass groupClass,
    char const* groupClassName, char const* suitePath, int order, TTestGroupRegistrar const* next)
    : Factory(factory),
      GroupClass(groupClass),
      GroupClassName(groupClassName),
      Order(order),
      Next(next),
      SuitePath(suitePath)
{
}
//---------------------------------------------------------------------------
TTestGroupRegistrar const* TTestGroupRegistrar::First()
{
    return g_LastRegistrar;
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
