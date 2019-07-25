





































#include "nsHTMLFormControlAccessible.h"

#include "States.h"
#include "nsAccessibilityAtoms.h"
#include "nsAccUtils.h"
#include "nsRelUtils.h"
#include "nsTextEquivUtils.h"

#include "nsIDOMDocument.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLLegendElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIEditor.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsISelectionController.h"
#include "jsapi.h"
#include "nsIJSContextStack.h"
#include "nsIServiceManager.h"
#include "nsITextControlFrame.h"





nsHTMLCheckboxAccessible::
  nsHTMLCheckboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsFormControlAccessible(aContent, aShell)
{
}

PRUint32
nsHTMLCheckboxAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_CHECKBUTTON;
}

NS_IMETHODIMP nsHTMLCheckboxAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLCheckboxAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
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
nsHTMLCheckboxAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}

PRUint64
nsHTMLCheckboxAccessible::NativeState()
{
  PRUint64 state = nsFormControlAccessible::NativeState();

  state |= states::CHECKABLE;
  PRBool checkState = PR_FALSE;   

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





nsHTMLRadioButtonAccessible::
  nsHTMLRadioButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsRadioButtonAccessible(aContent, aShell)
{
}

PRUint64
nsHTMLRadioButtonAccessible::NativeState()
{
  PRUint64 state = nsAccessibleWrap::NativeState();

  state |= states::CHECKABLE;
  
  PRBool checked = PR_FALSE;   

  nsCOMPtr<nsIDOMHTMLInputElement> htmlRadioElement =
    do_QueryInterface(mContent);
  if (htmlRadioElement)
    htmlRadioElement->GetChecked(&checked);

  if (checked)
    state |= states::CHECKED;

  return state;
}

void
nsHTMLRadioButtonAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                                        PRInt32 *aSetSize)
{
  nsAutoString nsURI;
  mContent->NodeInfo()->GetNamespaceURI(nsURI);
  nsAutoString tagName;
  mContent->NodeInfo()->GetName(tagName);

  nsAutoString type;
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::type, type);
  nsAutoString name;
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::name, name);

  nsCOMPtr<nsIDOMNodeList> inputs;

  nsCOMPtr<nsIDOMHTMLInputElement> radio(do_QueryInterface(mContent));
  nsCOMPtr<nsIDOMHTMLFormElement> form;
  radio->GetForm(getter_AddRefs(form));
  if (form) {
    form->GetElementsByTagNameNS(nsURI, tagName, getter_AddRefs(inputs));
  } else {
    nsIDocument* doc = mContent->GetOwnerDoc();
    nsCOMPtr<nsIDOMDocument> document(do_QueryInterface(doc));
    if (document)
      document->GetElementsByTagNameNS(nsURI, tagName, getter_AddRefs(inputs));
  }

  NS_ENSURE_TRUE(inputs, );

  PRUint32 inputsCount = 0;
  inputs->GetLength(&inputsCount);

  
  PRInt32 indexOf = 0;
  PRInt32 count = 0;

  for (PRUint32 index = 0; index < inputsCount; index++) {
    nsCOMPtr<nsIDOMNode> itemNode;
    inputs->Item(index, getter_AddRefs(itemNode));

    nsCOMPtr<nsIContent> item(do_QueryInterface(itemNode));
    if (item &&
        item->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                          type, eCaseMatters) &&
        item->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::name,
                          name, eCaseMatters)) {

      count++;

      if (item == mContent)
        indexOf = count;
    }
  }

  *aPosInSet = indexOf;
  *aSetSize = count;
}





nsHTMLButtonAccessible::
  nsHTMLButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}

NS_IMETHODIMP nsHTMLButtonAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLButtonAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("press"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsHTMLButtonAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}

PRUint64
nsHTMLButtonAccessible::NativeState()
{
  PRUint64 state = nsHyperTextAccessibleWrap::NativeState();

  if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                            nsAccessibilityAtoms::submit, eIgnoreCase))
    state |= states::DEFAULT;

  return state;
}

PRUint32
nsHTMLButtonAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PUSHBUTTON;
}

