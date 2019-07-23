







































#include "nsXULFormControlAccessible.h"
#include "nsHTMLFormControlAccessible.h"
#include "nsAccessibilityAtoms.h"
#include "nsAccessibleTreeWalker.h"
#include "nsXULMenuAccessible.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIDOMXULButtonElement.h"
#include "nsIDOMXULCheckboxElement.h"
#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULTextboxElement.h"
#include "nsIEditor.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsITextControlFrame.h"
#include "nsIPresShell.h"





nsXULButtonAccessible::
  nsXULButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell) :
  nsAccessibleWrap(aNode, aShell)
{
}




NS_IMPL_ISUPPORTS_INHERITED0(nsXULButtonAccessible, nsAccessible)




NS_IMETHODIMP
nsXULButtonAccessible::GetNumActions(PRUint8 *aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  *aCount = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsXULButtonAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("press"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsXULButtonAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex == 0)
    return DoCommand();

  return NS_ERROR_INVALID_ARG;
}




nsresult
nsXULButtonAccessible::Init()
{
  nsresult rv = nsAccessibleWrap::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  if (ContainsMenu())
    nsCoreUtils::GeneratePopupTree(mDOMNode);

  return NS_OK;
}




nsresult
nsXULButtonAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PUSHBUTTON;
  return NS_OK;
}

nsresult
nsXULButtonAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  

  
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  PRBool disabled = PR_FALSE;
  nsCOMPtr<nsIDOMXULControlElement> xulFormElement(do_QueryInterface(mDOMNode));
  if (xulFormElement) {
    xulFormElement->GetDisabled(&disabled);
    if (disabled)
      *aState |= nsIAccessibleStates::STATE_UNAVAILABLE;
    else 
      *aState |= nsIAccessibleStates::STATE_FOCUSABLE;
  }

  
  nsCOMPtr<nsIDOMXULButtonElement> xulButtonElement(do_QueryInterface(mDOMNode));
  if (xulButtonElement) {
    nsAutoString type;
    xulButtonElement->GetType(type);
    if (type.EqualsLiteral("checkbox") || type.EqualsLiteral("radio")) {
      *aState |= nsIAccessibleStates::STATE_CHECKABLE;
      PRBool checked = PR_FALSE;
      PRInt32 checkState = 0;
      xulButtonElement->GetChecked(&checked);
      if (checked) {
        *aState |= nsIAccessibleStates::STATE_PRESSED;
        xulButtonElement->GetCheckState(&checkState);
        if (checkState == nsIDOMXULButtonElement::CHECKSTATE_MIXED) { 
          *aState |= nsIAccessibleStates::STATE_MIXED;
        }
      }
    }
  }

  if (ContainsMenu())
    *aState |= nsIAccessibleStates::STATE_HASPOPUP;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (content->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::_default))
    *aState |= nsIAccessibleStates::STATE_DEFAULT;

  return NS_OK;
}




void
nsXULButtonAccessible::CacheChildren()
{
  
  
  

  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  PRBool isMenu = content->AttrValueIs(kNameSpaceID_None,
                                       nsAccessibilityAtoms::type,
                                       nsAccessibilityAtoms::menu,
                                       eCaseMatters);

  PRBool isMenuButton = isMenu ?
    PR_FALSE :
    content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                         nsAccessibilityAtoms::menuButton, eCaseMatters);

  if (!isMenu && !isMenuButton)
    return;

  nsCOMPtr<nsIAccessible> buttonAccessible;
  nsCOMPtr<nsIAccessible> menupopupAccessible;

  nsAccessibleTreeWalker walker(mWeakShell, mDOMNode, PR_TRUE);
  walker.GetFirstChild();

  while (walker.mState.accessible) {
    PRUint32 role = nsAccUtils::Role(walker.mState.accessible);

    if (role == nsIAccessibleRole::ROLE_MENUPOPUP) {
      
      menupopupAccessible = walker.mState.accessible;

    } else if (isMenuButton && role == nsIAccessibleRole::ROLE_PUSHBUTTON) {
      
      
      buttonAccessible = walker.mState.accessible;
      break;
    }

    walker.GetNextSibling();
  }

  if (!menupopupAccessible)
    return;

  mChildren.AppendObject(menupopupAccessible);

  nsRefPtr<nsAccessible> menupopupAcc =
    nsAccUtils::QueryObject<nsAccessible>(menupopupAccessible);
  menupopupAcc->SetParent(this);

  if (buttonAccessible) {
    mChildren.AppendObject(buttonAccessible);

    nsRefPtr<nsAccessible> buttonAcc =
      nsAccUtils::QueryObject<nsAccessible>(buttonAccessible);
    buttonAcc->SetParent(this);
  }
}




