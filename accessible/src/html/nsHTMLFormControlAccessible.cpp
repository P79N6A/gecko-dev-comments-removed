






































#include "nsAccessibleTreeWalker.h"
#include "nsAccessibilityAtoms.h"
#include "nsHTMLFormControlAccessible.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIDOMNSHTMLButtonElement.h"
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



nsHTMLCheckboxAccessible::nsHTMLCheckboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsFormControlAccessible(aNode, aShell)
{ 
}

nsresult
nsHTMLCheckboxAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_CHECKBUTTON;
  return NS_OK;
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
    else
      aName.AssignLiteral("check"); 

    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsHTMLCheckboxAccessible::DoAction(PRUint8 index)
{
  if (index == 0) {   
    return DoCommand();
  }
  return NS_ERROR_INVALID_ARG;
}

nsresult
nsHTMLCheckboxAccessible::GetStateInternal(PRUint32 *aState,
                                           PRUint32 *aExtraState)
{
  nsresult rv = nsFormControlAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_CHECKABLE;

  PRBool checked = PR_FALSE;   
  PRBool mixed = PR_FALSE;     

  nsCOMPtr<nsIDOMNSHTMLInputElement>
           html5CheckboxElement(do_QueryInterface(mDOMNode));
           
  if (html5CheckboxElement) {
    html5CheckboxElement->GetIndeterminate(&mixed);

    if (mixed) {
      *aState |= nsIAccessibleStates::STATE_MIXED;
      return NS_OK;  
    }
  }
  
  nsCOMPtr<nsIDOMHTMLInputElement> htmlCheckboxElement(do_QueryInterface(mDOMNode));
  if (htmlCheckboxElement) {
    htmlCheckboxElement->GetChecked(&checked);
  
    if (checked)
      *aState |= nsIAccessibleStates::STATE_CHECKED;
  }

  return NS_OK;
}



nsHTMLRadioButtonAccessible::nsHTMLRadioButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsRadioButtonAccessible(aNode, aShell)
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

  nsCOMPtr<nsIDOMHTMLInputElement> htmlRadioElement(do_QueryInterface(mDOMNode));
  if (htmlRadioElement)
    htmlRadioElement->GetChecked(&checked);

  if (checked)
    *aState |= nsIAccessibleStates::STATE_CHECKED;

  return NS_OK;
}

nsresult
nsHTMLRadioButtonAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  NS_ENSURE_TRUE(mDOMNode, NS_ERROR_FAILURE);

  nsresult rv = nsRadioButtonAccessible::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString nsURI;
  mDOMNode->GetNamespaceURI(nsURI);
  nsAutoString tagName;
  mDOMNode->GetLocalName(tagName);

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ENSURE_STATE(content);

  nsAutoString type;
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::type, type);
  nsAutoString name;
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::name, name);

  nsCOMPtr<nsIDOMNodeList> inputs;

  nsCOMPtr<nsIDOMHTMLInputElement> radio(do_QueryInterface(mDOMNode));
  nsCOMPtr<nsIDOMHTMLFormElement> form;
  radio->GetForm(getter_AddRefs(form));
  if (form) {
    form->GetElementsByTagNameNS(nsURI, tagName, getter_AddRefs(inputs));
  } else {
    nsCOMPtr<nsIDOMDocument> document;
    mDOMNode->GetOwnerDocument(getter_AddRefs(document));
    if (document)
      document->GetElementsByTagNameNS(nsURI, tagName, getter_AddRefs(inputs));
  }

  NS_ENSURE_TRUE(inputs, NS_OK);

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

      if (itemNode == mDOMNode)
        indexOf = count;
    }
  }

  nsAccUtils::SetAccGroupAttrs(aAttributes, 0, indexOf, count);

  return  NS_OK;
}



nsHTMLButtonAccessible::nsHTMLButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsHyperTextAccessibleWrap(aNode, aShell)
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

NS_IMETHODIMP nsHTMLButtonAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Click) {
    return DoCommand();
  }
  return NS_ERROR_INVALID_ARG;
}

nsresult
nsHTMLButtonAccessible::GetStateInternal(PRUint32 *aState,
                                         PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);

  nsAutoString buttonType;
  element->GetAttribute(NS_LITERAL_STRING("type"), buttonType);
  if (buttonType.LowerCaseEqualsLiteral("submit"))
    *aState |= nsIAccessibleStates::STATE_DEFAULT;

  return NS_OK;
}

nsresult
nsHTMLButtonAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PUSHBUTTON;
  return NS_OK;
}

nsresult
nsHTMLButtonAccessible::GetNameInternal(nsAString& aName)
{
  nsresult rv = nsAccessible::GetNameInternal(aName);
  if (!aName.IsEmpty())
    return NS_OK;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  
  nsAutoString name;
  if (!content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value,
                        name) &&
      !content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::alt,
                        name)) {
    
    nsIFrame* frame = GetFrame();
    if (frame) {
      nsIFormControlFrame* fcFrame = do_QueryFrame(frame);
      if (fcFrame)
        fcFrame->GetFormProperty(nsAccessibilityAtoms::defaultLabel, name);
    }
  }

  if (name.IsEmpty() &&
      !content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::src,
                        name)) {
    content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::data, name);
  }

  name.CompressWhitespace();
  aName = name;

  return NS_OK;
}




nsHTML4ButtonAccessible::nsHTML4ButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsHyperTextAccessibleWrap(aNode, aShell)
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

