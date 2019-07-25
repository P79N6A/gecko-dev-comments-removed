




#include "HTMLFormControlAccessible.h"

#include "Accessible-inl.h"
#include "nsAccUtils.h"
#include "nsARIAMap.h"
#include "nsTextEquivUtils.h"
#include "Relation.h"
#include "Role.h"
#include "States.h"

#include "nsContentList.h"
#include "nsIAccessibleRelation.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLLegendElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMNodeList.h"
#include "nsIEditor.h"
#include "nsIFormControl.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsISelectionController.h"
#include "jsapi.h"
#include "nsIJSContextStack.h"
#include "nsIServiceManager.h"
#include "nsITextControlFrame.h"

#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::a11y;





HTMLCheckboxAccessible::
  HTMLCheckboxAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  LeafAccessible(aContent, aDoc)
{
}

role
HTMLCheckboxAccessible::NativeRole()
{
  return roles::CHECKBUTTON;
}

PRUint8
HTMLCheckboxAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
HTMLCheckboxAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {    
    
    PRUint64 state = NativeState();

    if (state & states::CHECKED)
      aName.AssignLiteral("uncheck"); 
    else if (state & states::MIXED)
      aName.AssignLiteral("cycle"); 
    else
      aName.AssignLiteral("check"); 

    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
HTMLCheckboxAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}

PRUint64
HTMLCheckboxAccessible::NativeState()
{
  PRUint64 state = LeafAccessible::NativeState();

  state |= states::CHECKABLE;
  bool checkState = false;   

  nsCOMPtr<nsIDOMHTMLInputElement> htmlCheckboxElement =
    do_QueryInterface(mContent);
           
  if (htmlCheckboxElement) {
    htmlCheckboxElement->GetIndeterminate(&checkState);

    if (checkState) {
      state |= states::MIXED;
    } else {   
      htmlCheckboxElement->GetChecked(&checkState);
    
      if (checkState)
        state |= states::CHECKED;
    }
  }
  return state;
}




bool
HTMLCheckboxAccessible::IsWidget() const
{
  return true;
}






HTMLRadioButtonAccessible::
  HTMLRadioButtonAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  RadioButtonAccessible(aContent, aDoc)
{
}

PRUint64
HTMLRadioButtonAccessible::NativeState()
{
  PRUint64 state = AccessibleWrap::NativeState();

  state |= states::CHECKABLE;

  bool checked = false;   
  nsCOMPtr<nsIDOMHTMLInputElement> htmlRadioElement =
    do_QueryInterface(mContent);
  if (htmlRadioElement)
    htmlRadioElement->GetChecked(&checked);

  if (checked)
    state |= states::CHECKED;

  return state;
}

void
HTMLRadioButtonAccessible::GetPositionAndSizeInternal(PRInt32* aPosInSet,
                                                      PRInt32* aSetSize)
{
  PRInt32 namespaceId = mContent->NodeInfo()->NamespaceID();
  nsAutoString tagName;
  mContent->NodeInfo()->GetName(tagName);

  nsAutoString type;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::type, type);
  nsAutoString name;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);

  nsRefPtr<nsContentList> inputElms;

  nsCOMPtr<nsIFormControl> formControlNode(do_QueryInterface(mContent));
  dom::Element* formElm = formControlNode->GetFormElement();
  if (formElm)
    inputElms = NS_GetContentList(formElm, namespaceId, tagName);
  else
    inputElms = NS_GetContentList(mContent->OwnerDoc(), namespaceId, tagName);
  NS_ENSURE_TRUE(inputElms, );

  PRUint32 inputCount = inputElms->Length(false);

  
  PRInt32 indexOf = 0;
  PRInt32 count = 0;

  for (PRUint32 index = 0; index < inputCount; index++) {
    nsIContent* inputElm = inputElms->Item(index, false);
    if (inputElm->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                              type, eCaseMatters) &&
        inputElm->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                              name, eCaseMatters)) {
      count++;
      if (inputElm == mContent)
        indexOf = count;
    }
  }

  *aPosInSet = indexOf;
  *aSetSize = count;
}





HTMLButtonAccessible::
  HTMLButtonAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}

PRUint8
HTMLButtonAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
HTMLButtonAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("press"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
HTMLButtonAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}

