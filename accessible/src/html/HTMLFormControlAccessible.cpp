




#include "HTMLFormControlAccessible.h"

#include "Accessible-inl.h"
#include "nsAccUtils.h"
#include "nsEventShell.h"
#include "nsTextEquivUtils.h"
#include "Relation.h"
#include "Role.h"
#include "States.h"
#include "TreeWalker.h"

#include "nsContentList.h"
#include "nsCxPusher.h"
#include "mozilla/dom/HTMLInputElement.h"
#include "nsIAccessibleRelation.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIEditor.h"
#include "nsIFormControl.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsISelectionController.h"
#include "jsapi.h"
#include "nsIServiceManager.h"
#include "nsITextControlFrame.h"

#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::dom;
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

uint8_t
HTMLCheckboxAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
HTMLCheckboxAccessible::GetActionName(uint8_t aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {    
    
    uint64_t state = NativeState();

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
HTMLCheckboxAccessible::DoAction(uint8_t aIndex)
{
  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}

uint64_t
HTMLCheckboxAccessible::NativeState()
{
  uint64_t state = LeafAccessible::NativeState();

  state |= states::CHECKABLE;
  HTMLInputElement* input = HTMLInputElement::FromContent(mContent);
  if (!input)
    return state;

  if (input->Indeterminate())
    return state | states::MIXED;

  if (input->Checked())
    return state | states::CHECKED;
           
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

uint64_t
HTMLRadioButtonAccessible::NativeState()
{
  uint64_t state = AccessibleWrap::NativeState();

  state |= states::CHECKABLE;

  HTMLInputElement* input = HTMLInputElement::FromContent(mContent);
  if (input && input->Checked())
    state |= states::CHECKED;

  return state;
}

void
HTMLRadioButtonAccessible::GetPositionAndSizeInternal(int32_t* aPosInSet,
                                                      int32_t* aSetSize)
{
  int32_t namespaceId = mContent->NodeInfo()->NamespaceID();
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
  NS_ENSURE_TRUE_VOID(inputElms);

  uint32_t inputCount = inputElms->Length(false);

  
  int32_t indexOf = 0;
  int32_t count = 0;

  for (uint32_t index = 0; index < inputCount; index++) {
    nsIContent* inputElm = inputElms->Item(index, false);
    if (inputElm->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                              type, eCaseMatters) &&
        inputElm->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                              name, eCaseMatters) && mDoc->HasAccessible(inputElm)) {
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

uint8_t
HTMLButtonAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
HTMLButtonAccessible::GetActionName(uint8_t aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("press"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
HTMLButtonAccessible::DoAction(uint8_t aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}

uint64_t
HTMLButtonAccessible::State()
{
  uint64_t state = HyperTextAccessibleWrap::State();
  if (state == states::DEFUNCT)
    return state;

  
  
  
  if (mParent && mParent->IsHTMLFileInput()) {
    uint64_t parentState = mParent->State();
    state |= parentState & (states::BUSY | states::REQUIRED |
                            states::HASPOPUP | states::INVALID);
  }

  return state;
}

uint64_t
HTMLButtonAccessible::NativeState()
{
  uint64_t state = HyperTextAccessibleWrap::NativeState();

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

ENameValueFlag
HTMLButtonAccessible::NativeName(nsString& aName)
{
  
  
  
  
  
  
  
  

  ENameValueFlag nameFlag = Accessible::NativeName(aName);
  if (!aName.IsEmpty() || mContent->Tag() != nsGkAtoms::input ||
      !mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                             nsGkAtoms::image, eCaseMatters))
    return nameFlag;

  if (!mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::alt, aName))
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, aName);

  aName.CompressWhitespace();
  return eNameOK;
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
  mType = eHTMLTextFieldType;
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

ENameValueFlag
HTMLTextFieldAccessible::NativeName(nsString& aName)
{
  ENameValueFlag nameFlag = Accessible::NativeName(aName);
  if (!aName.IsEmpty())
    return nameFlag;

  if (mContent->GetBindingParent()) {
    
    
    
    
    
    Accessible* parent = Parent();
    if (parent)
      parent->GetName(aName);
  }

  if (!aName.IsEmpty())
    return eNameOK;

  
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::placeholder, aName);
  return eNameOK;
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

  HTMLInputElement* input = HTMLInputElement::FromContent(mContent);
  if (input)
    input->GetValue(aValue);
}

void
HTMLTextFieldAccessible::ApplyARIAState(uint64_t* aState) const
{
  HyperTextAccessibleWrap::ApplyARIAState(aState);

  aria::MapToState(aria::eARIAAutoComplete, mContent->AsElement(), aState);
}

uint64_t
HTMLTextFieldAccessible::NativeState()
{
  uint64_t state = HyperTextAccessibleWrap::NativeState();

  
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                            nsGkAtoms::password, eIgnoreCase)) {
    state |= states::PROTECTED;
  }

  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::readonly)) {
    state |= states::READONLY;
  }

  
  HTMLInputElement* input = HTMLInputElement::FromContent(mContent);
  state |= input && input->IsSingleLineTextControl() ?
    states::SINGLE_LINE : states::MULTI_LINE;

  if (!(state & states::EDITABLE) ||
      (state & (states::PROTECTED | states::MULTI_LINE)))
    return state;

  
  Accessible* widget = ContainerWidget();
  if (widget && widget-IsAutoComplete()) {
    state |= states::HASPOPUP | states::SUPPORTS_AUTOCOMPLETION;
    return state;
  }

  
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::list))
    return state | states::SUPPORTS_AUTOCOMPLETION | states::HASPOPUP;

  
  
  if (mParent && Preferences::GetBool("browser.formfill.enable")) {
    
    
    
    
    
    nsAutoString autocomplete;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::autocomplete,
                      autocomplete);

    if (!autocomplete.LowerCaseEqualsLiteral("off")) {
      nsIContent* formContent = input->GetFormElement();
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

uint8_t
HTMLTextFieldAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
HTMLTextFieldAccessible::GetActionName(uint8_t aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("activate");
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
HTMLTextFieldAccessible::DoAction(uint8_t aIndex)
{
  if (aIndex == 0)
    return TakeFocus();

  return NS_ERROR_INVALID_ARG;
}

already_AddRefed<nsIEditor>
HTMLTextFieldAccessible::GetEditor() const
{
  nsCOMPtr<nsIDOMNSEditableElement> editableElt(do_QueryInterface(mContent));
  if (!editableElt)
    return nullptr;

  
  
  
  nsCxPusher pusher;
  pusher.PushNull();

  nsCOMPtr<nsIEditor> editor;
  editableElt->GetEditor(getter_AddRefs(editor));

  return editor.forget();
}

void
HTMLTextFieldAccessible::CacheChildren()
{
  
  
  
  TreeWalker walker(this, mContent);
  Accessible* child = nullptr;
  while ((child = walker.NextChild())) {
    if (child->IsTextLeaf())
      AppendChild(child);
    else
      Document()->UnbindFromDocument(child);
  }
}




bool
HTMLTextFieldAccessible::IsWidget() const
{
  return true;
}

Accessible*
HTMLTextFieldAccessible::ContainerWidget() const
{
  return mParent && mParent->Role() == roles::AUTOCOMPLETE ? mParent : nullptr;
}






HTMLFileInputAccessible::
HTMLFileInputAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
  mType = eHTMLFileInputType;
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
    Accessible* button = GetChildAt(0);
    if (button && button->Role() == roles::PUSHBUTTON) {
      nsRefPtr<AccStateChangeEvent> childEvent =
        new AccStateChangeEvent(button, event->GetState(),
                                event->IsStateEnabled(),
                                (event->IsFromUserInput() ? eFromUserInput
                                                          : eNoUserInput));
      nsEventShell::FireEvent(childEvent);
    }
  }

  return NS_OK;
}






