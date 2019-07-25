






































#include "nsXULFormControlAccessible.h"

#include "nsAccessibilityAtoms.h"
#include "nsAccUtils.h"
#include "nsAccTreeWalker.h"
#include "nsCoreUtils.h"
#include "nsDocAccessible.h"
#include "nsRelUtils.h"


#include "nsHTMLFormControlAccessible.h"
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





nsXULButtonAccessible::
  nsXULButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
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
  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}




PRBool
nsXULButtonAccessible::Init()
{
  if (!nsAccessibleWrap::Init())
    return PR_FALSE;

  if (ContainsMenu())
    nsCoreUtils::GeneratePopupTree(mContent);

  return PR_TRUE;
}




PRUint32
nsXULButtonAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PUSHBUTTON;
}

nsresult
nsXULButtonAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  

  
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  PRBool disabled = PR_FALSE;
  nsCOMPtr<nsIDOMXULControlElement> xulFormElement(do_QueryInterface(mContent));
  if (xulFormElement) {
    xulFormElement->GetDisabled(&disabled);
    if (disabled)
      *aState |= nsIAccessibleStates::STATE_UNAVAILABLE;
    else 
      *aState |= nsIAccessibleStates::STATE_FOCUSABLE;
  }

  
  nsCOMPtr<nsIDOMXULButtonElement> xulButtonElement(do_QueryInterface(mContent));
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

  if (mContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::_default))
    *aState |= nsIAccessibleStates::STATE_DEFAULT;

  return NS_OK;
}




void
nsXULButtonAccessible::CacheChildren()
{
  
  
  

  
  
  PRBool isMenu = mContent->AttrValueIs(kNameSpaceID_None,
                                       nsAccessibilityAtoms::type,
                                       nsAccessibilityAtoms::menu,
                                       eCaseMatters);

  PRBool isMenuButton = isMenu ?
    PR_FALSE :
    mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                          nsAccessibilityAtoms::menuButton, eCaseMatters);

  if (!isMenu && !isMenuButton)
    return;

  nsAccessible* menupopup = nsnull;
  nsAccessible* button = nsnull;

  nsAccTreeWalker walker(mWeakShell, mContent, PR_TRUE);

  nsAccessible* child = nsnull;
  while ((child = walker.NextChild())) {
    PRUint32 role = child->Role();

    if (role == nsIAccessibleRole::ROLE_MENUPOPUP) {
      
      menupopup = child;

    } else if (isMenuButton && role == nsIAccessibleRole::ROLE_PUSHBUTTON) {
      
      
      button = child;
      break;

    } else {
      
      GetDocAccessible()->UnbindFromDocument(child);
    }
  }

  if (!menupopup)
    return;

  AppendChild(menupopup);
  if (button)
    AppendChild(button);
}




PRBool
nsXULButtonAccessible::ContainsMenu()
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsAccessibilityAtoms::menu, &nsAccessibilityAtoms::menuButton, nsnull};

  return mContent->FindAttrValueIn(kNameSpaceID_None,
                                   nsAccessibilityAtoms::type,
                                   strings, eCaseMatters) >= 0;
}





nsXULDropmarkerAccessible::
  nsXULDropmarkerAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsFormControlAccessible(aContent, aShell)
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

  nsCOMPtr<nsIDOMXULButtonElement> parentButtonElement =
    do_QueryInterface(mContent->GetParent());

  if (parentButtonElement) {
    parentButtonElement->GetOpen(&isOpen);
    if (aToggleOpen)
      parentButtonElement->SetOpen(!isOpen);
  }
  else {
    nsCOMPtr<nsIDOMXULMenuListElement> parentMenuListElement =
      do_QueryInterface(parentButtonElement);
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

PRUint32
nsXULDropmarkerAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PUSHBUTTON;
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

                      




nsXULCheckboxAccessible::
  nsXULCheckboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsFormControlAccessible(aContent, aShell)
{
}

PRUint32
nsXULCheckboxAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_CHECKBUTTON;
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




NS_IMETHODIMP
nsXULCheckboxAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}




nsresult
nsXULCheckboxAccessible::GetStateInternal(PRUint32 *aState,
                                          PRUint32 *aExtraState)
{
  
  nsresult rv = nsFormControlAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);
  
  *aState |= nsIAccessibleStates::STATE_CHECKABLE;
  
  
  nsCOMPtr<nsIDOMXULCheckboxElement> xulCheckboxElement =
    do_QueryInterface(mContent);
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