PRUint64
HTMLButtonAccessible::State()
{
  PRUint64 state = HyperTextAccessibleWrap::State();
  if (state == states::DEFUNCT)
    return state;

  
  
  
  if (mParent && mParent->IsHTMLFileInput()) {
    PRUint64 parentState = mParent->State();
    state |= parentState & (states::BUSY | states::REQUIRED |
                            states::HASPOPUP | states::INVALID);
  }

  return state;
}

PRUint64
HTMLButtonAccessible::NativeState()
{
  PRUint64 state = HyperTextAccessibleWrap::NativeState();

  nsEventStates elmState = mContent->AsElement()->State();
  if (elmState.HasState(NS_EVENT_STATE_DEFAULT))
    state |= states::DEFAULT;

  return state;
}

role
HTMLButtonAccessible::NativeRole()
{
  return roles::PUSHBUTTON;
}

nsresult
HTMLButtonAccessible::GetNameInternal(nsAString& aName)
{
  Accessible::GetNameInternal(aName);
  if (!aName.IsEmpty() || mContent->Tag() != nsGkAtoms::input)
    return NS_OK;

  
  nsAutoString name;
  if (!mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value,
                         name) &&
      !mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::alt,
                         name)) {
    
    nsIFrame* frame = GetFrame();
    if (frame) {
      nsIFormControlFrame* fcFrame = do_QueryFrame(frame);
      if (fcFrame)
        fcFrame->GetFormProperty(nsGkAtoms::defaultLabel, name);
    }
  }

  if (name.IsEmpty() &&
      !mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::src, name)) {
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::data, name);
  }

  name.CompressWhitespace();
  aName = name;

  return NS_OK;
}




bool
HTMLButtonAccessible::IsWidget() const
{
  return true;
}






HTMLTextFieldAccessible::
  HTMLTextFieldAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED2(HTMLTextFieldAccessible,
                             Accessible,                             
                             nsIAccessibleText,
                             nsIAccessibleEditableText)

role
HTMLTextFieldAccessible::NativeRole()
{
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                            nsGkAtoms::password, eIgnoreCase)) {
    return roles::PASSWORD_TEXT;
  }
  
  return roles::ENTRY;
}

nsresult
HTMLTextFieldAccessible::GetNameInternal(nsAString& aName)
{
  nsresult rv = Accessible::GetNameInternal(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aName.IsEmpty())
    return NS_OK;

  if (mContent->GetBindingParent())
  {
    
    
    
    
    
    Accessible* parent = Parent();
    if (parent)
      parent->GetName(aName);
  }

  if (!aName.IsEmpty())
    return NS_OK;

  
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::placeholder, aName);

  return NS_OK;
}

void
HTMLTextFieldAccessible::Value(nsString& aValue)
{
  aValue.Truncate();
  if (NativeState() & states::PROTECTED)    
    return;

  nsCOMPtr<nsIDOMHTMLTextAreaElement> textArea(do_QueryInterface(mContent));
  if (textArea) {
    textArea->GetValue(aValue);
    return;
  }
  
  nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(mContent));
  if (inputElement) {
    inputElement->GetValue(aValue);
  }
}

void
HTMLTextFieldAccessible::ApplyARIAState(PRUint64* aState) const
{
  HyperTextAccessibleWrap::ApplyARIAState(aState);

  aria::MapToState(aria::eARIAAutoComplete, mContent->AsElement(), aState);
}

PRUint64
HTMLTextFieldAccessible::State()
{
  PRUint64 state = HyperTextAccessibleWrap::State();
  if (state & states::DEFUNCT)
    return state;

  
  
  
  if (mParent && mParent->IsHTMLFileInput()) {
    PRUint64 parentState = mParent->State();
    state |= parentState & (states::BUSY | states::REQUIRED |
      states::HASPOPUP | states::INVALID);
  }

  return state;
}

