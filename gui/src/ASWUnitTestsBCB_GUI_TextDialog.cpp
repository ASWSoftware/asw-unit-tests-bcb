/* **************************************************************************
ASWUnitTestsBCB_GUI_TextDialog.cpp
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
#include "ASWUnitTestsBCB_GUI_TextDialog.h"
//---------------------------------------------------------------------------
#include <memory>
//---------------------------------------------------------------------------
#include <System.SysUtils.hpp>
#include <System.UITypes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.StdCtrls.hpp>
//---------------------------------------------------------------------------
#include "ASWUnitTestsBCB_Types.h"
#include "ASWUnitTestsBCB_Utils.h"
//---------------------------------------------------------------------------
using namespace System;
using namespace System::Sysutils;
using namespace System::Uitypes;
using namespace Vcl::Controls;
using namespace Vcl::Extctrls;
using namespace Vcl::Forms;
using namespace Vcl::Stdctrls;
//---------------------------------------------------------------------------

namespace ASWUnitTestsBCB
{

//---------------------------------------------------------------------------
void ShowTextDialog(UnicodeString const& caption, std::string const& text)
{
    // Built in code rather than from a .dfm, since it's this simple and is also shown before the main form exists
    // (e.g. for --help). CreateNew (the int overload) makes a form with no .dfm resource.
    std::unique_ptr<TForm> const form(new TForm(static_cast<System::Classes::TComponent*>(nullptr), 0));
    form->Caption = caption;
    form->BorderStyle = bsSizeable;
    form->BorderIcons = TBorderIcons() << biSystemMenu;
    form->Position = poScreenCenter;
    form->ClientWidth = form->ScaleValue(760);
    form->ClientHeight = form->ScaleValue(520);

    TPanel* const buttonPanel = new TPanel(form.get());
    buttonPanel->Parent = form.get();
    buttonPanel->Align = alBottom;
    buttonPanel->BevelOuter = bvNone;
    buttonPanel->Height = form->ScaleValue(44);

    TButton* const okButton = new TButton(form.get());
    okButton->Parent = buttonPanel;
    okButton->Caption = "OK";
    okButton->ModalResult = mrOk;
    okButton->Default = true;
    okButton->Cancel = true;
    okButton->Anchors = TAnchors() << akTop << akRight;
    okButton->SetBounds(buttonPanel->ClientWidth - form->ScaleValue(96), form->ScaleValue(9), form->ScaleValue(84),
        form->ScaleValue(26));

    TMemo* const memo = new TMemo(form.get());
    memo->Parent = form.get();
    memo->Align = alClient;
    memo->ReadOnly = true;
    memo->ScrollBars = ssBoth;
    memo->WordWrap = false;
    memo->Font->Name = "Consolas";
    memo->Font->Size = 10;
    memo->Text = AdjustLineBreaks(FromUTF8(text));

    form->ActiveControl = okButton;
    form->ShowModal();
}
//---------------------------------------------------------------------------

} // namespace ASWUnitTestsBCB
