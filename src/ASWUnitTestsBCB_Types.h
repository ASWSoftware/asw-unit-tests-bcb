/* **************************************************************************
ASWUnitTestsBCB_Types.h
Author: Anthony S. West - ASW Software

Basic types and definitions the framework uses everywhere, such as nullptr
for the classic compiler.

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

#ifndef ASWUnitTestsBCB_TypesH
#define ASWUnitTestsBCB_TypesH
//---------------------------------------------------------------------------
#include <cstddef>
//---------------------------------------------------------------------------

// nullptr for bcc32 (the classic compiler), which predates C++11 and doesn't know it: there it's NULL, so code written
// with nullptr builds with every compiler. It's then the integer constant 0, which converts to any pointer type
// (closures included) as nullptr does, but also to an integer: don't overload on std::nullptr_t, or count on nullptr
// not matching an integer parameter.
#if defined(__BORLANDC__) && !defined(__clang__) && !defined(nullptr)
#define nullptr NULL
#endif

//---------------------------------------------------------------------------
#endif // #ifndef ASWUnitTestsBCB_TypesH