NS_IMETHODIMP nsHTML4ButtonAccessible::DoAction(PRUint8 index)
{
  if (index == 0) {
    return DoCommand();
  }
  return NS_ERROR_INVALID_ARG;
}

nsresult
nsHTML4ButtonAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PUSHBUTTON;
  return NS_OK;
}

nsresult
nsHTML4ButtonAccessible::GetStateInternal(PRUint32 *aState,
                                          PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  NS_ASSERTION(element, "No element for button's dom node!");

  *aState |= nsIAccessibleStates::STATE_FOCUSABLE;

  nsAutoString buttonType;
  element->GetAttribute(NS_LITERAL_STRING("type"), buttonType);
  if (buttonType.LowerCaseEqualsLiteral("submit"))
    *aState |= nsIAccessibleStates::STATE_DEFAULT;

  return NS_OK;
}



nsHTMLTextFieldAccessible::nsHTMLTextFieldAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsHyperTextAccessibleWrap(aNode, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED3(nsHTMLTextFieldAccessible, nsAccessible, nsHyperTextAccessible, nsIAccessibleText, nsIAccessibleEditableText)

nsresult
nsHTMLTextFieldAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_ENTRY;
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (content &&
      content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                           nsAccessibilityAtoms::password, eIgnoreCase)) {
    *aRole = nsIAccessibleRole::ROLE_PASSWORD_TEXT;
  }
  return NS_OK;
}

nsresult
nsHTMLTextFieldAccessible::GetNameInternal(nsAString& aName)
{
  nsresult rv = nsAccessible::GetNameInternal(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aName.IsEmpty())
    return NS_OK;

  nsCOMPtr<nsIContent> content = do_QueryInterface(mDOMNode);
  if (!content->GetBindingParent())
    return NS_OK;

  
  
  
  
  
  nsCOMPtr<nsIAccessible> parent;
  rv = GetParent(getter_AddRefs(parent));
  return parent ? parent->GetName(aName) : rv;
}

NS_IMETHODIMP nsHTMLTextFieldAccessible::GetValue(nsAString& _retval)
{
  PRUint32 state;
  nsresult rv = GetStateInternal(&state, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  if (state & nsIAccessibleStates::STATE_PROTECTED)    
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMHTMLTextAreaElement> textArea(do_QueryInterface(mDOMNode));
  if (textArea) {
    return textArea->GetValue(_retval);
  }
  
  nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(mDOMNode));
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

  
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ASSERTION(content, "Should not have gotten here if upcalled GetExtState() succeeded");

  if (content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                           nsAccessibilityAtoms::password, eIgnoreCase)) {
    *aState |= nsIAccessibleStates::STATE_PROTECTED;
  }
  else {
    nsCOMPtr<nsIAccessible> parent;
    GetParent(getter_AddRefs(parent));
    if (nsAccUtils::Role(parent) == nsIAccessibleRole::ROLE_AUTOCOMPLETE)
      *aState |= nsIAccessibleStates::STATE_HASPOPUP;
  }

  if (content->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::readonly)) {
    *aState |= nsIAccessibleStates::STATE_READONLY;
  }

  if (!aExtraState)
    return NS_OK;

  nsCOMPtr<nsIDOMHTMLInputElement> htmlInput(do_QueryInterface(mDOMNode));
  
  if (htmlInput) {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_SINGLE_LINE;
  }
  else {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_MULTI_LINE;
  }

  if (!(*aExtraState & nsIAccessibleStates::EXT_STATE_EDITABLE))
    return NS_OK;

  nsCOMPtr<nsIContent> bindingContent = content->GetBindingParent();
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
    content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::autocomplete,
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
    nsCOMPtr<nsIDOMNSHTMLElement> element(do_QueryInterface(mDOMNode));
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
  nsCOMPtr<nsIDOMNSEditableElement> editableElt(do_QueryInterface(mDOMNode));
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







nsHTMLGroupboxAccessible::nsHTMLGroupboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsHyperTextAccessibleWrap(aNode, aShell)
{ 
}

nsresult
nsHTMLGroupboxAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_GROUPING;
  return NS_OK;
}

nsIContent* nsHTMLGroupboxAccessible::GetLegend()
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mDOMNode);
  NS_ENSURE_TRUE(content, nsnull);

  nsresult count = 0;
  nsIContent *testLegendContent;
  while ((testLegendContent = content->GetChildAt(count ++ )) != nsnull) {
    if (testLegendContent->NodeInfo()->Equals(nsAccessibilityAtoms::legend,
                                              content->GetNameSpaceID())) {
      
      return testLegendContent;
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




nsHTMLLegendAccessible::nsHTMLLegendAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsHyperTextAccessibleWrap(aNode, aShell)
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
    
    nsCOMPtr<nsIAccessible> groupboxAccessible = GetParent();
    if (nsAccUtils::Role(groupboxAccessible) == nsIAccessibleRole::ROLE_GROUPING) {
      
      
      nsCOMPtr<nsIAccessible> testLabelAccessible =
        nsRelUtils::GetRelatedAccessible(groupboxAccessible,
                                         nsIAccessibleRelation::RELATION_LABELLED_BY);

      if (testLabelAccessible == this) {
        
        
        return nsRelUtils::
          AddTarget(aRelationType, aRelation, groupboxAccessible);
      }
    }
  }

  return NS_OK;
}

nsresult
nsHTMLLegendAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_LABEL;
  return NS_OK;
}