nsXULGroupboxAccessible::
  nsXULGroupboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint32
nsXULGroupboxAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_GROUPING;
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
    
    
    
    PRInt32 childCount = GetChildCount();
    for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
      nsAccessible *childAcc = GetChildAt(childIdx);
      if (childAcc->Role() == nsIAccessibleRole::ROLE_LABEL) {
        
        
        
        nsCOMPtr<nsIAccessible> testGroupboxAccessible =
          nsRelUtils::GetRelatedAccessible(childAcc,
                                           nsIAccessibleRelation::RELATION_LABEL_FOR);

        if (testGroupboxAccessible == this) {
          
          return nsRelUtils::
            AddTarget(aRelationType, aRelation, childAcc);
        }
      }
    }
  }

  return NS_OK;
}





nsXULProgressMeterAccessible::
  nsXULProgressMeterAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsFormControlAccessible(aContent, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED1(nsXULProgressMeterAccessible,
                             nsFormControlAccessible,
                             nsIAccessibleValue)



PRUint32
nsXULProgressMeterAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PROGRESSBAR;
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

  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value, aValue);
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

  nsAutoString value;
  if (mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::max, value)) {
    PRInt32 result = NS_OK;
    *aMaximumValue = value.ToDouble(&result);
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

  nsAutoString attrValue;
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value, attrValue);

  
  if (attrValue.IsEmpty())
    return NS_OK;

  PRInt32 error = NS_OK;
  double value = attrValue.ToDouble(&error);
  if (NS_FAILED(error))
    return NS_OK; 

  
  
  if (!mContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::max))
    value /= 100;

  *aCurrentValue = value;
  return NS_OK;
}

NS_IMETHODIMP
nsXULProgressMeterAccessible::SetCurrentValue(double aValue)
{
  return NS_ERROR_FAILURE; 
}






nsXULRadioButtonAccessible::
  nsXULRadioButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsRadioButtonAccessible(aContent, aShell)
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

  nsCOMPtr<nsIDOMXULSelectControlItemElement> radioButton =
    do_QueryInterface(mContent);
  if (radioButton) {
    radioButton->GetSelected(&selected);
    if (selected) {
      *aState |= nsIAccessibleStates::STATE_CHECKED;
    }
  }

  return NS_OK;
}

void
nsXULRadioButtonAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                                       PRInt32 *aSetSize)
{
  nsAccUtils::GetPositionAndSizeForXULSelectControlItem(mContent, aPosInSet,
                                                        aSetSize);
}















nsXULRadioGroupAccessible::
  nsXULRadioGroupAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULSelectableAccessible(aContent, aShell)
{ 
}

PRUint32
nsXULRadioGroupAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_GROUPING;
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

                      




nsXULStatusBarAccessible::
  nsXULStatusBarAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint32
nsXULStatusBarAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_STATUSBAR;
}






nsXULToolbarButtonAccessible::
  nsXULToolbarButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULButtonAccessible(aContent, aShell)
{
}

void
nsXULToolbarButtonAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                                         PRInt32 *aSetSize)
{
  PRInt32 setSize = 0;
  PRInt32 posInSet = 0;

  nsAccessible* parent(GetParent());
  NS_ENSURE_TRUE(parent,);

  PRInt32 childCount = parent->GetChildCount();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible* child = parent->GetChildAt(childIdx);
    if (IsSeparator(child)) { 
      if (posInSet)
        break; 

      setSize = 0; 

    } else {
      setSize++; 

      if (child == this)
        posInSet = setSize; 
    }
  }

  *aPosInSet = posInSet;
  *aSetSize = setSize;
}

PRBool
nsXULToolbarButtonAccessible::IsSeparator(nsAccessible *aAccessible)
{
  nsCOMPtr<nsIDOMNode> domNode;
  aAccessible->GetDOMNode(getter_AddRefs(domNode));
  nsCOMPtr<nsIContent> contentDomNode(do_QueryInterface(domNode));

  if (!contentDomNode)
    return PR_FALSE;

  return (contentDomNode->Tag() == nsAccessibilityAtoms::toolbarseparator) ||
         (contentDomNode->Tag() == nsAccessibilityAtoms::toolbarspacer) ||
         (contentDomNode->Tag() == nsAccessibilityAtoms::toolbarspring);
}