nsresult
nsHTMLButtonAccessible::GetNameInternal(nsAString& aName)
{
  nsAccessible::GetNameInternal(aName);
  if (!aName.IsEmpty())
    return NS_OK;

  
  nsAutoString name;
  if (!mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value,
                         name) &&
      !mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::alt,
                         name)) {
    
    nsIFrame* frame = GetFrame();
    if (frame) {
      nsIFormControlFrame* fcFrame = do_QueryFrame(frame);
      if (fcFrame)
        fcFrame->GetFormProperty(nsAccessibilityAtoms::defaultLabel, name);
    }
  }

  if (name.IsEmpty() &&
      !mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::src,
                         name)) {
    mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::data, name);
  }

  name.CompressWhitespace();
  aName = name;

  return NS_OK;
}






nsHTML4ButtonAccessible::
  nsHTML4ButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}

NS_IMETHODIMP nsHTML4ButtonAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;;
}

NS_IMETHODIMP nsHTML4ButtonAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("press"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsHTML4ButtonAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}

PRUint32
nsHTML4ButtonAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PUSHBUTTON;
}

PRUint64
nsHTML4ButtonAccessible::NativeState()
{
  PRUint64 state = nsHyperTextAccessibleWrap::NativeState();

  state |= states::FOCUSABLE;

  if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                            nsAccessibilityAtoms::submit, eIgnoreCase))
    state |= states::DEFAULT;

  return state;
}






nsHTMLTextFieldAccessible::
  nsHTMLTextFieldAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED3(nsHTMLTextFieldAccessible, nsAccessible, nsHyperTextAccessible, nsIAccessibleText, nsIAccessibleEditableText)

PRUint32
nsHTMLTextFieldAccessible::NativeRole()
{
  if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                            nsAccessibilityAtoms::password, eIgnoreCase)) {
    return nsIAccessibleRole::ROLE_PASSWORD_TEXT;
  }
  return nsIAccessibleRole::ROLE_ENTRY;
}

nsresult
nsHTMLTextFieldAccessible::GetNameInternal(nsAString& aName)
{
  nsresult rv = nsAccessible::GetNameInternal(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aName.IsEmpty())
    return NS_OK;

  if (mContent->GetBindingParent())
  {
    
    
    
    
    
    nsAccessible* parent = GetParent();
    parent->GetName(aName);
  }

  if (!aName.IsEmpty())
    return NS_OK;

  
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::placeholder, aName);

  return NS_OK;
}

NS_IMETHODIMP nsHTMLTextFieldAccessible::GetValue(nsAString& _retval)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (NativeState() & states::PROTECTED)    
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMHTMLTextAreaElement> textArea(do_QueryInterface(mContent));
  if (textArea) {
    return textArea->GetValue(_retval);
  }
  
  nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(mContent));
  if (inputElement) {
    return inputElement->GetValue(_retval);
  }

  return NS_ERROR_FAILURE;
}

PRUint64
nsHTMLTextFieldAccessible::NativeState()
{
  PRUint64 state = nsHyperTextAccessibleWrap::NativeState();

  
  if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                            nsAccessibilityAtoms::password, eIgnoreCase)) {
    state |= states::PROTECTED;
  }
  else {
    nsAccessible* parent = GetParent();
    if (parent && parent->Role() == nsIAccessibleRole::ROLE_AUTOCOMPLETE)
      state |= states::HASPOPUP;
  }

  if (mContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::readonly)) {
    state |= states::READONLY;
  }

  nsCOMPtr<nsIDOMHTMLInputElement> htmlInput(do_QueryInterface(mContent));
  
  if (htmlInput) {
    state |= states::SINGLE_LINE;
  }
  else {
    state |= states::MULTI_LINE;
  }

  if (!(state & states::EDITABLE))
    return state;

  nsCOMPtr<nsIContent> bindingContent = mContent->GetBindingParent();
  if (bindingContent &&
      bindingContent->NodeInfo()->Equals(nsAccessibilityAtoms::textbox,
                                         kNameSpaceID_XUL)) {
     if (bindingContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                                     nsAccessibilityAtoms::autocomplete,
                                     eIgnoreCase)) {
       
       
       state |= states::SUPPORTS_AUTOCOMPLETION;
     }
  } else if (gIsFormFillEnabled && htmlInput && !(state & states::PROTECTED)) {
    
    
    
    
    
    nsAutoString autocomplete;
    mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::autocomplete,
                      autocomplete);

    if (!autocomplete.LowerCaseEqualsLiteral("off")) {
      nsCOMPtr<nsIDOMHTMLFormElement> form;
      htmlInput->GetForm(getter_AddRefs(form));
      nsCOMPtr<nsIContent> formContent(do_QueryInterface(form));
      if (formContent) {
        formContent->GetAttr(kNameSpaceID_None,
                             nsAccessibilityAtoms::autocomplete, autocomplete);
      }

      if (!formContent || !autocomplete.LowerCaseEqualsLiteral("off"))
        state |= states::SUPPORTS_AUTOCOMPLETION;
    }
  }

  return state;
}

