





































#include "nsHTMLFormControlAccessible.h"

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
    
    PRUint32 state;
    nsresult rv = GetStateInternal(&state, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    if (state & nsIAccessibleStates::STATE_CHECKED)
      aName.AssignLiteral("uncheck"); 
    else if (state & nsIAccessibleStates::STATE_MIXED)
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

nsresult
nsHTMLCheckboxAccessible::GetStateInternal(PRUint32 *aState,
                                           PRUint32 *aExtraState)
{
  nsresult rv = nsFormControlAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_CHECKABLE;

  PRBool state = PR_FALSE;   

  nsCOMPtr<nsIDOMHTMLInputElement> htmlCheckboxElement =
    do_QueryInterface(mContent);
           
  if (htmlCheckboxElement) {
    htmlCheckboxElement->GetIndeterminate(&state);

    if (state) {
      *aState |= nsIAccessibleStates::STATE_MIXED;
    } else {   
      htmlCheckboxElement->GetChecked(&state);
    
      if (state)
        *aState |= nsIAccessibleStates::STATE_CHECKED;
    }
  }
  return NS_OK;
}





nsHTMLRadioButtonAccessible::
  nsHTMLRadioButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsRadioButtonAccessible(aContent, aShell)
{
}

nsresult
nsHTMLRadioButtonAccessible::GetStateInternal(PRUint32 *aState,
                                              PRUint32 *aExtraState)
{
  nsresult rv = nsAccessibleWrap::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_CHECKABLE;
  
  PRBool checked = PR_FALSE;   

  nsCOMPtr<nsIDOMHTMLInputElement> htmlRadioElement =
    do_QueryInterface(mContent);
  if (htmlRadioElement)
    htmlRadioElement->GetChecked(&checked);

  if (checked)
    *aState |= nsIAccessibleStates::STATE_CHECKED;

  return NS_OK;
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

nsresult
nsHTMLButtonAccessible::GetStateInternal(PRUint32 *aState,
                                         PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                            nsAccessibilityAtoms::submit, eIgnoreCase))
    *aState |= nsIAccessibleStates::STATE_DEFAULT;

  return NS_OK;
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

nsresult
nsHTML4ButtonAccessible::GetStateInternal(PRUint32 *aState,
                                          PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_FOCUSABLE;

  if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                            nsAccessibilityAtoms::submit, eIgnoreCase))
    *aState |= nsIAccessibleStates::STATE_DEFAULT;

  return NS_OK;
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
  PRUint32 state;
  nsresult rv = GetStateInternal(&state, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  if (state & nsIAccessibleStates::STATE_PROTECTED)    
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

nsresult
nsHTMLTextFieldAccessible::GetStateInternal(PRUint32 *aState,
                                            PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  
  if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                            nsAccessibilityAtoms::password, eIgnoreCase)) {
    *aState |= nsIAccessibleStates::STATE_PROTECTED;
  }
  else {
    nsAccessible* parent = GetParent();
    if (parent && parent->Role() == nsIAccessibleRole::ROLE_AUTOCOMPLETE)
      *aState |= nsIAccessibleStates::STATE_HASPOPUP;
  }

  if (mContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::readonly)) {
    *aState |= nsIAccessibleStates::STATE_READONLY;
  }

  if (!aExtraState)
    return NS_OK;

  nsCOMPtr<nsIDOMHTMLInputElement> htmlInput(do_QueryInterface(mContent));
  
  if (htmlInput) {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_SINGLE_LINE;
  }
  else {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_MULTI_LINE;
  }

  if (!(*aExtraState & nsIAccessibleStates::EXT_STATE_EDITABLE))
    return NS_OK;

  nsCOMPtr<nsIContent> bindingContent = mContent->GetBindingParent();
  if (bindingContent &&
      bindingContent->NodeInfo()->Equals(nsAccessibilityAtoms::textbox,
                                         kNameSpaceID_XUL) &&
      bindingContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                                  nsAccessibilityAtoms::autocomplete,
                                  eIgnoreCase)) {
    
    
    *aExtraState |= nsIAccessibleStates::EXT_STATE_SUPPORTS_AUTOCOMPLETION;
  } else if (gIsFormFillEnabled && htmlInput &&
             !(*aState & nsIAccessibleStates::STATE_PROTECTED)) {
    
    
    
    
    
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
        *aExtraState |= nsIAccessibleStates::EXT_STATE_SUPPORTS_AUTOCOMPLETION;
    }
  }

  return NS_OK;
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