PRUint64
HTMLTextFieldAccessible::NativeState()
{
  PRUint64 state = HyperTextAccessibleWrap::NativeState();

  
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                            nsGkAtoms::password, eIgnoreCase)) {
    state |= states::PROTECTED;
  }

  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::readonly)) {
    state |= states::READONLY;
  }

  
  nsCOMPtr<nsIDOMHTMLInputElement> htmlInput(do_QueryInterface(mContent));
  state |= htmlInput ? states::SINGLE_LINE : states::MULTI_LINE;

  if (!(state & states::EDITABLE) ||
      (state & (states::PROTECTED | states::MULTI_LINE)))
    return state;

  
  Accessible* widget = ContainerWidget();
  if (widget && widget-IsAutoComplete()) {
    state |= states::HASPOPUP | states::SUPPORTS_AUTOCOMPLETION;
    return state;
  }

  
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::list))
    return state | states::SUPPORTS_AUTOCOMPLETION;

  
  
  if (mParent && Preferences::GetBool("browser.formfill.enable")) {
    
    
    
    
    
    nsAutoString autocomplete;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::autocomplete,
                      autocomplete);

    if (!autocomplete.LowerCaseEqualsLiteral("off")) {
      nsCOMPtr<nsIDOMHTMLFormElement> form;
      htmlInput->GetForm(getter_AddRefs(form));
      nsCOMPtr<nsIContent> formContent(do_QueryInterface(form));
      if (formContent) {
        formContent->GetAttr(kNameSpaceID_None,
                             nsGkAtoms::autocomplete, autocomplete);
      }

      if (!formContent || !autocomplete.LowerCaseEqualsLiteral("off"))
        state |= states::SUPPORTS_AUTOCOMPLETION;
    }
  }

  return state;
}

PRUint8
HTMLTextFieldAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
HTMLTextFieldAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("activate");
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
HTMLTextFieldAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex == 0) {
    nsCOMPtr<nsIDOMHTMLElement> element(do_QueryInterface(mContent));
    if (element)
      return element->Focus();

    return NS_ERROR_FAILURE;
  }
  return NS_ERROR_INVALID_ARG;
}

already_AddRefed<nsIEditor>
HTMLTextFieldAccessible::GetEditor() const
{
  nsCOMPtr<nsIDOMNSEditableElement> editableElt(do_QueryInterface(mContent));
  if (!editableElt)
    return nsnull;

  
  
  
  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  bool pushed = stack && NS_SUCCEEDED(stack->Push(nsnull));

  nsCOMPtr<nsIEditor> editor;
  editableElt->GetEditor(getter_AddRefs(editor));

  if (pushed) {
    JSContext* cx;
    stack->Pop(&cx);
    NS_ASSERTION(!cx, "context should be null");
  }

  return editor.forget();
}




bool
HTMLTextFieldAccessible::IsWidget() const
{
  return true;
}

Accessible*
HTMLTextFieldAccessible::ContainerWidget() const
{
  return mParent && mParent->Role() == roles::AUTOCOMPLETE ? mParent : nsnull;
}






HTMLFileInputAccessible::
HTMLFileInputAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
  mFlags |= eHTMLFileInputAccessible;
}

role
HTMLFileInputAccessible::NativeRole()
{
  
  
  return roles::TEXT_CONTAINER;
}

nsresult
HTMLFileInputAccessible::HandleAccEvent(AccEvent* aEvent)
{
  nsresult rv = HyperTextAccessibleWrap::HandleAccEvent(aEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  AccStateChangeEvent* event = downcast_accEvent(aEvent);
  if (event &&
      (event->GetState() == states::BUSY ||
       event->GetState() == states::REQUIRED ||
       event->GetState() == states::HASPOPUP ||
       event->GetState() == states::INVALID)) {
    Accessible* input = GetChildAt(0);
    if (input && input->Role() == roles::ENTRY) {
      nsRefPtr<AccStateChangeEvent> childEvent =
        new AccStateChangeEvent(input, event->GetState(),
                                event->IsStateEnabled(),
                                (event->IsFromUserInput() ? eFromUserInput : eNoUserInput));
      nsEventShell::FireEvent(childEvent);
    }

    Accessible* button = GetChildAt(1);
    if (button && button->Role() == roles::PUSHBUTTON) {
      nsRefPtr<AccStateChangeEvent> childEvent =
        new AccStateChangeEvent(button, event->GetState(),
                                event->IsStateEnabled(),
                                (event->IsFromUserInput() ? eFromUserInput : eNoUserInput));
      nsEventShell::FireEvent(childEvent);
    }
  }
  return NS_OK;
}





HTMLGroupboxAccessible::
  HTMLGroupboxAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}