NS_IMETHODIMP nsHTMLTextFieldAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;;
}

NS_IMETHODIMP nsHTMLTextFieldAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("activate");
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsHTMLTextFieldAccessible::DoAction(PRUint8 index)
{
  if (index == 0) {
    nsCOMPtr<nsIDOMHTMLElement> element(do_QueryInterface(mContent));
    if ( element ) {
      return element->Focus();
    }
    return NS_ERROR_FAILURE;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsHTMLTextFieldAccessible::GetAssociatedEditor(nsIEditor **aEditor)
{
  *aEditor = nsnull;
  nsCOMPtr<nsIDOMNSEditableElement> editableElt(do_QueryInterface(mContent));
  NS_ENSURE_TRUE(editableElt, NS_ERROR_FAILURE);

  
  
  
  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  PRBool pushed = stack && NS_SUCCEEDED(stack->Push(nsnull));

  nsCOMPtr<nsIEditor> editor;
  nsresult rv = editableElt->GetEditor(aEditor);

  if (pushed) {
    JSContext* cx;
    stack->Pop(&cx);
    NS_ASSERTION(!cx, "context should be null");
  }

  return rv;
}





nsHTMLGroupboxAccessible::
  nsHTMLGroupboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}

PRUint32
nsHTMLGroupboxAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_GROUPING;
}

nsIContent* nsHTMLGroupboxAccessible::GetLegend()
{
  nsresult count = 0;
  nsIContent *legendContent = nsnull;
  while ((legendContent = mContent->GetChildAt(count++)) != nsnull) {
    if (legendContent->NodeInfo()->Equals(nsAccessibilityAtoms::legend,
                                          mContent->GetNameSpaceID())) {
      
      return legendContent;
    }
  }

  return nsnull;
}

nsresult
nsHTMLGroupboxAccessible::GetNameInternal(nsAString& aName)
{
  nsresult rv = nsAccessible::GetNameInternal(aName);
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

NS_IMETHODIMP
nsHTMLGroupboxAccessible::GetRelationByType(PRUint32 aRelationType,
                                            nsIAccessibleRelation **aRelation)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetRelationByType(aRelationType,
                                                             aRelation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aRelationType == nsIAccessibleRelation::RELATION_LABELLED_BY) {
    
    return nsRelUtils::
      AddTargetFromContent(aRelationType, aRelation, GetLegend());
  }

  return NS_OK;
}






nsHTMLLegendAccessible::
  nsHTMLLegendAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}

NS_IMETHODIMP
nsHTMLLegendAccessible::GetRelationByType(PRUint32 aRelationType,
                                          nsIAccessibleRelation **aRelation)
{
  nsresult rv = nsHyperTextAccessibleWrap::
    GetRelationByType(aRelationType, aRelation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aRelationType == nsIAccessibleRelation::RELATION_LABEL_FOR) {
    
    nsAccessible* groupbox = GetParent();

    if (groupbox && groupbox->Role() == nsIAccessibleRole::ROLE_GROUPING) {
      
      
      nsCOMPtr<nsIAccessible> testLabelAccessible =
        nsRelUtils::GetRelatedAccessible(groupbox,
                                         nsIAccessibleRelation::RELATION_LABELLED_BY);

      if (testLabelAccessible == this) {
        
        
        return nsRelUtils::
          AddTarget(aRelationType, aRelation, groupbox);
      }
    }
  }

  return NS_OK;
}

PRUint32
nsHTMLLegendAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_LABEL;
}