NS_IMPL_ISUPPORTS_INHERITED1(HTMLRangeAccessible, LeafAccessible,
                             nsIAccessibleValue)

role
HTMLRangeAccessible::NativeRole()
{
  return roles::SLIDER;
}

bool
HTMLRangeAccessible::IsWidget() const
{
  return true;
}

void
HTMLRangeAccessible::Value(nsString& aValue)
{
  LeafAccessible::Value(aValue);
  if (!aValue.IsEmpty())
    return;

  HTMLInputElement::FromContent(mContent)->GetValue(aValue);
}

NS_IMETHODIMP
HTMLRangeAccessible::GetMaximumValue(double* aMaximumValue)
{
  nsresult rv = LeafAccessible::GetMaximumValue(aMaximumValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aMaximumValue = HTMLInputElement::FromContent(mContent)->GetMaximum().toDouble();
  return NS_OK;
}


NS_IMETHODIMP
HTMLRangeAccessible::GetMinimumValue(double* aMinimumValue)
{
  nsresult rv = LeafAccessible::GetMinimumValue(aMinimumValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aMinimumValue = HTMLInputElement::FromContent(mContent)->GetMinimum().toDouble();
  return NS_OK;
}


NS_IMETHODIMP
HTMLRangeAccessible::GetMinimumIncrement(double* aMinimumIncrement)
{
  nsresult rv = LeafAccessible::GetMinimumIncrement(aMinimumIncrement);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aMinimumIncrement = HTMLInputElement::FromContent(mContent)->GetStep().toDouble();
  return NS_OK;
}

NS_IMETHODIMP
HTMLRangeAccessible::GetCurrentValue(double* aCurrentValue)
{
  nsresult rv = LeafAccessible::GetCurrentValue(aCurrentValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aCurrentValue = HTMLInputElement::FromContent(mContent)->GetValueAsDecimal().toDouble();
  return NS_OK;
}

NS_IMETHODIMP
HTMLRangeAccessible::SetCurrentValue(double aValue)
{
  ErrorResult er;
  HTMLInputElement::FromContent(mContent)->SetValueAsNumber(aValue, er);
  return er.ErrorCode();
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

  return nullptr;
}

ENameValueFlag
HTMLGroupboxAccessible::NativeName(nsString& aName)
{
  ENameValueFlag nameFlag = Accessible::NativeName(aName);
  if (!aName.IsEmpty())
    return nameFlag;

  nsIContent* legendContent = GetLegend();
  if (legendContent)
    nsTextEquivUtils::AppendTextEquivFromContent(this, legendContent, &aName);

  return eNameOK;
}

Relation
HTMLGroupboxAccessible::RelationByType(uint32_t aType)
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
HTMLLegendAccessible::RelationByType(uint32_t aType)
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

already_AddRefed<nsIPersistentProperties>
HTMLFigureAccessible::NativeAttributes()
{
  nsCOMPtr<nsIPersistentProperties> attributes =
    HyperTextAccessibleWrap::NativeAttributes();

  
  nsAccUtils::SetAccAttr(attributes, nsGkAtoms::xmlroles,
                         NS_LITERAL_STRING("figure"));
  return attributes.forget();
}

role
HTMLFigureAccessible::NativeRole()
{
  return roles::FIGURE;
}

ENameValueFlag
HTMLFigureAccessible::NativeName(nsString& aName)
{
  ENameValueFlag nameFlag = HyperTextAccessibleWrap::NativeName(aName);
  if (!aName.IsEmpty())
    return nameFlag;

  nsIContent* captionContent = Caption();
  if (captionContent)
    nsTextEquivUtils::AppendTextEquivFromContent(this, captionContent, &aName);

  return eNameOK;
}

Relation
HTMLFigureAccessible::RelationByType(uint32_t aType)
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

  return nullptr;
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
HTMLFigcaptionAccessible::RelationByType(uint32_t aType)
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