PRBool
nsXULButtonAccessible::ContainsMenu()
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsAccessibilityAtoms::menu, &nsAccessibilityAtoms::menuButton, nsnull};

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  return content->FindAttrValueIn(kNameSpaceID_None, nsAccessibilityAtoms::type,
                                  strings, eCaseMatters) >= 0;
}








nsXULDropmarkerAccessible::nsXULDropmarkerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsFormControlAccessible(aNode, aShell)
{ 
}




NS_IMETHODIMP nsXULDropmarkerAccessible::GetNumActions(PRUint8 *aResult)
{
  *aResult = 1;
  return NS_OK;
}

PRBool nsXULDropmarkerAccessible::DropmarkerOpen(PRBool aToggleOpen)
{
  PRBool isOpen = PR_FALSE;

  nsCOMPtr<nsIDOMNode> parentButtonNode;
  mDOMNode->GetParentNode(getter_AddRefs(parentButtonNode));
  nsCOMPtr<nsIDOMXULButtonElement> parentButtonElement(do_QueryInterface(parentButtonNode));

  if (parentButtonElement) {
    parentButtonElement->GetOpen(&isOpen);
    if (aToggleOpen)
      parentButtonElement->SetOpen(!isOpen);
  }
  else {
    nsCOMPtr<nsIDOMXULMenuListElement> parentMenuListElement(do_QueryInterface(parentButtonNode));
    if (parentMenuListElement) {
      parentMenuListElement->GetOpen(&isOpen);
      if (aToggleOpen)
        parentMenuListElement->SetOpen(!isOpen);
    }
  }

  return isOpen;
}




NS_IMETHODIMP nsXULDropmarkerAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    if (DropmarkerOpen(PR_FALSE))
      aName.AssignLiteral("close");
    else
      aName.AssignLiteral("open");
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}




NS_IMETHODIMP nsXULDropmarkerAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Click) {
    DropmarkerOpen(PR_TRUE); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}




nsresult
nsXULDropmarkerAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PUSHBUTTON;
  return NS_OK;
}

nsresult
nsXULDropmarkerAccessible::GetStateInternal(PRUint32 *aState,
                                            PRUint32 *aExtraState)
{
  *aState = 0;

  if (IsDefunct()) {
    if (aExtraState)
      *aExtraState = nsIAccessibleStates::EXT_STATE_DEFUNCT;

    return NS_OK_DEFUNCT_OBJECT;
  }

  if (aExtraState)
    *aExtraState = 0;

  if (DropmarkerOpen(PR_FALSE))
    *aState = nsIAccessibleStates::STATE_PRESSED;

  return NS_OK;
}








nsXULCheckboxAccessible::nsXULCheckboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsFormControlAccessible(aNode, aShell)
{ 
}




nsresult
nsXULCheckboxAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_CHECKBUTTON;
  return NS_OK;
}




NS_IMETHODIMP nsXULCheckboxAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;
}




NS_IMETHODIMP nsXULCheckboxAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
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




NS_IMETHODIMP nsXULCheckboxAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Click) {
   return DoCommand();
  }
  return NS_ERROR_INVALID_ARG;
}




nsresult
nsXULCheckboxAccessible::GetStateInternal(PRUint32 *aState,
                                          PRUint32 *aExtraState)
{
  
  nsresult rv = nsFormControlAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);
  
  *aState |= nsIAccessibleStates::STATE_CHECKABLE;
  
  
  nsCOMPtr<nsIDOMXULCheckboxElement> xulCheckboxElement(do_QueryInterface(mDOMNode));
  if (xulCheckboxElement) {
    PRBool checked = PR_FALSE;
    xulCheckboxElement->GetChecked(&checked);
    if (checked) {
      *aState |= nsIAccessibleStates::STATE_CHECKED;
      PRInt32 checkState = 0;
      xulCheckboxElement->GetCheckState(&checkState);
      if (checkState == nsIDOMXULCheckboxElement::CHECKSTATE_MIXED)
        *aState |= nsIAccessibleStates::STATE_MIXED;
    }
  }

  return NS_OK;
}





nsXULGroupboxAccessible::nsXULGroupboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{ 
}

nsresult
nsXULGroupboxAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_GROUPING;
  return NS_OK;
}