role
HTMLGroupboxAccessible::NativeRole()
{
  return roles::GROUPING;
}

nsIContent*
HTMLGroupboxAccessible::GetLegend()
{
  for (nsIContent* legendContent = mContent->GetFirstChild(); legendContent;
       legendContent = legendContent->GetNextSibling()) {
    if (legendContent->NodeInfo()->Equals(nsGkAtoms::legend,
                                          mContent->GetNameSpaceID())) {
      
      return legendContent;
    }
  }

  return nsnull;
}

nsresult
HTMLGroupboxAccessible::GetNameInternal(nsAString& aName)
{
  nsresult rv = Accessible::GetNameInternal(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aName.IsEmpty())
    return NS_OK;

  nsIContent *legendContent = GetLegend();
  if (legendContent) {
    return nsTextEquivUtils::
      AppendTextEquivFromContent(this, legendContent, &aName);
  }

  return NS_OK;
}

Relation
HTMLGroupboxAccessible::RelationByType(PRUint32 aType)
{
  Relation rel = HyperTextAccessibleWrap::RelationByType(aType);
    
  if (aType == nsIAccessibleRelation::RELATION_LABELLED_BY)
    rel.AppendTarget(mDoc, GetLegend());

  return rel;
}





HTMLLegendAccessible::
  HTMLLegendAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}

Relation
HTMLLegendAccessible::RelationByType(PRUint32 aType)
{
  Relation rel = HyperTextAccessibleWrap::RelationByType(aType);
  if (aType != nsIAccessibleRelation::RELATION_LABEL_FOR)
    return rel;

  Accessible* groupbox = Parent();
  if (groupbox && groupbox->Role() == roles::GROUPING)
    rel.AppendTarget(groupbox);

  return rel;
}

role
HTMLLegendAccessible::NativeRole()
{
  return roles::LABEL;
}





HTMLFigureAccessible::
  HTMLFigureAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}

nsresult
HTMLFigureAccessible::GetAttributesInternal(nsIPersistentProperties* aAttributes)
{
  nsresult rv = HyperTextAccessibleWrap::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::xmlroles,
                         NS_LITERAL_STRING("figure"));
  return NS_OK;
}

role
HTMLFigureAccessible::NativeRole()
{
  return roles::FIGURE;
}

nsresult
HTMLFigureAccessible::GetNameInternal(nsAString& aName)
{
  nsresult rv = HyperTextAccessibleWrap::GetNameInternal(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aName.IsEmpty())
    return NS_OK;

  nsIContent* captionContent = Caption();
  if (captionContent) {
    return nsTextEquivUtils::
      AppendTextEquivFromContent(this, captionContent, &aName);
  }

  return NS_OK;
}

Relation
HTMLFigureAccessible::RelationByType(PRUint32 aType)
{
  Relation rel = HyperTextAccessibleWrap::RelationByType(aType);
  if (aType == nsIAccessibleRelation::RELATION_LABELLED_BY)
    rel.AppendTarget(mDoc, Caption());

  return rel;
}

nsIContent*
HTMLFigureAccessible::Caption() const
{
  for (nsIContent* childContent = mContent->GetFirstChild(); childContent;
       childContent = childContent->GetNextSibling()) {
    if (childContent->NodeInfo()->Equals(nsGkAtoms::figcaption,
                                         mContent->GetNameSpaceID())) {
      return childContent;
    }
  }

  return nsnull;
}





HTMLFigcaptionAccessible::
  HTMLFigcaptionAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}

role
HTMLFigcaptionAccessible::NativeRole()
{
  return roles::CAPTION;
}

Relation
HTMLFigcaptionAccessible::RelationByType(PRUint32 aType)
{
  Relation rel = HyperTextAccessibleWrap::RelationByType(aType);
  if (aType != nsIAccessibleRelation::RELATION_LABEL_FOR)
    return rel;

  Accessible* figure = Parent();
  if (figure &&
      figure->GetContent()->NodeInfo()->Equals(nsGkAtoms::figure,
                                               mContent->GetNameSpaceID())) {
    rel.AppendTarget(figure);
  }

  return rel;
}
