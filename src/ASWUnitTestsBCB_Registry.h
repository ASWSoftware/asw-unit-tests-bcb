/* **************************************************************************
ASWUnitTestsBCB_Registry.h
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

#ifndef ASWUnitTestsBCB_RegistryH
#define ASWUnitTestsBCB_RegistryH
//---------------------------------------------------------------------------
#include <System.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_TestBase.h"
#include "ASWUnitTestsBCB_Types.h"
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

// Creates a test group with new; the caller owns it.
typedef TTestGroupBase* (*TTestGroupFactory)();

// The factory ASWBCB_REGISTER_TEST_GROUP registers for 'TGroup'.
template <class TGroup>
TTestGroupBase* CreateTestGroup()
{
    return new TGroup();
}


/////////////////////////////////////////////////////////////////////////////
// TTestGroupRegistrar
//
// One self-registered test group class (see ASWBCB_REGISTER_TEST_GROUP),
// with its factory, the suite its registration names (if any), and its
// ordering hint. Groups left at the default order of 0 run in alphabetical
// order by group name; an explicit lower/higher order runs a group
// before/after that block, with ties broken alphabetically. For a group in
// a suite, the order only places it among that suite's groups and suites
// (see TTestHandler::CanonicalGroupOrder()).
//
// Each registrar is a static object that adds itself to a linked list when
// it's constructed. The list's head is a plain pointer, set to nullptr before
// any constructor runs, so registration works whatever order the
// translation units are initialized in.
/////////////////////////////////////////////////////////////////////////////
class TTestGroupRegistrar
{
private:
    TTestGroupRegistrar(TTestGroupRegistrar const&);            // Not copyable
    TTestGroupRegistrar& operator=(TTestGroupRegistrar const&); // Not copyable

public:
    TTestGroupFactory const Factory;
    System::TClass const GroupClass;
    char const* const GroupClassName; // As written in the registration, e.g. "TTest_TMyClass"
    int const Order;
    TTestGroupRegistrar const* const Next;
    // The suite path the registration names, e.g. "Framework.Console" (see TSuitePath); "" when it names none.
    // Checked by TTestHandler::Initialize().
    char const* const SuitePath;

public:
    // The first registered group, or nullptr when there's none; follow Next for the rest, in no particular order.
    static TTestGroupRegistrar const* First();

public:
    // Registers a group (see ASWBCB_REGISTER_TEST_GROUP), adding it to the list First() starts. 'suitePath' is ""
    // for a registration that names no suite.
    TTestGroupRegistrar(TTestGroupFactory factory, System::TClass groupClass, char const* groupClassName,
        char const* suitePath, int order);
    // Makes an unregistered list of groups instead, with this one ahead of 'next' (nullptr for the last), for
    // TTestHandler::Initialize() to run only those groups (e.g. in a test of the handler itself).
    TTestGroupRegistrar(TTestGroupFactory factory, System::TClass groupClass, char const* groupClassName,
        char const* suitePath, int order, TTestGroupRegistrar const* next);
};

} // namespace ASWUnitTestsBCB

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// ASWBCB_REGISTER_TEST_GROUP / ASWBCB_REGISTER_TEST_GROUP_ORDERED
// ASWBCB_REGISTER_TEST_GROUP_IN_SUITE / ASWBCB_REGISTER_TEST_GROUP_IN_SUITE_ORDERED
//
// Place at namespace or file scope in a test group's .cpp file to register
// that group's class with TTestHandler, which creates the group with
// "new Class()" when the run starts. ASWBCB_REGISTER_TEST_GROUP runs the
// group in alphabetical order by group name, alongside every other group
// that doesn't specify an order. ASWBCB_REGISTER_TEST_GROUP_ORDERED
// overrides that with an explicit order: groups run in ascending order, with
// ties broken alphabetically by name. A group in a suite (see TSuitePath)
// is ordered among that suite's groups and suites only, and a suite sorts
// as order 0, by its own name. Use at most one per line.
//
// The _IN_SUITE forms also put the group in suite 'suitePath' (a string
// literal, e.g. "Framework.Console"), as passing a TSuitePath to the
// TTestGroupBase constructor does. Unlike the constructor's, the
// registration's suite also holds the stand-in that fails each of the
// group's tests if its constructor throws. A group whose constructor names
// a different suite stops the run at startup.
//
// Examples:
//     ASWBCB_REGISTER_TEST_GROUP(ASWUnitTestsBCB::TTest_TMyClass)
//     ASWBCB_REGISTER_TEST_GROUP_ORDERED(ASWUnitTestsBCB::TTest_TMyClass, -1) // Runs first
//     ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ASWUnitTestsBCB::TTest_TMyClass, "Framework.Console")
//
// Nothing else refers to a test group's .cpp file, so a linker could leave
// it out when the test code is built into a static library; link the test
// units' object files into the test program itself.
//---------------------------------------------------------------------------
#define ASWUnitTestsBCB_CONCAT_INNER(a, b) a ## b
#define ASWUnitTestsBCB_CONCAT(a, b) ASWUnitTestsBCB_CONCAT_INNER(a, b)

// __LINE__, since bcc32 has no __COUNTER__.
#define ASWUnitTestsBCB_UNIQUE_NAME(base) ASWUnitTestsBCB_CONCAT(base, __LINE__)

#define ASWBCB_REGISTER_TEST_GROUP_IN_SUITE_ORDERED(ClassName, suitePath, order) \
    static ::ASWUnitTestsBCB::TTestGroupRegistrar ASWUnitTestsBCB_UNIQUE_NAME(g_ASWUnitTestsBCB_Registrar_)( \
        ::ASWUnitTestsBCB::CreateTestGroup<ClassName>, __classid(ClassName), #ClassName, (suitePath), (order));

#define ASWBCB_REGISTER_TEST_GROUP_IN_SUITE(ClassName, suitePath) \
    ASWBCB_REGISTER_TEST_GROUP_IN_SUITE_ORDERED(ClassName, suitePath, 0)

#define ASWBCB_REGISTER_TEST_GROUP_ORDERED(ClassName, order) \
    ASWBCB_REGISTER_TEST_GROUP_IN_SUITE_ORDERED(ClassName, "", order)

#define ASWBCB_REGISTER_TEST_GROUP(ClassName) \
    ASWBCB_REGISTER_TEST_GROUP_IN_SUITE_ORDERED(ClassName, "", 0)

//---------------------------------------------------------------------------

#endif // #ifndef ASWUnitTestsBCB_RegistryH