nsresult
nsXULGroupboxAccessible::GetNameInternal(nsAString& aName)
{
  
  nsCOMPtr<nsIAccessible> label =
    nsRelUtils::GetRelatedAccessible(this, nsIAccessibleRelation::RELATION_LABELLED_BY);

  if (label) {
    return label->GetName(aName);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULGroupboxAccessible::GetRelationByType(PRUint32 aRelationType,
                                           nsIAccessibleRelation **aRelation)
{
  nsresult rv = nsAccessibleWrap::GetRelationByType(aRelationType, aRelation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aRelationType == nsIAccessibleRelation::RELATION_LABELLED_BY) {
    
    
    
    nsCOMPtr<nsIAccessible> testLabelAccessible;
    while (NextChild(testLabelAccessible)) {
      if (nsAccUtils::Role(testLabelAccessible) == nsIAccessibleRole::ROLE_LABEL) {
        
        
        
        nsCOMPtr<nsIAccessible> testGroupboxAccessible =
          nsRelUtils::GetRelatedAccessible(testLabelAccessible,
                                           nsIAccessibleRelation::RELATION_LABEL_FOR);

        if (testGroupboxAccessible == this) {
          
          return nsRelUtils::
            AddTarget(aRelationType, aRelation, testLabelAccessible);
        }
      }
    }
  }

  return NS_OK;
}





NS_IMPL_ISUPPORTS_INHERITED1(nsXULProgressMeterAccessible,
                             nsFormControlAccessible,
                             nsIAccessibleValue)

nsXULProgressMeterAccessible::
  nsXULProgressMeterAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell) :
  nsFormControlAccessible(aNode, aShell)
{
}



nsresult
nsXULProgressMeterAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PROGRESSBAR;
  return NS_OK;
}



NS_IMETHODIMP
nsXULProgressMeterAccessible::GetValue(nsAString& aValue)
{
  nsresult rv = nsFormControlAccessible::GetValue(aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aValue.IsEmpty())
    return NS_OK;

  double maxValue = 0;
  rv = GetMaximumValue(&maxValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (maxValue != 1) {
    double curValue = 0;
    rv = GetCurrentValue(&curValue);
    NS_ENSURE_SUCCESS(rv, rv);

    double percentValue = (curValue / maxValue) * 100;
    nsAutoString value;
    value.AppendFloat(percentValue); 
    value.AppendLiteral("%");
    aValue = value;
    return NS_OK;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value, aValue);
  if (aValue.IsEmpty())
    aValue.AppendLiteral("0");  

  aValue.AppendLiteral("%");
  return NS_OK;
}

NS_IMETHODIMP
nsXULProgressMeterAccessible::GetMaximumValue(double *aMaximumValue)
{
  nsresult rv = nsFormControlAccessible::GetMaximumValue(aMaximumValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  nsAutoString value;
  if (content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::max, value)) {
    PRInt32 result = NS_OK;
    *aMaximumValue = value.ToFloat(&result);
    return result;
  }

  *aMaximumValue = 1; 
  return NS_OK;
}

NS_IMETHODIMP
nsXULProgressMeterAccessible::GetMinimumValue(double *aMinimumValue)
{
  nsresult rv = nsFormControlAccessible::GetMinimumValue(aMinimumValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aMinimumValue = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsXULProgressMeterAccessible::GetMinimumIncrement(double *aMinimumIncrement)
{
  nsresult rv = nsFormControlAccessible::GetMinimumIncrement(aMinimumIncrement);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aMinimumIncrement = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsXULProgressMeterAccessible::GetCurrentValue(double *aCurrentValue)
{
  nsresult rv = nsFormControlAccessible::GetCurrentValue(aCurrentValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  nsAutoString attrValue;
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value, attrValue);

  
  if (attrValue.IsEmpty())
    return NS_OK;

  PRInt32 error = NS_OK;
  double value = attrValue.ToFloat(&error);
  if (NS_FAILED(error))
    return NS_OK; 

  
  
  if (!content->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::max))
    value /= 100;

  *aCurrentValue = value;
  return NS_OK;
}

NS_IMETHODIMP
nsXULProgressMeterAccessible::SetCurrentValue(double aValue)
{
  return NS_ERROR_FAILURE; 
}







nsXULRadioButtonAccessible::nsXULRadioButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsRadioButtonAccessible(aNode, aShell)
{ 
}


nsresult
nsXULRadioButtonAccessible::GetStateInternal(PRUint32 *aState,
                                             PRUint32 *aExtraState)
{
  nsresult rv = nsFormControlAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_CHECKABLE;
  
  PRBool selected = PR_FALSE;   

  nsCOMPtr<nsIDOMXULSelectControlItemElement> radioButton(do_QueryInterface(mDOMNode));
  if (radioButton) {
    radioButton->GetSelected(&selected);
    if (selected) {
      *aState |= nsIAccessibleStates::STATE_CHECKED;
    }
  }

  return NS_OK;
}

nsresult
nsXULRadioButtonAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  NS_ENSURE_TRUE(mDOMNode, NS_ERROR_FAILURE);

  nsresult rv = nsFormControlAccessible::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAccUtils::SetAccAttrsForXULSelectControlItem(mDOMNode, aAttributes);

  return NS_OK;
}











nsXULRadioGroupAccessible::nsXULRadioGroupAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsXULSelectableAccessible(aNode, aShell)
{ 
}

nsresult
nsXULRadioGroupAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_GROUPING;
  return NS_OK;
}

nsresult
nsXULRadioGroupAccessible::GetStateInternal(PRUint32 *aState,
                                            PRUint32 *aExtraState)
{
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  
  
  
  *aState &= ~(nsIAccessibleStates::STATE_FOCUSABLE |
               nsIAccessibleStates::STATE_FOCUSED);

  return NS_OK;
}







nsXULStatusBarAccessible::nsXULStatusBarAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{ 
}




nsresult
nsXULStatusBarAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_STATUSBAR;
  return NS_OK;
}