nsXULToolbarAccessible::
  nsXULToolbarAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint32
nsXULToolbarAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_TOOLBAR;
}

nsresult
nsXULToolbarAccessible::GetNameInternal(nsAString& aName)
{
  nsAutoString name;
  if (mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::toolbarname,
                        name)) {
    name.CompressWhitespace();
    aName = name;
  }

  return NS_OK;
}






nsXULToolbarSeparatorAccessible::
  nsXULToolbarSeparatorAccessible(nsIContent *aContent,
                                  nsIWeakReference *aShell) :
  nsLeafAccessible(aContent, aShell)
{
}

PRUint32
nsXULToolbarSeparatorAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_SEPARATOR;
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





nsXULTextFieldAccessible::
 nsXULTextFieldAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
 nsHyperTextAccessibleWrap(aContent, aShell)
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

  nsCOMPtr<nsIDOMXULTextBoxElement> textBox(do_QueryInterface(mContent));
  if (textBox) {
    return textBox->GetValue(aValue);
  }
  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (menuList) {
    return menuList->GetLabel(aValue);
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsXULTextFieldAccessible::GetARIAState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetARIAState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  nsStateMapEntry::MapToStates(mContent, aState, aExtraState, eARIAAutoComplete);

  return NS_OK;
}

nsresult
nsXULTextFieldAccessible::GetStateInternal(PRUint32 *aState,
                                           PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> inputField(GetInputField());
  NS_ENSURE_TRUE(inputField, NS_ERROR_FAILURE);

  
  
  
  nsRefPtr<nsHTMLTextFieldAccessible> tempAccessible =
    new nsHTMLTextFieldAccessible(inputField, mWeakShell);
  if (!tempAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = tempAccessible->GetStateInternal(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  if (gLastFocusedNode == mContent)
    *aState |= nsIAccessibleStates::STATE_FOCUSED;

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (menuList) {
    
    if (!mContent->AttrValueIs(kNameSpaceID_None,
                               nsAccessibilityAtoms::editable,
                               nsAccessibilityAtoms::_true, eIgnoreCase)) {
      *aState |= nsIAccessibleStates::STATE_READONLY;
    }
  }

  return NS_OK;
}

PRUint32
nsXULTextFieldAccessible::NativeRole()
{
  if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                            nsAccessibilityAtoms::password, eIgnoreCase))
    return nsIAccessibleRole::ROLE_PASSWORD_TEXT;
  return nsIAccessibleRole::ROLE_ENTRY;
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
    nsCOMPtr<nsIDOMXULElement> element(do_QueryInterface(mContent));
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

  nsCOMPtr<nsIContent> inputField = GetInputField();
  nsCOMPtr<nsIDOMNSEditableElement> editableElt(do_QueryInterface(inputField));
  NS_ENSURE_TRUE(editableElt, NS_ERROR_FAILURE);
  return editableElt->GetEditor(aEditor);
}




void
nsXULTextFieldAccessible::CacheChildren()
{
  
  
  nsCOMPtr<nsIContent> inputContent(GetInputField());
  if (!inputContent)
    return;

  nsAccTreeWalker walker(mWeakShell, inputContent, PR_FALSE);

  nsAccessible* child = nsnull;
  while ((child = walker.NextChild()) && AppendChild(child));
}




already_AddRefed<nsIContent>
nsXULTextFieldAccessible::GetInputField() const
{
  nsCOMPtr<nsIDOMNode> inputFieldDOMNode;
  nsCOMPtr<nsIDOMXULTextBoxElement> textBox = do_QueryInterface(mContent);
  if (textBox) {
    textBox->GetInputField(getter_AddRefs(inputFieldDOMNode));

  } else {
    
    nsCOMPtr<nsIDOMXULMenuListElement> menuList = do_QueryInterface(mContent);
    if (menuList)
      menuList->GetInputField(getter_AddRefs(inputFieldDOMNode));
  }

  NS_ASSERTION(inputFieldDOMNode, "No input field for nsXULTextFieldAccessible");

  nsIContent* inputField = nsnull;
  if (inputFieldDOMNode)
    CallQueryInterface(inputFieldDOMNode, &inputField);

  return inputField;
}