nsXULToolbarButtonAccessible::nsXULToolbarButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsXULButtonAccessible(aNode, aShell)
{
}

nsresult
nsXULToolbarButtonAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  NS_ENSURE_TRUE(mDOMNode, NS_ERROR_FAILURE);

  nsresult rv = nsXULButtonAccessible::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAccessible> parent(GetParent());
  PRInt32 setSize = 0;
  PRInt32 posInSet = 0;

  if (parent) {
    nsCOMPtr<nsIAccessible> sibling;
    nsCOMPtr<nsIAccessible> tempSibling;
    parent->GetFirstChild(getter_AddRefs(sibling));
    while (sibling) {
      if (IsSeparator(sibling)) { 
        if (posInSet)
          break; 
        setSize = 0; 
      } else {
        setSize++; 
        if (sibling == this)
          posInSet = setSize; 
      }
      sibling->GetNextSibling(getter_AddRefs(tempSibling));
      sibling.swap(tempSibling);
    }
  }
  
  nsAccUtils::SetAccGroupAttrs(aAttributes, 0, posInSet, setSize);

  return NS_OK;
}

PRBool
nsXULToolbarButtonAccessible::IsSeparator(nsIAccessible *aAccessible)
{
  nsCOMPtr<nsIDOMNode> domNode;
  nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(aAccessible));
  accessNode->GetDOMNode(getter_AddRefs(domNode));
  nsCOMPtr<nsIContent> contentDomNode(do_QueryInterface(domNode));

  if (!contentDomNode)
    return PR_FALSE;

  return (contentDomNode->Tag() == nsAccessibilityAtoms::toolbarseparator) ||
         (contentDomNode->Tag() == nsAccessibilityAtoms::toolbarspacer) ||
         (contentDomNode->Tag() == nsAccessibilityAtoms::toolbarspring);
}





nsXULToolbarAccessible::nsXULToolbarAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{ 
}

nsresult
nsXULToolbarAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_TOOLBAR;
  return NS_OK;
}





nsXULToolbarSeparatorAccessible::nsXULToolbarSeparatorAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsLeafAccessible(aNode, aShell)
{ 
}

nsresult
nsXULToolbarSeparatorAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_SEPARATOR;
  return NS_OK;
}

nsresult
nsXULToolbarSeparatorAccessible::GetStateInternal(PRUint32 *aState,
                                                  PRUint32 *aExtraState)
{
  *aState = 0;  

  if (IsDefunct()) {
    if (aExtraState)
      *aExtraState = nsIAccessibleStates::EXT_STATE_DEFUNCT;

    return NS_OK_DEFUNCT_OBJECT;
  }

  if (aExtraState)
    *aExtraState = 0;

  return NS_OK;
}





nsXULTextFieldAccessible::nsXULTextFieldAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell) :
 nsHyperTextAccessibleWrap(aNode, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED3(nsXULTextFieldAccessible, nsAccessible, nsHyperTextAccessible, nsIAccessibleText, nsIAccessibleEditableText)

NS_IMETHODIMP nsXULTextFieldAccessible::GetValue(nsAString& aValue)
{
  PRUint32 state;
  nsresult rv = GetStateInternal(&state, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  if (state & nsIAccessibleStates::STATE_PROTECTED)    
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULTextBoxElement> textBox(do_QueryInterface(mDOMNode));
  if (textBox) {
    return textBox->GetValue(aValue);
  }
  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mDOMNode));
  if (menuList) {
    return menuList->GetLabel(aValue);
  }
  return NS_ERROR_FAILURE;
}

already_AddRefed<nsIDOMNode> nsXULTextFieldAccessible::GetInputField()
{
  nsIDOMNode *inputField = nsnull;
  nsCOMPtr<nsIDOMXULTextBoxElement> textBox = do_QueryInterface(mDOMNode);
  if (textBox) {
    textBox->GetInputField(&inputField);
    return inputField;
  }
  nsCOMPtr<nsIDOMXULMenuListElement> menuList = do_QueryInterface(mDOMNode);
  if (menuList) {   
    menuList->GetInputField(&inputField);
  }
  NS_ASSERTION(inputField, "No input field for nsXULTextFieldAccessible");
  return inputField;
}

nsresult
nsXULTextFieldAccessible::GetARIAState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetARIAState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  nsStateMapEntry::MapToStates(content, aState, aExtraState, eARIAAutoComplete);

  return NS_OK;
}

nsresult
nsXULTextFieldAccessible::GetStateInternal(PRUint32 *aState,
                                           PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> inputField = GetInputField();
  NS_ENSURE_TRUE(inputField, NS_ERROR_FAILURE);

  
  
  
  nsHTMLTextFieldAccessible* tempAccessible =
    new nsHTMLTextFieldAccessible(inputField, mWeakShell);
  if (!tempAccessible)
    return NS_ERROR_OUT_OF_MEMORY;
  nsCOMPtr<nsIAccessible> kungFuDeathGrip = tempAccessible;
  rv = tempAccessible->GetStateInternal(aState, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  if (gLastFocusedNode == mDOMNode) {
    *aState |= nsIAccessibleStates::STATE_FOCUSED;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ASSERTION(content, "Not possible since we have an mDOMNode");

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mDOMNode));
  if (menuList) {
    
    if (!content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::editable,
                              nsAccessibilityAtoms::_true, eIgnoreCase)) {
      *aState |= nsIAccessibleStates::STATE_READONLY;
    }
  }
  else {
    
    if (content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                             nsAccessibilityAtoms::password, eIgnoreCase)) {
      *aState |= nsIAccessibleStates::STATE_PROTECTED;
    }
    if (content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::readonly,
                             nsAccessibilityAtoms::_true, eIgnoreCase)) {
      *aState |= nsIAccessibleStates::STATE_READONLY;
    }
  }

  if (!aExtraState)
    return NS_OK;

  PRBool isMultiLine = content->HasAttr(kNameSpaceID_None,
                                        nsAccessibilityAtoms::multiline);

  if (isMultiLine) {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_MULTI_LINE;
  }
  else {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_SINGLE_LINE;
  }

  return NS_OK;
}

nsresult
nsXULTextFieldAccessible::GetRoleInternal(PRUint32 *aRole)
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





NS_IMETHODIMP nsXULTextFieldAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;
}




NS_IMETHODIMP nsXULTextFieldAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("activate"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}




NS_IMETHODIMP nsXULTextFieldAccessible::DoAction(PRUint8 index)
{
  if (index == 0) {
    nsCOMPtr<nsIDOMXULElement> element(do_QueryInterface(mDOMNode));
    if (element)
    {
      element->Focus();
      return NS_OK;
    }
    return NS_ERROR_FAILURE;
  }
  return NS_ERROR_INVALID_ARG;
}

PRBool
nsXULTextFieldAccessible::GetAllowsAnonChildAccessibles()
{
  return PR_FALSE;
}

NS_IMETHODIMP nsXULTextFieldAccessible::GetAssociatedEditor(nsIEditor **aEditor)
{
  *aEditor = nsnull;
  nsCOMPtr<nsIDOMNode> inputField = GetInputField();
  nsCOMPtr<nsIDOMNSEditableElement> editableElt(do_QueryInterface(inputField));
  NS_ENSURE_TRUE(editableElt, NS_ERROR_FAILURE);
  return editableElt->GetEditor(aEditor);
}
